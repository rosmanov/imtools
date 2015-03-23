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
#include <string.h>
#include "log.hxx"

namespace imtools { namespace log {
/////////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 1024

typedef std::vector<std::string> ErrorStack;
static ErrorStack g_error_stack;

#ifdef IMTOOLS_DEBUG
static Level g_level{Level::DEBUG};
#else
static Level g_level{Level::NOTICE};
#endif

static const char* g_level_names[] = {
  [Level::ERROR]   = "Error",
  [Level::WARNING] = "Warning",
  [Level::NOTICE]  = "Notice",
  [Level::INFO]    = "Info",
  [Level::DEBUG]   = "Debug",
  [Level::NONE]    = "",
};

/////////////////////////////////////////////////////////////////////

void
push_error(const std::string& msg) noexcept
{
  g_error_stack.push_back(msg);
}


void
push_error(const char* format, ...) noexcept
{
  if (format) {
    va_list args;
    va_start(args, format);
    char message[1024];
    int message_len = vsnprintf(message, sizeof(message), format, args);
    g_error_stack.push_back(std::string(message, message_len));
    va_end(args);
  }
}


void
warn_all() noexcept
{
  if (!g_error_stack.empty()) {
    for (ErrorStack::iterator it = g_error_stack.begin(); it != g_error_stack.end(); ++it) {
      warning_log("%s", (*it).c_str());
    }
    g_error_stack.clear();
  }
}


void
write(Level level, const char* format, ...) noexcept
{
  if (level > g_level) {
    return;
  }

  if (format) {
    char buf[MAX_LINE_LENGTH];
    const size_t buf_size{MAX_LINE_LENGTH};
    size_t len{0};

    IT_IO_SCOPED_LOCK(__scoped_lock);

    len += snprintf(buf + len, buf_size - len, "[%s] ", g_level_names[level]);
    if (level >= Level::DEBUG) {
      len += snprintf(buf + len, buf_size - len, "[%ld:%ld] ",
        static_cast<long>(getpid()), static_cast<long>(IMTOOLS_THREAD_ID));
    }

    if (len < buf_size) {
      va_list args;
      va_start(args, format);
      len += vsnprintf(buf + len, buf_size - len, format, args);
      va_end(args);
    }

    if (len >= buf_size) { // truncated
      memcpy(buf + buf_size - sizeof("..."), "...", sizeof("...") - 1);
      len = buf_size - 1;
    }

    buf[len++] = '\n';

    ::write(level < Level::WARNING ? STDOUT_FILENO : STDERR_FILENO, buf, len);
  }
}


void
set_level(Level level) noexcept
{
  g_level = level;
}

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::log
// vim: et ts=2 sts=2 sw=2
