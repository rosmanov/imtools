/**
 * \file WebSocket server.
 *
 * \copyright Copyright © 2014,2015 Ruslan Osmanov <rrosmanov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "imserver.hxx"

#define IMTOOLS_SERVER_OBJECT_LOG(__type, fmt, ...) \
  __type ## _log("[%s:%u] " fmt, m_host.c_str(), m_port, __VA_ARGS__)
#define IMTOOLS_SERVER_OBJECT_LOG0(__type, msg) \
  __type ## _log("[%s:%u] " msg, m_host.c_str(), m_port)

using namespace imtools;
using namespace imtools::imserver;
using boost::property_tree::ptree;
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// Declare static members for linker
Server::connection_list_type Server::s_connections;
Server::instance_list_type Server::s_instances;


static inline void
_registerSignalHandler(struct sigaction& sa, int signal)
{
  if (sigaction(signal, &sa, NULL) < 0) {
    throw ErrorException("Failed to register signal %d: %s", signal, strerror(errno));
  }
}


static inline void
registerSignals()
{
  verbose_log("Setting signal handlers\n");
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_sigaction = &Server::signalHandler;
  sa.sa_flags = SA_SIGINFO;
  _registerSignalHandler(sa, SIGTERM);
  _registerSignalHandler(sa, SIGINT);
}


/// Outputs help message to stdout or stderr depending on `is_error`.
static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, g_usage_template, g_program_name);
}


static std::unique_ptr<Command>
get_command(Command::Type type, const Command::element_vector_t& elements)
{
  std::unique_ptr<CommandFactory> factory_ptr;

  switch (type) {
    case Command::Type::RESIZE:
      factory_ptr = std::unique_ptr<CommandFactory>(new imtools::imresize::ResizeCommandFactory());
      break;

    case Command::Type::MERGE:
      throw ErrorException("Command::Type::MERGE not implemented");

    case Command::Type::UNKNOWN: // no break
    default:
      throw ErrorException("Unknown command code: %d", type);
  }

  return std::unique_ptr<Command>(factory_ptr->create(elements));
}


Server::~Server()
{
  try {
    s_instances.erase(shared_from_this());
  } catch (std::bad_weak_ptr e) {
    // skip
  }
}


Server::Server(uint16_t port, const std::string& host)
: m_port(port), m_host(host)
{
  websocketpp::lib::error_code ec;

  m_server.init_asio(ec);
  if (ec) {
    throw ErrorException(Server::getErrorMessage(ec));
  }

  m_server.set_reuse_addr(true);
  m_server.set_open_handler(bind(&Server::onOpen, this, ::_1));
  m_server.set_close_handler(bind(&Server::onClose, this, ::_1));
  m_server.set_message_handler(bind(&Server::onMessage, this, ::_1, ::_2));
}


Server::ptr_type
Server::create(uint16_t port, const std::string& host)
{
  ptr_type ps(new Server(port, host));
  s_instances.insert(ps);
  return ps;
}


Server::Option
Server::getOption(const std::string& k)
{
  if (k.length() < 4) {
    return Server::Option::UNKNOWN;
  }

  Server::Option option;

  switch (k[0]) {
    case 'p':
      option = k == "port" ? Server::Option::PORT : Server::Option::UNKNOWN;
      break;
    case 'h':
      option = k == "host" ? Server::Option::HOST: Server::Option::UNKNOWN;
      break;
    default:
      option = Server::Option::UNKNOWN;
      break;
  }

  return option;
}


void
Server::parseConfig(Server::ptr_list_type& server_list, const std::string& filename)
{
  ptree pt;

  try {
    verbose_log("Parsing configuration file '%s'\n", filename.c_str());
    boost::property_tree::ini_parser::read_ini(filename, pt);

    for (const auto& app_iter : pt) {
      std::string app_name = app_iter.first.data();
      debug_log("Parsing application '%s'\n", app_name.c_str());

      ptree pt_cfg = app_iter.second;
      uint16_t port = 9902;
      std::string host;

      for (const auto& cfg_iter : pt_cfg) {
        std::string k = cfg_iter.first.data();
        std::string v = cfg_iter.second.data();
        Server::Option option = Server::getOption(k);
        debug_log("k: %s v: %s o: %d\n", k.c_str(), v.c_str(), option);

        switch (option) {
          case Server::Option::HOST:
            host = v;
            break;
          case Server::Option::PORT:
            port = static_cast<uint16_t>(std::stoi(v));
            break;
          case Server::Option::UNKNOWN: // no break
          default:
            warning_log("Unknown option code: %d\n", option);
            break;
        }
      }

      if (port) {
        verbose_log("Adding server %s:%u\n", host.c_str(), port);
        server_list.push_back(ptr_type(Server::create(port, host)));
#ifndef IMTOOLS_THREADS
        if (pt.size() > 1) {
          warning_log("We are using only the first valid application, "
              "since threading support is disabled in this build.\n");
        }
        break;
#endif // ! IMTOOLS_THREADS
      } else {
        warning_log("No valid input values found for application '%s'. "
            "Skipping.\n", app_name.c_str());
      }
    }
  } catch (boost::property_tree::ini_parser_error& e) {
    throw ErrorException("Failed to parse configuration file: %s\n", e.what());
  } catch (std::exception& e) {
    throw ErrorException("Unhandled error in configuration parser: %s. "
        "Please file a bug.", e.what());
  }
}


void
Server::stop()
{
  websocketpp::lib::error_code ec;

  m_server.stop_listening(ec);
  if (ec) {
    IMTOOLS_SERVER_OBJECT_LOG(error, "Failed to stop listening: %s\n", Server::getErrorMessage(ec));
  }

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Closing connections");
  std::string reason;
  for (auto& it : s_connections) {
    m_server.close(it, websocketpp::close::status::normal, reason, ec);

    if (ec) {
      IMTOOLS_SERVER_OBJECT_LOG(error, "Failed to close connection: %s\n", Server::getErrorMessage(ec));
    }
  }
  if (!ec) {
    IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Connections closed successfully\n");
  }
}


void
Server::signalHandler(int signal, siginfo_t* siginfo, void*)
{
  verbose_log("Got signal '%d', PID: %ld, UID: %ld. Stopping server\n",
      signal, (long)siginfo->si_pid, (long)siginfo->si_uid);

  for (auto& it : Server::s_instances) {
    // For safety, copy into shared_ptr before usage
    if (auto shared_it = it.lock()) {
      shared_it->stop();
    }
  }
}


void
Server::run()
{
  websocketpp::lib::error_code ec;

  if (m_host == "") {
    m_server.listen(boost::asio::ip::tcp::v4(), m_port);
  } else {
    m_server.listen(m_host, std::to_string(m_port));
  }

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Starting server's async connection acceptance\n");
  m_server.start_accept(ec);
  if (ec) {
    throw ErrorException(Server::getErrorMessage(ec));
  }

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Running I/O service loop\n");
  m_server.run();
}


const char*
Server::getErrorMessage(const websocketpp::lib::error_code& ec)
{
  std::stringstream ecs;
  ecs << ec << " (" << ec.message() << ")";
  return (ecs.str()).c_str();
}


/// Unary operation for std::transform(). Converts property tree value to Command::element_t.
Command::element_t
Server::convertPtreeValue(const ptree::value_type& v)
{
  return Command::element_t(v.first.data(), v.second.data());
}


void
Server::onOpen(connection_hdl hdl) const
{
  s_connections.insert(hdl);
}


void
Server::onClose(connection_hdl hdl) const
{
  s_connections.erase(hdl);
}


void
Server::onMessage(connection_hdl hdl, websocket_server_type::message_ptr msg)
{
  IMTOOLS_SERVER_OBJECT_LOG0(debug, "Message sent to default handler\n");
  websocket_server_type::connection_ptr con = m_server.get_con_from_hdl(hdl);

  ptree pt;
  ptree pt_arg;
  std::stringstream ss(msg->get_payload());

  try {
    IMTOOLS_SERVER_OBJECT_LOG(debug, "Parsing JSON: %s\n", ss.str().c_str());
    boost::property_tree::json_parser::read_json(ss, pt);

    Command::element_vector_t elements;
    pt_arg = pt.get_child("arguments");
    std::transform(std::begin(pt_arg), std::end(pt_arg),
        std::back_inserter(elements), Server::convertPtreeValue);

    auto command_name = pt.get<std::string>("command");
    auto command_ptr = get_command(Command::getType(command_name), elements);
    IMTOOLS_SERVER_OBJECT_LOG(debug, "Running command: '%s'\n", command_name.c_str());
    command_ptr->run();
  } catch (boost::property_tree::json_parser_error& e) {
    error_log("Failed to parse json: %s, input: %s\n", e.what(), ss.str().c_str());
  } catch (ErrorException& e) {
    error_log("Fatal error: %s\n", e.what());
  } catch (...) {
    error_log("Unknown error in '%s'!!! Please file a bug.\n", __func__);
  }
}


static void
run(const std::string& config_filename)
{
  Server::ptr_list_type server_list;
  Server::parseConfig(server_list, config_filename);
  if (server_list.empty()) {
    error_log("No servers found in configuration file '%s'\n", config_filename.c_str());
    exit(2);
  }

#ifdef IMTOOLS_THREADS
  uint_t i;
  uint_t n_servers = server_list.size();
#ifdef USE_OPENMP
  IT_INIT_OPENMP(imtools::max_threads());

  _Pragma("omp parallel for")
    for (i = 0; i < n_servers; ++i) {
      Server::ptr_type server = server_list[i];
      verbose_log("Started server %s:%u\n", server->getHost().c_str(), server->getPort());
      server->run();
    }
#else // ! USE_OPENMP = use boost
  boost::threadpool::pool thread_pool(imtools::max_threads());
  boost::mutex mutex;

  for (i = 0; i < n_servers; ++i) {
    IT_SCOPED_LOCK(lock, mutex);
    Server::ptr_type server = server_list[i];
    thread_pool.schedule(boost::bind(&Server::run, server));
  }

  thread_pool.wait(); // Wait for threads to finish
#endif // USE_OPENMP
#else // no threads
  Server::ptr_type server = server_list[0];
  server->run();
#endif // IMTOOLS_THREADS
}


int
main(int argc, char **argv)
{
  int next_option;
  int exit_code   = 0;

  std::string config_filename;

  g_program_name = argv[0];

#ifdef IMTOOLS_DEBUG
  setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering
#endif

  debug_log0("Parsing CLI args\n");
  try {
    do {
      next_option = getopt_long(argc, argv, g_short_options, g_long_options, NULL);

      switch (next_option) {
        case 'h':
          usage(false);
          exit(0);

        case 'v':
          verbose++;
          break;

        case 'V':
          print_version();
          exit(0);

        case 'c':
          config_filename = optarg;
          break;

        case -1:
          // done with options
          break;

        case '?':
          // unrecognized option
          usage(true);
          exit(2);

        default:
          error_log("getopt returned character code 0%o\n", next_option);
          usage(true);
          exit(2);
      }
    } while (next_option != -1);
  } catch (imtools::InvalidCliArgException& e) {
    error_log("%s\n", e.what());
    exit(2);
  }

  if (config_filename.empty()) {
    config_filename = "server.cfg";
  }

#ifdef IMTOOLS_DEBUG
  char cwd[1024];
  if (! getcwd(cwd, sizeof(cwd))) {
    error_log("getcwd() failed: %s\n", strerror(errno));
    exit(2);
  }
  debug_log("Current directory: '%s'\n", cwd);
  debug_log("Configuration file: '%s'\n", config_filename.c_str());
#endif

  try {
    registerSignals();
    run(config_filename);
  } catch (ErrorException& e) {
    error_log("%s\n", e.what());
    exit_code = 1;
  } catch (boost::system::system_error& e) {
    error_log("%s\n", e.what());
    exit_code = 1;
  } catch (...) {
    error_log("Unknown error!!! Please file a bug.\n");
    exit_code = 1;
  }

  return exit_code;
}

// vim: et ts=2 sts=2 sw=2
