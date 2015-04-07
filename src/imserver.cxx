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

#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif
#if 0
#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif
#endif
#include <signal.h>
#include <sys/wait.h>
#include <sstream>
#include <iomanip> // std::hex, std::fill, std::setw for SHA-1
#include <algorithm> // for std::transform()
#include <iterator> // for std::back_inserter()
#include <stdlib.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/asio/signal_set.hpp>

#if 0
#include <boost/uuid/sha1.hpp>
#else
#include <websocketpp/sha1/sha1.hpp>
#endif

#include "imtools.hxx"

/////////////////////////////////////////////////////////////////////

#ifndef MAX_LINE_LENGTH
# define MAX_LINE_LENGTH 1024
#endif

#define IMTOOLS_SERVER_OBJECT_LOG(__type, fmt, ...) \
  __type ## _log("[%s:%u] " fmt, getHost().c_str(), getPort(), __VA_ARGS__)
#define IMTOOLS_SERVER_OBJECT_LOG0(__type, msg) \
  __type ## _log("[%s:%u] " msg, getHost().c_str(), getPort())

/////////////////////////////////////////////////////////////////////

using imtools::ErrorException;
using imtools::Command;
using imtools::CommandFactory;
using imtools::CommandResult;

using imtools::imserver::g_server;
using imtools::imserver::g_daemonize;
using imtools::imserver::g_config_file;
using imtools::imserver::Config;
using imtools::imserver::AppConfig;
using imtools::imserver::Server;
using imtools::imserver::Util;
using imtools::imserver::AppConfigPtr;
using imtools::imserver::ConfigPtr;
using imtools::imserver::AppConfigPtrList;

using boost::property_tree::ptree;

/////////////////////////////////////////////////////////////////////

/// Whether we caught SIGTERM or SIGINT signal
static std::atomic_int g_got_sigterm{0};
/// Whether we caught SIGHUP signal
static std::atomic_int g_got_sighup{0};
/// Whether we caught SIGCHLD signal
static std::atomic_int g_got_sigchld{0};
/// Child process IDs
static std::set<pid_t> g_children;
/// Pid file descriptor
static int g_pid_fd{-1};
/// Original signal mask. Should be initialized only once at startup.
static sigset_t g_org_sigmask;
/// Original umask
static mode_t g_saved_umask;

/////////////////////////////////////////////////////////////////////
// Initalizing static member variables

bool Config::s_daemonize{false};
std::string Config::s_pid_file{"/var/run/imtools-server.pid"};
std::string Config::s_error_log_file{"/var/log/imtools-server.err"};

Config::LogLevel Config::s_log_level =
#ifdef IMTOOLS_DEBUG
Config::LogLevel::DEBUG;
#else
Config::LogLevel::NOTICE;
#endif

/////////////////////////////////////////////////////////////////////
// Forward decl.

static void run(AppConfigPtrList& config_list);

/////////////////////////////////////////////////////////////////////

/// Closes and removes PID file
static inline void
pidfile_remove()
{
  if (g_pid_fd >= 0) {
    debug_log0("Removing pid file");
    ::close(g_pid_fd);
    ::unlink(Config::getPidFilePath());
    g_pid_fd = -1;
  }
}


/*! Locks PID file
 * \param relock If `true`, then contents of the PID file is overwritten with current PID. */
static bool
pidfile_lock(bool relock = false)
{
  bool success = false;
  const char* lock_file = Config::getPidFilePath();

  do {
    if ((g_pid_fd = ::open(lock_file, O_RDWR | O_CREAT | O_CLOEXEC, 0640)) < 0) {
      error_log("failed to open lock file %s: %s, euid: %ld, egid: %ld",
          lock_file, strerror(errno), (long) geteuid(), (long) getegid());
      break;
    }

    if (!relock) {
      if (lockf(g_pid_fd, F_TLOCK, 0) < 0) {
        debug_log0("lockf: daemon is already running");
        pidfile_remove();
        exit(EXIT_SUCCESS);
      }
    }

    if (ftruncate(g_pid_fd, 0) == -1) {
      error_log("ftruncate: %s", strerror(errno));
      break;
    }

    char str[MAX_LINE_LENGTH];
    snprintf(str, sizeof(str), "%ld\n", static_cast<long>(getpid()));
    if (::write(g_pid_fd, str, strlen(str)) <= 0) {
      error_log("write: %s", strerror(errno));
      break;
    }

    debug_log0("Locked/updated pid file");
    success = true;
  } while (0);

  return (success);
}


