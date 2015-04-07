/* Copyright © 2014,2015 - Ruslan Osmanov <rrosmanov@gmail.com>
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
#pragma once
#ifndef IMTOOLS_IMSERVER_HXX
#define IMTOOLS_IMSERVER_HXX

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>
#include <unistd.h>

#include <string>
#include <memory>
#include <set>
#include <atomic>

#include <boost/property_tree/ptree.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "MetaCommand.hxx"
#include "imresize-api.hxx"
#include "immerge-api.hxx"
#include "imtools-meta.hxx"
#include "log.hxx"


namespace imtools { namespace imserver {
/////////////////////////////////////////////////////////////////////

class Server;
class Config;
class AppConfig;

typedef std::shared_ptr<Server> ServerPtr;
typedef std::shared_ptr<AppConfig> AppConfigPtr;
typedef std::shared_ptr<Config> ConfigPtr;
typedef std::vector<AppConfigPtr> AppConfigPtrList;
typedef std::unique_ptr<imtools::Command> CommandPtr;
typedef websocketpp::connection_hdl Connection;
typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;

/////////////////////////////////////////////////////////////////////

const char* g_program_name;
/// Pointer to the currently running WebSocket server endpoint.
/// (Since we fork(), we need only one variable.)
ServerPtr g_server;

/// Whether to run the server in background as a daemon.
bool g_daemonize{false};

/// Path to configuration file
std::string g_config_file{"server.cfg"};

/// Template for `printf`-like function.
const char* g_usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"WebSocket server for ImTools.\n"
"Usage: %1$s OPTIONS [command]\n\n"
"OPTIONS:\n"
" -h, --help       Display this help.\n"
" -V, --version    Print version\n"
" -v, --verbose    Turn on verbose output. Can be used multiple times\n"
"                  to increase verbosity (e.g. -vv). Default: off.\n"
" -c, --config     Path to configuration file. Default: server.cfg.\n"
" -d, --daemonize  Run in background as a daemon (overrides settings specified\n"
"                  in the configuration file). Default: off.\n"
"\nEXAMPLE:\n\n"
"%1$s -c server.cfg -v\n\n";

// CLI arguments.
const char *g_short_options = "hVvc:d";
const struct option g_long_options[] = {
  {"help",      no_argument,       NULL, 'h'},
  {"version",   no_argument,       NULL, 'V'},
  {"verbose",   no_argument,       NULL, 'v'},
  {"config",    required_argument, NULL, 'c'},
  {"daemonize", no_argument,       NULL, 'd'},
  {0,           0,                 0,    0}
};


/////////////////////////////////////////////////////////////////////
/// Misc. utilities
class Util
{
  public:
    Util() = delete;

    /*! \returns string representation of the boost/websocketpp error */
    static const char* getErrorMessage(const websocketpp::lib::error_code& ec) noexcept;

    /*! Unary operation for std::transform(). Converts property tree value to
     * Command::ArgumentItem */
    static Command::ArgumentItem convertPtreeValue(const boost::property_tree::ptree::value_type& v) noexcept;

    /*! \returns SHA-1 digest for `source` message in hexadecimal format */
    static std::string makeSHA1(const std::string& source) noexcept;

    /*! Retrieves user information from `/etc/passwd`
     * \param name user name
     * \param pwd output container
     * \returns true on succes, otherwise false
     */

    static bool getUserData(const char* name, struct ::passwd& pwd) noexcept;
    /*! Retrieves user group information from `/etc/group`
     * \param name user group name
     * \param grp output container
     * \returns true on succes, otherwise false
     */
    static bool getUserGroupData(const char* name, struct ::group& grp) noexcept;

};


/////////////////////////////////////////////////////////////////////
// Application pool options
class AppConfig
{
  public:
    /// Numeric representations of option names.
    enum class Option : int
    {
      UNKNOWN,
      PORT,
      HOST,
      CHDIR,
      ALLOW_ABSOLUTE_PATHS,
      PRIVATE_KEY,
      CHROOT,
      USER,
      GROUP,
      ERROR_LOG_FILE
    };

  public:
    virtual ~AppConfig() {}
    AppConfig() = delete;
    explicit AppConfig(const std::string& app_name) :
      m_app_name(app_name)
    {}

    inline operator bool() const noexcept {
      return (m_port && !m_key.empty() && !m_error_log.empty());
    }

    /*! Sets an option value
     * \param k option name (key)
     * \param v option value
     */
    void set(const std::string& k, const std::string& v) noexcept;

  public:
    // Accessors
    inline const std::string& getAppName() const noexcept { return m_app_name; }
    inline uint16_t getPort() const noexcept { return m_port; }
    inline const std::string& getHost() const noexcept { return m_host; }
    inline const std::string& getChdirDir() const noexcept { return m_chdir; }
    inline bool getAllowAbsolutePaths() const noexcept { return m_allow_absolute_paths; }
    inline const std::string& getPrivateKey() const noexcept { return m_key; }
    inline const std::string& getChrootDir() const noexcept { return m_chroot; }
    inline const std::string& getUser() const noexcept { return m_user; }
    inline const std::string& getGroup() const noexcept { return m_group; }
    inline const std::string& getErrorLogFile() const noexcept { return m_error_log; }

  protected:
    /*! \param k Option name
     * \returns Numeric representation of option name */
    static Option getOption(const std::string& k);

  protected:
    /// Application name (matches section name in configuration file)
    std::string m_app_name;

