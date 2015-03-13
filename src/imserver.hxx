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
typedef websocketpp::connection_hdl Connection;
typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;

/////////////////////////////////////////////////////////////////////

const char* g_program_name;
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
/// Server configuration for an application
class Config : public std::enable_shared_from_this<Config>
{
  public:
    enum class Option
    {
      UNKNOWN,
      PORT,
      HOST
    };

    Config() = delete;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    virtual ~Config() {}
    explicit Config(uint16_t port, const std::string& host)
      : m_port(port), m_host(host) {}

    static ConfigPtrList parse(const std::string& filename);

    inline uint16_t getPort() const { return m_port; }
    inline const std::string& getHost() const { return m_host; }

  protected:
    static Option getOption(const std::string& k);

    uint16_t m_port;
    std::string m_host;
};


/////////////////////////////////////////////////////////////////////
/// Application server
class Server : public std::enable_shared_from_this<Server>
{
  public:
    virtual ~Server() {}
    explicit Server(const ConfigPtr& config) noexcept : m_config(config) {}

    Server() = delete;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    virtual void run();
    virtual void stop();

    virtual void onOpen(Connection conn) noexcept;
    virtual void onClose(Connection conn) noexcept;
    virtual void onMessage(Connection conn, WebSocketServer::message_ptr msg) noexcept;

    inline uint16_t getPort() const noexcept { return m_config->getPort(); }
    inline const std::string& getHost() const noexcept { return m_config->getHost(); }

  protected:
    typedef std::set<Connection, std::owner_less<Connection>> ConnectionList;

    static const char* getErrorMessage(const websocketpp::lib::error_code& ec) noexcept;
    static Command::element_t convertPtreeValue(const boost::property_tree::ptree::value_type& v) noexcept;

    ConfigPtr m_config;
    WebSocketServer m_server;
    ConnectionList m_connections;
};

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::imserver

#endif // IMTOOLS_IMSERVER_HXX
// vim: et ts=2 sts=2 sw=2
