/* \file
 *
 * \copyright Copyright © 2014,2015  Ruslan Osmanov <rrosmanov@gmail.com>
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
#include <cstdarg>
#include <sstream> // istringstream
#include <string>
#include "exceptions.hxx"
#include "imtools-types.hxx"

namespace imtools {
/////////////////////////////////////////////////////////////////////

class ErrorException;
class InvalidCliArgException;

/////////////////////////////////////////////////////////////////////

template <class T> T
get_opt_arg(const std::string& optarg, const char* format, ...)
{
  T result;
  std::istringstream is(optarg);

  if (is >> result) {
    return result;
  }

  if (!format) {
    throw ErrorException("Invalid error format in %s", __func__ );
  }

  std::string error;
  va_list args;
  va_start(args, format);
  char message[1024];
  const int message_len = vsnprintf(message, sizeof(message), format, args);
  error = std::string(message, message_len);
  va_end(args);
  throw InvalidCliArgException(error);
}

template int get_opt_arg(const std::string& optarg, const char* format, ...);
template uint_t get_opt_arg(const std::string& optarg, const char* format, ...);
template double get_opt_arg(const std::string& optarg, const char* format, ...);

/////////////////////////////////////////////////////////////////////
} //namespace imtools
// vim: et ts=2 sts=2 sw=2