  protected:
    /// Whether absolute paths are passed to commands "as is"
    bool m_allow_absolute_paths = true;
    /// Network port to listen on
    uint16_t m_port = 9902;
    /// Server hostname or IP to listen on
    std::string m_host;
    /// Directory to `chdir` to
    std::string m_chdir;
    /// Private key for message digest
    std::string m_key;
    /// Directory to `chroot` to
    std::string m_chroot;
    /// Linux user name
    std::string m_user;
    /// Linux user group
    std::string m_group;
    /// Path to error log file
    std::string m_error_log;

};


/////////////////////////////////////////////////////////////////////
/// Server global configuration
class Config
{
  Config() = delete;

  public:
    /// Numeric representations of global options.
    enum class Option : int
    {
      UNKNOWN,
      DAEMONIZE,
      ERROR_LOG_FILE,
      PID_FILE,
      LOG_LEVEL
    };

    typedef imtools::log::Level LogLevel;

  public:
    /*! Parses a configuration file
     * \param filename Path to configuration file
     * \returns a set of app configuration instances */
    static AppConfigPtrList parse(const std::string& filename);

  public:
    // Accessors
    inline static bool getDaemonizeFlag() noexcept { return s_daemonize; }
    inline static std::string& getErrorLogFile() noexcept { return s_error_log_file; }
    inline static const char* getPidFilePath() noexcept { return s_pid_file.c_str(); }
    inline static LogLevel getLogLevel() noexcept { return s_log_level; }
    /*! \returns numeric representation of log level name. */
    static LogLevel getLogLevel(const std::string& name) noexcept;

  protected:
    /*! \param k Global option name (option within [global] section).
     * \returns Numeric representation of [global] option name */
    static Option getOption(const std::string& k);

    /*! Assigns value for a global option
     * \param k global option name (key)
     * \param v global option value
     */
    static void set(const std::string& k, const std::string& v) noexcept;

  protected:
    /// Whether to daemonize the calling process
    static bool s_daemonize;
    /// Path to PID file
    static std::string s_pid_file;
    /// Path to error log file
    static std::string s_error_log_file;
    /// Log verbosity
    static LogLevel s_log_level;
};


/////////////////////////////////////////////////////////////////////
/// WebSocket server for an application (section in the config file)
class Server
{
  public:
    enum class MessageType
    {
      ERROR,
      SUCCESS
    };

    enum class CommandType : int
    {
      UNKNOWN,
      STOP,
      RESTART
    };

  public:
    virtual ~Server() {}
    explicit Server(const AppConfigPtr& config) noexcept;

    Server() = delete;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

  public:
    /// Starts accepting connections
    void run();
    /// Stops accepting connections, closes all active connections
    void stop();

  public:
    /// Callback which is invoked when a new connection is opened.
    void onOpen(Connection conn) noexcept;
    /// Callback which is invoked when a connection is closed.
    void onClose(Connection conn) noexcept;
    /// Callback which is invoked when connection receives a request from the client.
    void onMessage(Connection conn, WebSocketServer::message_ptr msg) noexcept;
    // Callback which is invoked once for every unsuccessful WebSocket connection attempt.
    void onFail(Connection conn) noexcept;
    /// SIGTERM / SIGINT signal handler
    void sigtermHandler(boost::system::error_code ec, int signal_number) noexcept;
    /// SIGSEGV, SIGBUS, SIGABRT, SIGILL and SIGFPE signals handler
    void crashHandler(boost::system::error_code ec, int signal_number) noexcept;

  public:
    inline uint16_t getPort() const noexcept { return m_config->getPort(); }
    inline const std::string& getHost() const noexcept { return m_config->getHost(); }
    inline const std::string& getChdirDir() const noexcept { return m_config->getChdirDir(); }
    inline bool getAllowAbsolutePaths() const noexcept { return m_config->getAllowAbsolutePaths(); }
    inline const std::string& getAppName() const noexcept { return m_config->getAppName(); }
    inline const std::string& getPrivateKey() const noexcept { return m_config->getPrivateKey(); }
    inline const std::string& getChrootDir() const noexcept { return m_config->getChrootDir(); }
    inline const std::string& getUser() const noexcept { return m_config->getUser(); }
    inline const std::string& getGroup() const noexcept { return m_config->getGroup(); }
    inline const std::string& getErrorLogFile() const noexcept { return m_config->getErrorLogFile(); }

    /// \returns numeric representation of the server command name
    static CommandType getCommandType(const char* name) noexcept;

  protected:
    /// Sends response message to the client.
    void sendMessage(Connection conn, const std::string& message, const std::string& digest, MessageType type) noexcept;
    /// \returns whether `digest` corresponds to the `command`
    bool checkCommandDigest(const imtools::Command& command, const std::string& digest) const noexcept;
    /// Configures UID/GID for the worker process
    bool setupUser() noexcept;
    /// Opens error log file for the worker process, closes parent FDs
    bool openErorLog() noexcept;

  protected:
    typedef std::set<Connection, std::owner_less<Connection>> ConnectionList;

    /// Application pool settings
    AppConfigPtr m_config;
    /// WebSocket endpoint
    WebSocketServer m_server;
    /// Container for accepted connections.
    ConnectionList m_connections;
    /// Linux user ID
    uid_t m_uid{0};
    /// Linux group ID
    gid_t m_gid{0};
};

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::imserver
#endif // IMTOOLS_IMSERVER_HXX
// vim: et ts=2 sts=2 sw=2