/// Waits for all child processes to terminate
static void
wait_for_children()
{
  pid_t pid;
  int status;

  debug_log0("waiting for child processes to terminate");
  while ((pid = waitpid(-1, &status, WUNTRACED | WCONTINUED)) > 0) {
    g_children.erase(pid);

    if (WIFEXITED(status)) {
      verbose_log("Child process %ld exited, status=%d", (long) pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      verbose_log("Child process %ld killed by signal %d", (long) pid, WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
      verbose_log("Child process %ld stopped by signal %d", (long) pid, WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
      verbose_log("Child process %ld continued", (long) pid);
    }
  }
}


static inline void
shutdown() noexcept
{
  verbose_log("Shutdown in progress\n");

  if (killpg(0, SIGQUIT) != 0) {
    error_log("killpg: %s", strerror(errno));
  }

  // Wait for child processes to terminate
  wait_for_children();

  // Note, normally, `shutdown` is called from the watchdog process where
  // `g_server` is empty.
  if (g_server) {
    debug_log("stopping g_server, use count: %ld", g_server.use_count());
    g_server->stop();
  }

  // Unlock/remove PID file
  pidfile_remove();

  verbose_log("Shutdown completed\n");
}


Server::Server(const AppConfigPtr& config) noexcept
: m_config(config)
{
}


bool
Server::setupUser() noexcept
{
  uid_t uid{m_uid};
  gid_t gid{m_gid};
  struct ::passwd pwd;
  struct ::group grp;
  std::string user{getUser()};
  std::string group{getGroup()};

  if (user.empty() && group.empty()) {
    return true;
  }

  if (geteuid() != 0) {
    IMTOOLS_SERVER_OBJECT_LOG0(warning, "no sufficient privileges to change user/group");
    return true;
  }

  const char* user_c_str  = user.empty()  ? nullptr : user.c_str();
  const char* group_c_str = group.empty() ? nullptr : group.c_str();

  IMTOOLS_SERVER_OBJECT_LOG(debug, "Configuring uid/gid, user: %s, group: %s", user_c_str, group_c_str);

  if (group_c_str != nullptr) {
    if (!Util::getUserGroupData(group_c_str, grp)) {
      IMTOOLS_SERVER_OBJECT_LOG(error, "failed to retrieve group data for group '%s'", group_c_str);
      return false;
    }
    gid = grp.gr_gid;
  }

  if (user_c_str != nullptr) {
    if (!Util::getUserData(user_c_str, pwd)) {
      IMTOOLS_SERVER_OBJECT_LOG(error, "failed to retrieve passwd data for user '%s'", user_c_str);
      return false;
    }
    uid = pwd.pw_uid;
  }

  if (uid != m_uid || gid != m_gid) {
    if (::chown(getErrorLogFile().c_str(), uid, gid) < 0) {
      IMTOOLS_SERVER_OBJECT_LOG(warning, "chown: %s", strerror(errno));
    }
  }

  if (gid != m_gid) {
    IMTOOLS_SERVER_OBJECT_LOG(debug, "Setting gid = %d", static_cast<int>(gid));
    if (setgid(gid) < 0) {
      IMTOOLS_SERVER_OBJECT_LOG(error, "setgid: %s", strerror(errno));
      return false;
    }
    m_gid = gid;
  }

  if (uid != m_uid) {
    IMTOOLS_SERVER_OBJECT_LOG(debug, "Setting uid = %d", static_cast<int>(uid));
    if (initgroups(user_c_str, gid) < 0) {
      IMTOOLS_SERVER_OBJECT_LOG(error, "initgroups: %s", strerror(errno));
      return false;
    }
    if (setuid(uid) < 0) {
      IMTOOLS_SERVER_OBJECT_LOG(error, "setuid: %s", strerror(errno));
      return false;
    }
    m_uid = uid;
  }

  return true;
}


/*! Terminates worker processes, re-reads configuration file and starts new
 * workers correspondingly. */
static void
reconfigure(AppConfigPtrList& config_list)
{
  debug_log("Re-configuration in progress, g_server use count: %ld", g_server.use_count());

  // Perform total cleanup
  shutdown();

  // Update PID file
  if (!pidfile_lock(true)) {
    exit(EXIT_FAILURE);
  }

  // Re-read configuration file
  config_list = Config::parse(g_config_file);

  debug_log("Re-configuration completed, g_server use count: %ld", g_server.use_count());
}


/// Crash signals handler
static void
crash_handler(int sig)
{
  error_log("Caught signal %d", sig);

#ifdef IMTOOLS_DEBUG
  signal(sig, SIG_DFL);
  if (raise(sig) != 0) {
    debug_log("raise(%d) failed", sig);
    exit(EXIT_FAILURE);
  }
#else
  exit(EXIT_FAILURE);
#endif
}


/// Outputs help message to stdout or stderr depending on `is_error`.
static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr,
      imtools::imserver::g_usage_template, imtools::imserver::g_program_name);
}


/// \returns pointer to server command instance
/// \throws ErrorException
static imtools::imserver::CommandPtr
get_command(Command::Type type, const Command::Arguments& arguments)
{
  std::unique_ptr<CommandFactory> factory_ptr;

  switch (type) {
    case Command::Type::META:
      factory_ptr = std::unique_ptr<CommandFactory>(new imtools::MetaCommandFactory());
      break;

    case Command::Type::RESIZE:
      factory_ptr = std::unique_ptr<CommandFactory>(new imtools::imresize::ResizeCommandFactory());
      break;

    case Command::Type::MERGE:
      factory_ptr = std::unique_ptr<CommandFactory>(new imtools::immerge::MergeCommandFactory());
      break;

    case Command::Type::UNKNOWN: // no break
    default:
      throw ErrorException("Unknown command code: %d", type);
  }

  debug_log0("Creating CommandPtr");
  return imtools::imserver::CommandPtr(factory_ptr->create(arguments));
}


AppConfig::Option
AppConfig::getOption(const std::string& k)
{
  typedef AppConfig::Option Option;

  Option option;

  switch (k[0]) {
    case 'a': option = k == "allow_absolute_paths" ? Option::ALLOW_ABSOLUTE_PATHS : Option::UNKNOWN; break;
    case 'e': option = k == "error_log"            ? Option::ERROR_LOG_FILE       : Option::UNKNOWN; break;
    case 'g': option = k == "group"                ? Option::GROUP                : Option::UNKNOWN; break;
    case 'h': option = k == "host"                 ? Option::HOST                 : Option::UNKNOWN; break;
    case 'k': option = k == "key"                  ? Option::PRIVATE_KEY          : Option::UNKNOWN; break;
    case 'p': option = k == "port"                 ? Option::PORT                 : Option::UNKNOWN; break;
    case 'u': option = k == "user"                 ? Option::USER                 : Option::UNKNOWN; break;
    case 'c':
      if (k == "chdir") {
        option = Option::CHDIR;
      } else if (k == "chroot") {
        option = Option::CHROOT;
      } else {
        option = Option::UNKNOWN;
      }
      break;
    default:
      option = Option::UNKNOWN;
      break;
  }

  return option;
}


Config::Option
Config::getOption(const std::string& k)
{
  Config::Option option;

  switch (k[0]) {
    case 'd':
      option = k == "daemonize" ? Config::Option::DAEMONIZE : Config::Option::UNKNOWN;
      break;
    case 'e':
      option = k == "error_log" ? Config::Option::ERROR_LOG_FILE : Config::Option::UNKNOWN;
      break;
    case 'l':
      option = k == "log_level" ? Config::Option::LOG_LEVEL : Config::Option::UNKNOWN;
      break;
    case 'p':
      option = k == "pid" ? Config::Option::PID_FILE : Config::Option::UNKNOWN;
      break;
    default:
      option = Config::Option::UNKNOWN;
      break;
  }

  return option;
}


Config::LogLevel
Config::getLogLevel(const std::string& k) noexcept
{
  Config::LogLevel level;

  switch (k[0]) {
    case 'e': level = k == "error"   ? Config::LogLevel::ERROR   : Config::LogLevel::NONE; break;
    case 'w': level = k == "warning" ? Config::LogLevel::WARNING : Config::LogLevel::NONE; break;
    case 'n': level = k == "notice"  ? Config::LogLevel::NOTICE  : Config::LogLevel::NONE; break;
    case 'i': level = k == "info"    ? Config::LogLevel::INFO    : Config::LogLevel::NONE; break;
    case 'd': level = k == "debug"   ? Config::LogLevel::DEBUG   : Config::LogLevel::NONE; break;
    default:
      level = Config::LogLevel::NONE;
  }

  return level;
}


bool
Util::getUserGroupData(const char* name, struct ::group& grp) noexcept
{
  struct ::group *grp_ptr = &grp;
  struct ::group *temp_grp_ptr;

  long bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
  if (bufsize == -1) {
    bufsize = 16384; // should be more than enough
  }

  char* buff = static_cast<char*>(malloc(bufsize));
  if (buff == NULL) {
    error_log("malloc: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  int ret = getgrnam_r(name, grp_ptr, buff, bufsize, &temp_grp_ptr);
  free(buff);

  if (temp_grp_ptr == nullptr) {
    if (ret == 0)
      error_log("No matching group record was found: %s", name);
    else
      error_log("getgrnam_r: %s", strerror(errno));
    return false;
  }

  return true;
}


bool
Util::getUserData(const char* name, struct ::passwd& pwd) noexcept
{
  struct ::passwd *pwd_ptr = &pwd;
  struct ::passwd *temp_pwd_ptr;

  long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1) {
    bufsize = 16384; // should be more than enough
  }

  char* buff = static_cast<char*>(malloc(bufsize));
  if (buff == NULL) {
    error_log("malloc: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  int s = getpwnam_r(name, pwd_ptr, buff, bufsize, &temp_pwd_ptr);
  free(buff);

  return (s == 0 && pwd_ptr);
}


void
Config::set(const std::string& k, const std::string& v) noexcept
{
  typedef Config::Option Option;

  auto option = getOption(k);
  debug_log("gk: %s gv: %s go: %d", k.c_str(), v.c_str(), option);

  switch (option) {
    case Option::PID_FILE:       s_pid_file       = v;                                  break;
    case Option::ERROR_LOG_FILE: s_error_log_file = v;                                  break;
    case Option::DAEMONIZE:      s_daemonize      = (v=="yes");                         break;
    case Option::LOG_LEVEL:      s_log_level      = getLogLevel(static_cast<std::string const&>(v)); break;
    case Option::UNKNOWN: // no break
    default:
      warning_log("Unknown option code: %d", option);
      break;
  }
}


void
AppConfig::set(const std::string& k, const std::string& v) noexcept
{
  typedef AppConfig::Option Option;

  auto option = getOption(k);

  debug_log("k: %s v: %s o: %d", k.c_str(), v.c_str(), option);

  switch (option) {
    case Option::ALLOW_ABSOLUTE_PATHS:
      m_allow_absolute_paths = (v == "yes");
      break;
    case Option::PORT:           m_port      = static_cast<uint16_t>(std::stoi(v)); break;
    case Option::HOST:           m_host      = v;                                   break;
    case Option::CHDIR:          m_chdir     = v;                                   break;
    case Option::PRIVATE_KEY:    m_key       = v;                                   break;
    case Option::CHROOT:         m_chroot    = v;                                   break;
    case Option::USER:           m_user      = v;                                   break;
    case Option::GROUP:          m_group     = v;                                   break;
    case Option::ERROR_LOG_FILE: m_error_log = v;                                   break;
    case Option::UNKNOWN: // no break
    default: warning_log("Unknown option code: %d", option); break;
  }
}


AppConfigPtrList
Config::parse(const std::string& filename)
{
  ptree pt;
  AppConfigPtrList list;

  try {
    verbose_log("Parsing configuration file '%s'", filename.c_str());
    boost::property_tree::ini_parser::read_ini(filename, pt);
    list.reserve(pt.size());

    for (const auto& app_iter : pt) {
      std::string section = app_iter.first.data();
      ptree pt_cfg = app_iter.second;

      if (section == "global") {
        debug_log0("Parsing global configuration");
        for (const auto& cfg_iter : pt_cfg) {
          set(cfg_iter.first.data(), cfg_iter.second.data());
        }
        continue;
      }

      debug_log("Parsing application '%s'", section.c_str());
      auto* ac = new AppConfig(section);
      for (const auto& cfg_iter : pt_cfg) {
        ac->set(cfg_iter.first.data(), cfg_iter.second.data());
      }
      if (ac->getErrorLogFile().empty()) {
        ac->set("error_log", std::string{"/var/log/imtools-server."} + section + ".log");
      }

      if (*ac) {
        verbose_log("Adding conf for server %s:%u", ac->getHost().c_str(), ac->getPort());
        list.push_back(AppConfigPtr(ac));
      } else {
        warning_log("No valid input values found for application '%s'. "
            "Skipping.\n", section.c_str());
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

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Closing connections");

  m_server.stop_listening(ec);
  if (ec) {
    IMTOOLS_SERVER_OBJECT_LOG(error, "Failed to stop listening: %s", Util::getErrorMessage(ec));
  }

  std::string reason;
  for (auto& it : m_connections) {
    m_server.close(it, websocketpp::close::status::going_away, reason, ec);

    if (ec) {
      IMTOOLS_SERVER_OBJECT_LOG(error, "Failed to close connection: %s", Util::getErrorMessage(ec));
    }
  }

  if (!ec) {
    IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Connections closed successfully");
  }

  m_server.stop();
}


void
Server::sendMessage(Connection conn, const std::string& message, const std::string& digest, MessageType type) noexcept
{
  ptree pt;

  try {
    std::stringstream json_stream;
    websocketpp::lib::error_code ec;

    // Generate JSON: {"error" : (int), "response" : "(string)", "digest" : "(string)"}
    pt.put("error", (int) (type == Server::MessageType::ERROR));
    pt.put("response", message);
    pt.put("digest", digest);
#ifdef IMTOOLS_DEBUG
    // Pretty-print JSON
    boost::property_tree::json_parser::write_json(json_stream, pt, true);
#else
    boost::property_tree::json_parser::write_json(json_stream, pt, false);
#endif

    m_server.send(conn, json_stream.str(), websocketpp::frame::opcode::text, ec);
    if (ec) {
      error_log("Fatal error in %s: %s", __func__, Util::getErrorMessage(ec));
    }
  } catch (boost::property_tree::ptree_bad_data& e) {
    error_log("Failed to generate JSON: %s", e.what());
  } catch (std::exception& e) {
    error_log("Fatal error: %s", e.what());
  } catch (...) {
    error_log("Unknown error in %s. File a bug", __func__);
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
Server::onFail(Connection conn) noexcept
{
  WebSocketServer::connection_ptr con = m_server.get_con_from_hdl(conn);
  warning_log("Connection failed: %s", con->get_ec().message().c_str());
}


void
Server::onMessage(Connection conn, WebSocketServer::message_ptr msg) noexcept
{
  IMTOOLS_SERVER_OBJECT_LOG0(debug, "Message sent to default handler");
  WebSocketServer::connection_ptr con = m_server.get_con_from_hdl(conn);

  ptree pt;
  std::string error;
  std::string input_digest;
  Command::Arguments arguments;
  std::stringstream ss(msg->get_payload());

  try {
    IMTOOLS_SERVER_OBJECT_LOG(debug, "Parsing JSON: %s", ss.str().c_str());
    boost::property_tree::json_parser::read_json(ss, pt);

    auto pt_arg       = pt.get_child("arguments");
    auto command_name = pt.get<std::string>("command");
    input_digest      = pt.get<std::string>("digest");

    IMTOOLS_SERVER_OBJECT_LOG0(debug, "Started Transforming ptree values");
    std::transform(std::begin(pt_arg), std::end(pt_arg),
        std::back_inserter(arguments), Util::convertPtreeValue);
    IMTOOLS_SERVER_OBJECT_LOG0(debug, "Finished transforming ptree values");

    auto command_ptr = get_command(Command::getType(command_name), arguments);

    IMTOOLS_SERVER_OBJECT_LOG(debug, "Checking digest for command '%s'", command_name.c_str());
    if (!checkCommandDigest(*command_ptr, input_digest)) {
      throw ErrorException("Invalid digest");
    }

    IMTOOLS_SERVER_OBJECT_LOG(debug, "Running command: '%s'", command_name.c_str());
    command_ptr->setAllowAbsolutePaths(getAllowAbsolutePaths());
    CommandResult result;
    command_ptr->run(result);

    if (result) {
      sendMessage(conn, result, input_digest, Server::MessageType::SUCCESS);
      return;
    }

    error = "Empty result";
  } catch (boost::property_tree::json_parser_error& e) {
    error = std::string("Invalid JSON: ") + e.what();
    error_log("Failed to parse json: %s, input: %s", e.what(), ss.str().c_str());
  } catch (ErrorException& e) {
    error = std::string("Fatal error: ") + e.what();
    error_log("%s", error.c_str());
  } catch (...) {
    error = "Internal Server Error";
    error_log("Unknown error in '%s'!!! Please file a bug.", __func__);
  }

  debug_log("sending error message: %s", error.c_str());
  sendMessage(conn, error, input_digest, Server::MessageType::ERROR);
}

void
Server::sigtermHandler(boost::system::error_code ec, int signal_number) noexcept
{
  if (!ec) {
    IMTOOLS_SERVER_OBJECT_LOG(verbose, "Caught signal %d. Stopping", signal_number);
    m_server.stop();
  }
}


void
Server::crashHandler(boost::system::error_code ec, int signal_number) noexcept
{
  if (!ec) {
    IMTOOLS_SERVER_OBJECT_LOG(verbose, "Caught signal %d. Aborting", signal_number);
    crash_handler(signal_number);
  }
}


bool
Server::openErorLog() noexcept
{
  int error_log_fd;

  if ((error_log_fd = ::open(getErrorLogFile().c_str(),
          O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC,
          S_IRUSR | S_IWUSR)) == -1)
  {
    IMTOOLS_SERVER_OBJECT_LOG(error, "failed to open error log file '%s': %s",
        getErrorLogFile().c_str(), strerror(errno));
    return false;
  }

  /* Main process log file is handled only by STDERR_FILENO and STDOUT_FILENO
   * descriptors. On success ::dup2() will close the second FD silently. So we
   * don't need to close any FD explicitly.
   */
  if (::dup2(error_log_fd, STDERR_FILENO) < 0 ||
      ::dup2(error_log_fd, STDOUT_FILENO) < 0)
  {
    IMTOOLS_SERVER_OBJECT_LOG(error, "failed to configure error log, dup2: %s",
        strerror(errno));
    return false;
  }
  if (::close(error_log_fd) < 0) {
    IMTOOLS_SERVER_OBJECT_LOG(warning, "failed to close error log file descriptor %d: %s",
        error_log_fd, strerror(errno));
    return false;
  }

  return true;
}


void
Server::run()
{
  IMTOOLS_SERVER_OBJECT_LOG0(debug, "Server::run()");
  using websocketpp::lib::placeholders::_1;
  using websocketpp::lib::placeholders::_2;
  using websocketpp::lib::bind;
  using websocketpp::lib::ref;

  websocketpp::lib::error_code ec;
  uint16_t port = getPort();
  std::string host(getHost());

  m_server.set_open_handler(bind(&Server::onOpen, this, ::_1));
  m_server.set_close_handler(bind(&Server::onClose, this, ::_1));
  m_server.set_message_handler(bind(&Server::onMessage, this, ::_1, ::_2));
  m_server.set_fail_handler(bind(&Server::onFail, this, ::_1));

  m_server.set_reuse_addr(true);

  m_server.init_asio(ec);
  if (ec) {
    throw ErrorException(Util::getErrorMessage(ec));
  }

  auto& io_service = m_server.get_io_service();
  boost::asio::signal_set term_signals(io_service, SIGINT, SIGTERM, SIGQUIT);
  //term_signals.add(SIGHUP);
  term_signals.async_wait(boost::bind(&Server::sigtermHandler, this, ::_1, ::_2));

  boost::asio::signal_set crash_signals(io_service, SIGSEGV, SIGBUS, SIGABRT);
  crash_signals.add(SIGILL);
  crash_signals.add(SIGFPE);
  crash_signals.async_wait(boost::bind(&Server::crashHandler, this, ::_1, ::_2));

  bool chroot_done = false;
  std::string chroot_dir(getChrootDir());
  if (!chroot_dir.empty()) {
    IMTOOLS_SERVER_OBJECT_LOG(debug, "chroot(%s)", chroot_dir.c_str());
    if (::chroot(chroot_dir.c_str()) < 0) {
      throw ErrorException("chroot() failed: %s", strerror(errno));
    }
    chroot_done = true;
  }

  std::string chdir_dir(getChdirDir());
  if (!chdir_dir.empty()) {
    IMTOOLS_SERVER_OBJECT_LOG(debug, "chdir(%s)", chdir_dir.c_str());
    if (::chdir(chdir_dir.c_str()) != 0) {
      throw ErrorException("chdir() failed: %s", strerror(errno));
    }
  } else if (chroot_done) {
    IMTOOLS_SERVER_OBJECT_LOG0(debug, "chdir(/)");
    ::chdir("/");
  }

  if (!openErorLog()) {
    throw ErrorException("worker failed to open log");
  }

  if (!setupUser()) {
    throw ErrorException("worker user/group configuration failed");
  }

#ifdef HAVE_PR_SET_DUMPABLE
  if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) != 0) {
    throw ErrorException("prctl(PR_SET_DUMPABLE): %s", strerror(errno));
  }
#endif

  if (host == "") {
    // XXX detect whether IPv6 is available
    m_server.listen(boost::asio::ip::tcp::v4(), port);
  } else {
    m_server.listen(host, std::to_string(port));
  }

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Starting connection acceptance");
  m_server.start_accept(ec);
  if (ec) {
    throw ErrorException(Util::getErrorMessage(ec));
  }

  IMTOOLS_SERVER_OBJECT_LOG0(verbose, "Running I/O service loop");
  m_server.run();
}


const char*
Util::getErrorMessage(const websocketpp::lib::error_code& ec) noexcept
{
  std::stringstream ecs;
  ecs << ec << " (" << ec.message() << ")";
  return (ecs.str()).c_str();
}


Command::ArgumentItem
Util::convertPtreeValue(const ptree::value_type& v) noexcept
{
  Command::Value* value_ptr;

  if (v.second.size()) {
    debug_log("Util::convertPtreeValue: key = %s, array size = %ld", v.first.c_str(), v.second.size());
    Command::Value::ArrayType array;
    array.reserve(v.second.size());

    for (auto& it : v.second) {
      if (it.second.size() == 0) {
        debug_log("Util::convertPtreeValue: key = %s, push_back(%s)", v.first.c_str(), it.second.data().c_str());
        array.push_back(it.second.data());
      }
    }

    value_ptr = new Command::ArrayValue(array);
  } else {
    value_ptr = new Command::StringValue(v.second.data());
  }

  return Command::ArgumentItem(v.first.data(), Command::CValuePtr(value_ptr));
}


std::string
Util::makeSHA1(const std::string& source) noexcept
{
  std::ostringstream oss;

#if 0 // use boost::uuid::detail::sha1
  unsigned int hash[5];
  boost::uuids::detail::sha1 sha1;

  sha1.process_bytes(source.c_str(), source.size());
  sha1.get_digest(hash);

  oss << std::hex << std::setfill('0') << std::setw(sizeof(hash[0]) * 2);
  for (std::size_t i = 0; i < sizeof(hash) / sizeof(hash[0]); ++i) {
    oss << hash[i];
  }
#else // use websocketpp::sha1
  unsigned char hash[20];
  websocketpp::sha1::calc(source.c_str(), source.length(), hash);

  oss << std::hex << std::setfill('0');
  for (const unsigned char* ptr = hash; ptr < hash + 20; ptr++) {
    oss << std::setw(2) << (unsigned int) *ptr;
  }
#endif

  return oss.str();
}


/*! Redirects stdin to /dev/null; stdout and stderr - to log file
 * \param path Path to the log file.
 */
static void
redirect_to_log(const char* path)
{
  debug_log("redirecting stdin, stdout and stderr to file %s", path);

  // Re-open stdin
  ::close(STDIN_FILENO);
  if (::open("/dev/null", O_RDONLY) == -1) {
    error_log("failed to reopen stdin while daemonizing: %s", strerror(errno));
    return;
  }

  // Redirect std
  int fd;
  if ((fd = ::open(path, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1) {
    error_log("failed to open log path %s: %s", path, strerror(errno));
    return;
  }

  ::dup2(fd, STDOUT_FILENO);
  ::dup2(fd, STDERR_FILENO);
  ::close(fd);
}


/*! Creates a daemon based on calling process. */
static void
daemonize()
{
  pid_t pid;

  // Check if the process is already daemonized
  // (the 'init' process is parent)
  if (getppid() == 1) {
    debug_log("%s called while the process is already daemonized", __func__);
    return;
  }

  // Detach from the terminal in a new process
  if ((pid = fork()) < 0) {
    error_log("fork: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Terminate parent process
  if (pid > 0) {
    debug_log("forked child %ld", static_cast<long>(pid));
    exit(EXIT_SUCCESS);
  }

  // Place the server in a new process group and session and detach its
  // controlling terminal.
  if (setsid() < 0) {
    error_log("setsid: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  debug_log0("created new session");

  // Set permissions for new files
  g_saved_umask = umask(027);

  // Prevent zombies by means of a second fork.
  // Second fork guarantees that the child is no longer a session leader,
  // preventing the daemon from ever acquiring a controlling terminal.
  if ((pid = fork()) < 0) {
    error_log("fork: %s", strerror(errno));
    exit(EXIT_FAILURE);
  } else if (pid != 0) {
    debug_log("forked child %ld", static_cast<long>(pid));

    // Make child process the leader of its own process group. This allows
    // signals to also be delivered to processes forked by the child process.
    setpgid(pid, 0);

    // Terminate parent process
    exit(EXIT_SUCCESS);
  }

  debug_log0("chdir(/)");
  if (chdir("/") != 0) {
    error_log("chdir: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Ensure that only one daemon instance is running
  if (!pidfile_lock()) {
    exit(EXIT_FAILURE);
  }

  // Redirect standard descriptors to log file
  redirect_to_log(Config::getErrorLogFile().c_str());
}


/*! Waits for registered signals and handles them. */
static void
process_signals()
{
  sigset_t sigmask;
  sigset_t old_sigmask;

  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGTERM);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGHUP);
  sigaddset(&sigmask, SIGCHLD);
  // SIGQUIT is handled by child processes

  if (sigprocmask(SIG_BLOCK, &sigmask, &old_sigmask) != 0) {
    error_log("sigprocmask: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for (;;) {
    sigsuspend(&old_sigmask);

    if (g_got_sigterm) {
      verbose_log("Caught termination signal");
      shutdown();
      exit(EXIT_SUCCESS);
    }

    if (g_got_sighup) {
      verbose_log("Caught SIGHUP");
      debug_log0("Broadcasting SIGQUIT");
      if (killpg(0, SIGQUIT) != 0) {
        error_log("killpg: %s", strerror(errno));
      }
      break;
    }

    if (g_got_sigchld) {
      debug_log0("Caught SIGCHLD");
      g_got_sigchld = 0;
      wait_for_children();
      if (g_children.empty()) {
        verbose_log("last child has terminated, so leaving signal handler loop.\n");
        shutdown();
        break;
      }
    }
  }

  sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
}


static void sigterm_handler(int) { g_got_sigterm = 1; }
static void sigchld_handler(int) { g_got_sigchld = 1; }
static void sighup_handler(int)  { g_got_sighup  = 1; }


/*! Configures signal handlers for the watchdog process process. */
static bool
configure_signals()
{
  bool success = false;
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = SA_NOCLDSTOP;

  debug_log0("Configuring signals");

  do {
    sa.sa_handler = sigchld_handler;
    if (sigaction(SIGCHLD, &sa, NULL) != 0) break;

    sa.sa_handler = sigterm_handler;
    if (sigaction(SIGTERM, &sa, NULL) != 0) break;
    if (sigaction(SIGINT,  &sa, NULL) != 0) break;

    sa.sa_handler = sighup_handler;
    if (sigaction(SIGHUP, &sa, NULL) != 0) break;

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) != 0) break;
    if (sigaction(SIGTTIN, &sa, NULL) != 0) break;
    if (sigaction(SIGTTOU, &sa, NULL) != 0) break;
    if (sigaction(SIGTSTP, &sa, NULL) != 0) break;
    // SIGQUIT is handled by children
    if (sigaction(SIGQUIT, &sa, NULL) != 0) break;

    sa.sa_flags |= SA_RESETHAND;
    sa.sa_handler = crash_handler;
    if (sigaction(SIGSEGV, &sa, NULL) != 0) break;
    if (sigaction(SIGBUS,  &sa, NULL) != 0) break;
    if (sigaction(SIGABRT, &sa, NULL) != 0) break;
    if (sigaction(SIGILL,  &sa, NULL) != 0) break;
    if (sigaction(SIGFPE,  &sa, NULL) != 0) break;

    success = true;
  } while (0);

  if (!success) {
    error_log("sigaction: %s", strerror(errno));
  }

  return (success);
}


/*! Starts per-application servers in separate processes.
 * \param config_list Configuration instances for the server applications (clients).
 */
static void
run(AppConfigPtrList& config_list)
{
  pid_t child_pid;
  sigset_t sigmask;

  if (config_list.empty()) {
    error_log("Configuration file is empty\n");
    exit(EXIT_FAILURE);
  }

  assert(g_children.empty());
  assert(!g_server);

  if (sigprocmask(SIG_SETMASK, &g_org_sigmask, &sigmask) != 0) {
    error_log("sigprocmask: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  mode_t saved_umask = umask(g_saved_umask);

  for (auto& config : config_list) {
    child_pid = fork();

    if (child_pid == -1) {
      error_log("fork: %s", strerror(errno));
      exit(EXIT_FAILURE);
    }

    if (child_pid == 0) { // child process
      g_got_sigterm = 0;
      g_got_sighup = 0;
      g_got_sigchld = 0;

      //g_server = imtools::imserver::ServerPtr(new Server(config));
      debug_log("replacing g_server, use count = %ld", g_server.use_count());
      g_server.reset(new Server(config));
      g_server->run();

      exit(EXIT_SUCCESS);
    }

    verbose_log("Forked worker, PID: %ld", static_cast<long>(child_pid));
    g_children.insert(child_pid);
  }

  umask(saved_umask);

  // The following goes within parent process

  // Restore signal mask
  if (sigprocmask(SIG_SETMASK, &sigmask, NULL) != 0) {
    error_log("sigprocmask failed to restore signal mask: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (configure_signals()) {
    process_signals();
  }
  wait_for_children();
}


Server::CommandType
Server::getCommandType(const char* name) noexcept
{
  Server::CommandType type;

  switch (name[0]) {
    case 's':
      if (strcmp(name, "stop") == 0) {
        type = Server::CommandType::STOP;
      } else {
        type = Server::CommandType::UNKNOWN;
      }
      break;
    case 'r':
      if (strcmp(name, "restart") == 0) {
        type = Server::CommandType::RESTART;
      } else {
        type = Server::CommandType::UNKNOWN;
      }
      break;
    default:
      type = Server::CommandType::UNKNOWN;
      break;
  }

  return type;
}


inline bool
Server::checkCommandDigest(const imtools::Command& command, const std::string& digest) const noexcept
{
#ifdef IMTOOLS_DEBUG
  std::string true_digest(Util::makeSHA1(getAppName() + command.serialize() + getPrivateKey()));
  debug_log("input digest: %s true digest: %s (%s + %s + %s)",
      digest.c_str(), true_digest.c_str(),
      getAppName().c_str(), command.serialize().c_str(), getPrivateKey().c_str());
  return (digest == true_digest);
#else
  return (digest == Util::makeSHA1(getAppName() + command.serialize() + getPrivateKey()));
#endif
}


static void
handle_command(Server::CommandType command_type)
{
  int fd;
  pid_t pid;
  pid_t gid;
  const char* pid_filename = Config::getPidFilePath();

  if (!(fd = open(pid_filename, O_RDONLY))) {
    error_log("open(%s): %s", pid_filename, strerror(errno));
    exit(EXIT_FAILURE);
  }

  char buf[MAX_LINE_LENGTH];
  ssize_t n_read = read(fd, buf, sizeof(buf) - 1);
  if (n_read <= 0) {
    error_log("failed to read pid from pid file %s: %s", pid_filename, strerror(errno));
    close(fd);
    exit(EXIT_FAILURE);
  }
  close(fd);
  buf[n_read] = '\0';

  debug_log("pid read from file: %s", buf);
  if ((pid = static_cast<pid_t>(std::stol(std::string(buf)))) <= 0) {
    error_log("invalid pid read from pid file %s", pid_filename);
    exit(EXIT_FAILURE);
  }

  if ((gid = getpgid(pid)) == -1) {
    error_log("getpgid(%ld): %s", static_cast<long>(pid), strerror(errno));
    exit(EXIT_FAILURE);
  }

  int sig;
  switch (command_type) {
    case Server::CommandType::STOP:    sig = SIGTERM; break;
    case Server::CommandType::RESTART: sig = SIGHUP;  break;
    default:
      error_log("unknown command\n");
      usage(true);
      exit(EXIT_FAILURE);
  }

  debug_log("Sending signal '%d' to all processes in group %ld",
      sig, static_cast<long>(gid));
  kill(-gid, sig);

  exit(EXIT_SUCCESS);
}


int
main(int argc, char **argv)
{
  int next_option;
  int exit_code   = 0;

  imtools::imserver::g_program_name = argv[0];

#ifdef IMTOOLS_DEBUG
  setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering
#endif

  debug_log0("Parsing CLI args");
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
          g_config_file = optarg;
          break;

        case 'd':
          g_daemonize = true;
          break;

        case -1:
          // done with options
          break;

        case '?':
          // unrecognized option
          usage(true);
          exit(2);

        default:
          error_log("getopt returned character code 0%o", next_option);
          usage(true);
          exit(2);
      }
    } while (next_option != -1);
  } catch (imtools::InvalidCliArgException& e) {
    error_log("%s", e.what());
    exit(2);
  }

  if (g_config_file.empty()) {
    g_config_file = "server.cfg";
  }
  char config_file_real_path[PATH_MAX];
  if (realpath(g_config_file.c_str(), config_file_real_path) == NULL) {
    error_log("realpath(%s): %s", g_config_file.c_str(), strerror(errno));
    exit(EXIT_FAILURE);
  }
  g_config_file = config_file_real_path;
  debug_log("Configuration file: '%s'", g_config_file.c_str());

  try {
    // Save current signal mask
    if (sigprocmask(SIG_SETMASK, NULL, &g_org_sigmask) != 0) {
      error_log("sigprocmask: %s", strerror(errno));
      exit(EXIT_FAILURE);
    }

    // Config should be parsed before command handling
    auto config_list = Config::parse(g_config_file);

    // Handle command
    if (optind < argc) {
      const char* command = argv[optind++];
      if (command) {
        handle_command(Server::getCommandType(command));
      }
    }

    if (g_daemonize || Config::getDaemonizeFlag()) {
      daemonize();
    }

    imtools::log::set_level(Config::getLogLevel());

    do {
      if (g_got_sighup) {
        g_got_sighup = 0;
        reconfigure(config_list);
      }

      run(config_list);
    } while(g_got_sighup);
  } catch (ErrorException& e) {
    error_log("%s", e.what());
    exit_code = 1;
  } catch (boost::system::system_error& e) {
    error_log("%s", e.what());
    exit_code = 1;
  } catch (std::exception& e) {
    error_log("!!! Unhandled error: %s. Please file a bug.", e.what());
    exit_code = 1;
  }

  return exit_code;
}

// vim: et ts=2 sts=2 sw=2
