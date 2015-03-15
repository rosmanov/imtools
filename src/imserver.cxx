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

#include <sstream>
#include <signal.h>
#include <sys/wait.h>
#include <algorithm> // for std::transform()
#include <iterator> // for std::back_inserter()
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "imtools.hxx"

#define IMTOOLS_SERVER_OBJECT_LOG(__type, fmt, ...) \
  __type ## _log("[%s:%u] " fmt, getHost().c_str(), getPort(), __VA_ARGS__)
#define IMTOOLS_SERVER_OBJECT_LOG0(__type, msg) \
  __type ## _log("[%s:%u] " msg, getHost().c_str(), getPort())


using imtools::ErrorException;
using imtools::Command;
using imtools::CommandFactory;

using imtools::imserver::g_server;
using imtools::imserver::Config;
using imtools::imserver::Server;
using imtools::imserver::ConfigPtrList;

using boost::property_tree::ptree;


static void
wait_for_children()
{
  pid_t pid;
  int status;

  // Wait for all children
  while ((pid = waitpid(-1, &status, WUNTRACED | WCONTINUED)) > 0) {
    if (WIFEXITED(status)) {
      verbose_log("Child process %ld exited, status=%d\n", (long) pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      verbose_log("Child process %ld killed by signal %d\n", (long) pid, WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
      verbose_log("Child process %ld stopped by signal %d\n", (long) pid, WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
      verbose_log("Child process %ld continued\n", (long) pid);
    }
  }
}


static inline void
stop() noexcept
{
  if (g_server) {
    g_server->stop();
  }
  wait_for_children();
}


static void
signal_handler(int signal, siginfo_t* siginfo, void*)
{
  verbose_log("Got signal '%d', PID: %ld, UID: %ld. Waiting for child processes.\n",
      signal, (long) siginfo->si_pid, (long) siginfo->si_uid);
  stop();
}


static inline void
register_signal_handler(struct sigaction& sa, int signal)
{
  if (sigaction(signal, &sa, NULL) < 0) {
    throw ErrorException("Failed to register signal %d: %s", signal, strerror(errno));
  }
}


static inline void
register_signals()
{
  debug_log0("Setting signal handlers\n");

  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_sigaction = &signal_handler;
  sa.sa_flags = SA_SIGINFO | SA_NOCLDWAIT;

  register_signal_handler(sa, SIGTERM);
  register_signal_handler(sa, SIGINT);
}


/// Outputs help message to stdout or stderr depending on `is_error`.
static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr,
      imtools::imserver::g_usage_template, imtools::imserver::g_program_name);
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


Config::Option
Config::getOption(const std::string& k)
{
  if (k.length() < 4) {
    return Config::Option::UNKNOWN;
  }

  Config::Option option;

  switch (k[0]) {
    case 'p':
      option = k == "port" ? Config::Option::PORT : Config::Option::UNKNOWN;
      break;
    case 'h':
      option = k == "host" ? Config::Option::HOST : Config::Option::UNKNOWN;
      break;
    case 'c':
      option = k == "chdir" ? Config::Option::CHDIR : Config::Option::UNKNOWN;
      break;
    case 'a':
      option = k == "allow_absolute_paths" ? Config::Option::ALLOW_ABSOLUTE_PATHS : Config::Option::UNKNOWN;
      break;
    default:
      option = Config::Option::UNKNOWN;
      break;
  }

  return option;
}


ConfigPtrList
Config::parse(const std::string& filename)
{
  ptree pt;
  ConfigPtrList list;

  try {
    verbose_log("Parsing configuration file '%s'\n", filename.c_str());
    boost::property_tree::ini_parser::read_ini(filename, pt);
    list.reserve(pt.size());

    for (const auto& app_iter : pt) {
      std::string app_name = app_iter.first.data();
      debug_log("Parsing application '%s'\n", app_name.c_str());

      ptree pt_cfg = app_iter.second;
      uint16_t port = 9902;
      std::string host;
      bool allow_absolute_paths = false;
      std::string chdir_dir;

      for (const auto& cfg_iter : pt_cfg) {
        std::string k = cfg_iter.first.data();
        std::string v = cfg_iter.second.data();
        Config::Option option = Config::getOption(k);
        debug_log("k: %s v: %s o: %d\n", k.c_str(), v.c_str(), option);

        switch (option) {
          case Config::Option::HOST:
            host = v;
            break;
          case Config::Option::PORT:
            port = static_cast<uint16_t>(std::stoi(v));
            break;
          case Config::Option::CHDIR:
            chdir_dir = v;
            break;
          case Config::Option::ALLOW_ABSOLUTE_PATHS:
            allow_absolute_paths = (v == "true");
            break;
          case Config::Option::UNKNOWN: // no break
          default:
            warning_log("Unknown option code: %d\n", option);
            break;
        }
      }

      if (port) {
        verbose_log("Adding server %s:%u\n", host.c_str(), port);
        list.push_back(imtools::imserver::ConfigPtr(new Config(port, host,
                chdir_dir, allow_absolute_paths)));
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

  return list;
}


void
Server::stop()
{
  websocketpp::lib::error_code ec;

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Closing connections\n");

  m_server.stop_listening(ec);
  if (ec) {
    IMTOOLS_SERVER_OBJECT_LOG(error, "Failed to stop listening: %s\n", Server::getErrorMessage(ec));
  }

  std::string reason;
  for (auto& it : m_connections) {
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
Server::sendMessage(Connection conn, const std::string& message, MessageType type) noexcept
{
  ptree pt;

  try {
    std::stringstream json_stream;
    websocketpp::lib::error_code ec;

    // Generate JSON: {"error" : (int), "response" : "(string)"}
    pt.put("error", (int) (type == Server::MessageType::ERROR));
    pt.put("response", message);
    boost::property_tree::json_parser::write_json(json_stream, pt);

    m_server.send(conn, json_stream.str(), websocketpp::frame::opcode::text, ec);
    if (ec) {
      error_log("Fatal error in %s: %s\n", __func__, Server::getErrorMessage(ec));
    }
  } catch (boost::property_tree::ptree_bad_data& e) {
    error_log("Failed to generate JSON: %s\n", e.what());
  } catch (std::exception& e) {
    error_log("Fatal error: %s\n", e.what());
  } catch (...) {
    error_log("Unknown error in %s. File a bug\n", __func__);
  }
}


void
Server::onOpen(Connection conn) noexcept
{
  m_connections.insert(conn);
}


void
Server::onClose(Connection conn) noexcept
{
  m_connections.erase(conn);
}


void
Server::onMessage(Connection conn, WebSocketServer::message_ptr msg) noexcept
{
  IMTOOLS_SERVER_OBJECT_LOG0(debug, "Message sent to default handler\n");
  WebSocketServer::connection_ptr con = m_server.get_con_from_hdl(conn);

  ptree pt;
  ptree pt_arg;
  std::stringstream ss(msg->get_payload());
  std::string error;

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
    command_ptr->setAllowAbsolutePaths(getAllowAbsolutePaths());
    command_ptr->run();
  } catch (boost::property_tree::json_parser_error& e) {
    error = std::string("Invalid JSON: ") + e.what();
    error_log("Failed to parse json: %s, input: %s\n", e.what(), ss.str().c_str());
  } catch (ErrorException& e) {
    error = std::string("Fatal error: ") + e.what();
    error_log("%s\n", error.c_str());
  } catch (...) {
    error = "Internal Server Error";
    error_log("Unknown error in '%s'!!! Please file a bug.\n", __func__);
  }

  if (!error.empty()) {
    debug_log("sending error message: %s\n", error.c_str());
    sendMessage(conn, error, Server::MessageType::ERROR);
  }
}


void
Server::run()
{
  using websocketpp::lib::placeholders::_1;
  using websocketpp::lib::placeholders::_2;
  using websocketpp::lib::bind;

  websocketpp::lib::error_code ec;
  uint16_t port = getPort();
  std::string host(getHost());

  m_server.set_open_handler(bind(&Server::onOpen, this, ::_1));
  m_server.set_close_handler(bind(&Server::onClose, this, ::_1));
  m_server.set_message_handler(bind(&Server::onMessage, this, ::_1, ::_2));
  m_server.set_reuse_addr(true);

  m_server.init_asio(ec);
  if (ec) {
    throw ErrorException(Server::getErrorMessage(ec));
  }

  std::string chdir_dir(getChdirDir());
  if (!chdir_dir.empty()) {
    IMTOOLS_SERVER_OBJECT_LOG(debug, "chdir(%s)\n", chdir_dir.c_str());
    if (chdir(chdir_dir.c_str()) != 0) {
      throw ErrorException("chdir() failed: %s", strerror(errno));
    }
  }

  if (host == "") {
    // XXX detect whether IPv6 is available
    m_server.listen(boost::asio::ip::tcp::v4(), port);
  } else {
    m_server.listen(host, std::to_string(port));
  }

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Starting connection acceptance\n");
  m_server.start_accept(ec);
  if (ec) {
    throw ErrorException(Server::getErrorMessage(ec));
  }

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Running I/O service loop\n");
  m_server.run();
}


const char*
Server::getErrorMessage(const websocketpp::lib::error_code& ec) noexcept
{
  std::stringstream ecs;
  ecs << ec << " (" << ec.message() << ")";
  return (ecs.str()).c_str();
}


Command::element_t
Server::convertPtreeValue(const ptree::value_type& v) noexcept
{
  return Command::element_t(v.first.data(), v.second.data());
}



static void
run(const std::string& config_filename)
{
  pid_t child_pid;
  ConfigPtrList config_list = Config::parse(config_filename);

  if (config_list.empty()) {
    error_log("Configuration file is empty\n");
    exit(EXIT_FAILURE);
  }

  register_signals();

  for (auto& config : config_list) {
    child_pid = fork();

    if (child_pid == -1) {
      error_log("Failed to fork()\n");
      _exit(EXIT_FAILURE);
    }

    if (child_pid == 0) { // child process
      g_server = imtools::imserver::ServerPtr(new Server(config));
      g_server->run();
      _exit(EXIT_SUCCESS);
    }

    verbose_log("Forked child, PID: %ld\n", static_cast<long>(child_pid));
  }

  // The following goes within parent process
  stop();
}


int
main(int argc, char **argv)
{
  int next_option;
  int exit_code   = 0;

  imtools::imserver::g_program_name = argv[0];

  std::string config_filename;

#ifdef IMTOOLS_DEBUG
  setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering
#endif

  debug_log0("Parsing CLI args\n");
  try {
    do {
      next_option = getopt_long(argc, argv,
          imtools::imserver::g_short_options,
          imtools::imserver::g_long_options, NULL);

      switch (next_option) {
        case 'h':
          usage(false);
          exit(0);

        case 'v':
          imtools::verbose++;
          break;

        case 'V':
          imtools::print_version();
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
