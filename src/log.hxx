/* Copyright © 2014,2015  Ruslan Osmanov <rrosmanov@gmail.com>
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
#ifndef IMTOOLS_LOG_HXX
#define IMTOOLS_LOG_HXX

#include <sys/types.h>
#include <stdio.h>
#include <cstdarg>
#include <string>
#include <vector>
#include <unistd.h>

#include "imtools-types.hxx"
#include "threads.hxx"

/////////////////////////////////////////////////////////////////////
// Fwd decl

namespace imtools {
extern uint_t verbose;
}

namespace imtools { namespace log {
/////////////////////////////////////////////////////////////////////

#ifdef IMTOOLS_THREADS
# define IMTOOLS_THREAD_ID imtools::threads::get_id()
#else
# define IMTOOLS_THREAD_ID 0
#endif

#define verbose_log(...)                                           \
  do {                                                             \
    if (imtools::verbose) {                                        \
      imtools::log::write(imtools::log::Level::INFO, __VA_ARGS__); \
    }                                                              \
  } while (0)
#define verbose_log2(...)                                          \
  do {                                                             \
    if (imtools::verbose > 1) {                                    \
      imtools::log::write(imtools::log::Level::INFO, __VA_ARGS__); \
    }                                                              \
  } while (0)

#ifdef IMTOOLS_DEBUG
# define debug_log0(__str) imtools::log::write(imtools::log::Level::DEBUG, (__str))
# define debug_log(__fmt, ...) imtools::log::write(imtools::log::Level::DEBUG, (__fmt), __VA_ARGS__)
#else
# define debug_log0(__str)
# define debug_log(__fmt, ...)
#endif// IMTOOLS_DEBUG
#define error_log(...)   imtools::log::write(imtools::log::Level::ERROR,   __VA_ARGS__)
#define warning_log(...) imtools::log::write(imtools::log::Level::WARNING, __VA_ARGS__)
#define notice_log(...)  imtools::log::write(imtools::log::Level::NOTICE,  __VA_ARGS__)

#define strict_log(is_strict, ...)                \
  do {                                            \
    if ((is_strict)) {                            \
      throw imtools::ErrorException(__VA_ARGS__); \
    } else {                                      \
      warning_log(__VA_ARGS__);                   \
    }                                             \
  } while(0)

#ifdef IMTOOLS_DEBUG_PROFILER
# define timespec_to_float(__t) ((double)((__t).tv_sec + (__t).tv_nsec * 1e-9))
# define debug_timer_init(__t1, __t2) struct timespec __t1, __t2
# define debug_timer_start(__t) clock_gettime(CLOCK_REALTIME, &(__t))
# define debug_timer_end(__t1, __t2, __name)                           \
  do {                                                                 \
    IT_IO_SCOPED_LOCK(__scoped_lock);                                  \
    clock_gettime(CLOCK_REALTIME, &(__t2));                            \
    printf("[%s] Timer: " # __name ": "                                \
        (timespec_to_float(__t2) - timespec_to_float(__t1)) " sec\n"); \
  } while(0)
#else
# define debug_timer_init(__t1, __t2)
# define timespec_to_float(__t)
# define debug_timer_start(__t)
# define debug_timer_end(__t1, __t2, __name)
#endif


/////////////////////////////////////////////////////////////////////
// Types

enum Level : int
{
  ERROR,
  WARNING,
  NOTICE,
  INFO,
  DEBUG,
  NONE,
};

/////////////////////////////////////////////////////////////////////

void push_error(const std::string& msg) noexcept;
void push_error(const char* msg, ...) noexcept;
void warn_all() noexcept;
void write(Level level, const char* format, ...) noexcept;
void set_level(Level level) noexcept;

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::log
#endif // IMTOOLS_LOG_HXX
// vim: et ts=2 sts=2 sw=2
