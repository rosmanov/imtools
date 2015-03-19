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

#include <getopt.h>
#include <string>
#include <memory>
#include <set>

#include <boost/property_tree/ptree.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "imresize-api.hxx"
#include "imtools-meta.hxx"


namespace imtools { namespace imserver {
/////////////////////////////////////////////////////////////////////

class Server;
class Config;

typedef std::shared_ptr<Server> ServerPtr;
typedef std::shared_ptr<Config> ConfigPtr;
typedef std::vector<ConfigPtr> ConfigPtrList;
typedef std::unique_ptr<imtools::Command> CommandPtr;
typedef websocketpp::connection_hdl Connection;
typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;

/////////////////////////////////////////////////////////////////////

const char* g_program_name;
/// Pointer to the currently running WebSocket server endpoint.
/// (Since we fork(), we need only one variable.)
ServerPtr g_server;

/// Template for `printf`-like function.
const char* g_usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"WebSocket server for ImTools.\n"
"Usage: %1$s OPTIONS\n\n"
"OPTIONS:\n"
" -h, --help       Display this help.\n"
" -V, --version    Print version\n"
" -v, --verbose    Turn on verbose output. Can be used multiple times\n"
"                  to increase verbosity (e.g. -vv). Default: off.\n"
" -c, --config     Path to configuration file. Default: server.cfg.\n"
"\nEXAMPLE:\n\n"
"%1$s -c server.cfg -v\n\n";

// CLI arguments.
const char *g_short_options = "hVvc:";
const struct option g_long_options[] = {
  {"help",    no_argument,       NULL, 'h'},
  {"version", no_argument,       NULL, 'V'},
  {"verbose", no_argument,       NULL, 'v'},
  {"config",  required_argument, NULL, 'c'},
  {0,         0,                 0,    0}
};


/////////////////////////////////////////////////////////////////////
/// Misc. utilities
class Util
{
  public:
    Util() = delete;

    static const char* getErrorMessage(const websocketpp::lib::error_code& ec) noexcept;
    /// Unary operation for std::transform(). Converts property tree value to Command::element_t.
    static Command::element_t convertPtreeValue(const boost::property_tree::ptree::value_type& v) noexcept;
    /// \returns SHA-1 digest for `source` in hexadecimal format
    static std::string makeSHA1(const std::string& source) noexcept;
};


/////////////////////////////////////////////////////////////////////
/// Server configuration for an application
class Config : public std::enable_shared_from_this<Config>
{
  public:
    /// Numeric representation of configuration option names.
    enum class Option
    {
      /// Unhandled option type
      UNKNOWN,
      /// `port`
      PORT,
      /// `host`
      HOST,
      /// `chdir`
      CHDIR,
      /// `allow_absolute_paths`
      ALLOW_ABSOLUTE_PATHS,
      /// `key`
      PRIVATE_KEY
    };

    Config() = delete;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    virtual ~Config() {}
    explicit Config(const std::string& app_name, uint16_t port, const std::string& host,
        const std::string& chdir_dir, bool allow_absolute_paths, const std::string& key);

    /// Parses a configuration file
    /// \param filename Path to configuration file
    /// \returns a set of configuration instances.
    static ConfigPtrList parse(const std::string& filename);

    inline const std::string& getAppName() const noexcept { return m_app_name; }
    inline uint16_t getPort() const noexcept { return m_port; }
    inline const std::string& getHost() const noexcept { return m_host; }
    inline const std::string& getChdirDir() const noexcept { return m_chdir; }
    inline bool getAllowAbsolutePaths() const noexcept { return m_allow_absolute_paths; }
    inline const std::string& getPrivateKey() const noexcept { return m_private_key; }

  protected:
    /// \param k Option name
    /// \returns Numeric representation of option name
    static Option getOption(const std::string& k);

    /// Application name.
    std::string m_app_name;
    /// Value of 'port' option.
    uint16_t m_port;
    /// Value of 'host' option.
    std::string m_host;
    /// Value of 'chdir' option.
    std::string m_chdir;
    /// Value of 'allow_absolute_paths' option.
    bool m_allow_absolute_paths;
    /// Value of 'key' option.
    std::string m_private_key;
};


/////////////////////////////////////////////////////////////////////
/// WebSocket server for an application (section in the config file)
class Server : public std::enable_shared_from_this<Server>
{
  public:
    enum class MessageType
    {
      ERROR,
      SUCCESS
    };

    virtual ~Server() {}
    explicit Server(const ConfigPtr& config) noexcept : m_config(config) {}

    Server() = delete;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// Starts accepting connections
    virtual void run();
    /// Stops accepting connections, closes all active connections
    virtual void stop();

    /// Callback which is invoked when a new connection is opened.
    virtual void onOpen(Connection conn) noexcept;
    /// Callback which is invoked when a connection is closed.
    virtual void onClose(Connection conn) noexcept;
    /// Callback which is invoked when connection receives a request from the client.
    virtual void onMessage(Connection conn, WebSocketServer::message_ptr msg) noexcept;

    /// Returns port on which the server accepts WebSocket requests.
    inline uint16_t getPort() const noexcept { return m_config->getPort(); }
    /// Returns hostname on which the server accepts WebSocket requests. Empty means 'all hosts'
    inline const std::string& getHost() const noexcept { return m_config->getHost(); }
    /// Returns value of 'chdir' configuration option.
    inline const std::string& getChdirDir() const noexcept { return m_config->getChdirDir(); }
    /// Returns value of 'allow_absolute_paths' configuration option.
    inline bool getAllowAbsolutePaths() const noexcept { return m_config->getAllowAbsolutePaths(); }
    /// \returns Application name specified in the configuration file.
    inline const std::string& getAppName() const noexcept { return m_config->getAppName(); }
    /// \returns application private key specified in the configuration file
    inline const std::string& getPrivateKey() const noexcept { return m_config->getPrivateKey(); }

  protected:
    typedef std::set<Connection, std::owner_less<Connection>> ConnectionList;

    /// Sends response message to the client.
    virtual void sendMessage(Connection conn, const std::string& message, MessageType type) noexcept;

    virtual inline bool checkCommandDigest(const imtools::Command& command, const std::string& digest) const
    {
      return (digest == Util::makeSHA1(getAppName() + command.serialize() + getPrivateKey()));
    }

    /// Configuration of an application
    ConfigPtr m_config;
    /// WebSocket endpoint
    WebSocketServer m_server;
    /// Container for accepted connections.
    ConnectionList m_connections;
};

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::imserver
#endif // IMTOOLS_IMSERVER_HXX
// vim: et ts=2 sts=2 sw=2
