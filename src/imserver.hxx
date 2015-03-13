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
#ifndef IMTOOLS_IMSERVER_HXX
#define IMTOOLS_IMSERVER_HXX

#include <sys/wait.h>
#include <signal.h> // for sigaction()
#include <set>
#include <memory> // for std::unique_ptr(), std::weak_ptr()
#include <iterator> // for std::back_inserter()
#include <algorithm> // for std::transform()

// Boost parsers
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

// WebSocket library
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "imtools.hxx"
#include "imresize-api.hxx"


namespace imtools { namespace imserver {
using namespace imtools;
using boost::property_tree::ptree;
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::ref;
using websocketpp::lib::error_code;

typedef websocketpp::server<websocketpp::config::asio> websocket_server_type;

const char* g_program_name;

/// Template for `printf`-like function.
const char* g_usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"WebSocket server for ImTools.\n"
"Usage: %1$s OPTIONS\n\n"
"OPTIONS:\n"
" -h, --help       Display this help.\n"
" -V, --version    Print version\n"
" -v, --verbose    Turn on verbose output. Can be used multiple times\n"
"                  to increase verbosity (e.g. -vv). Default: off.\n"
" -c, --config     Path to configuration file. Optional.\n"
"\nEXAMPLE:\n\n"
"TODO\n\n";

// CLI arguments.
const char *g_short_options = "hVvc:";
const struct option g_long_options[] = {
  {"help",    no_argument,       NULL, 'h'},
  {"version", no_argument,       NULL, 'V'},
  {"verbose", no_argument,       NULL, 'v'},
  {"config",  required_argument, NULL, 'c'},
  {0,         0,                 0,    0}
};


class Server : public std::enable_shared_from_this<Server>
{
  private:
    // We are keeping instances of current class in s_instances member as a set
    // of weak pointers. Weak pointers cannot be created within constructor,
    // because they would expire at the end of constructor immediately.
    //
    // So we expose a factory method and make the constructor private.
    explicit Server(uint16_t port, const std::string& host);

  public:
    typedef std::set<connection_hdl, std::owner_less<connection_hdl>> connection_list_type;
    typedef std::weak_ptr<Server> instance_ptr_type;
    typedef std::set<instance_ptr_type, std::owner_less<instance_ptr_type>> instance_list_type;
    typedef std::shared_ptr<Server> ptr_type;
    typedef std::vector<ptr_type> ptr_list_type;

    enum class Option
    {
      UNKNOWN,
      PORT,
      HOST
    };

    virtual ~Server();
    Server() = delete;

    // Disable copying
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    static ptr_type create(uint16_t port, const std::string& host);
    static void signalHandler(int signal, siginfo_t* siginfo, void*);
    static const char* getErrorMessage(const error_code& ec);

    virtual void run();
    virtual void stop();
    virtual void onOpen(connection_hdl hdl) const;
    virtual void onClose(connection_hdl hdl) const;
    virtual void onMessage(connection_hdl hdl, websocket_server_type::message_ptr msg);

    static void parseConfig(ptr_list_type& server_list, const std::string& filename);
    static void stopAll();

    inline uint16_t getPort() const { return m_port; }
    inline const std::string& getHost() const { return m_host; }

  protected:
    static Command::element_t convertPtreeValue(const ptree::value_type& v);
    static Option getOption(const std::string& k);

    uint16_t m_port;
    std::string m_host;
    websocket_server_type m_server;

    static connection_list_type s_connections;
    static instance_list_type s_instances;
};

}} // namespace imtools::imserver

#endif // IMTOOLS_IMSERVER_HXX
// vim: et ts=2 sts=2 sw=2
