/* Copyright (C) 2014  Ruslan Osmanov <rrosmanov@gmail.com>
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

#ifndef IMTOOLS_LOG_HXX
#define IMTOOLS_LOG_HXX

#include "imtools.hxx"

using std::vector;
using std::string;

namespace imtools
{

namespace log
{

#ifdef IMTOOLS_DEBUG
# define debug_log0(__str) printf("[Debug] [%lu] " __str, pthread_self())
# define debug_log(__fmt, ...) printf("[Debug] [%lu] " __fmt, pthread_self(), __VA_ARGS__)
#else
# define debug_log0(__str)
# define debug_log(__fmt, ...)
#endif

#define verbose_log(...)                       \
  do {                                         \
    if (imtools::verbose) printf(__VA_ARGS__); \
  } while (0)
#define verbose_log2(...)                               \
  do {                                                  \
    if (imtools::verbose > 1) printf("v2: " __VA_ARGS__); \
  } while (0)

#define error_log(...) fprintf(stderr, "[Error] " __VA_ARGS__)
#define warning_log(...) fprintf(stderr, "[Warning] " __VA_ARGS__)

#define strict_log(is_strict, ...) \
  do {                             \
    if ((is_strict)) {             \
      error_log(__VA_ARGS__);      \
      throw ErrorException();      \
    } else {                       \
      warning_log(__VA_ARGS__);    \
    }                              \
  } while(0)

#ifdef IMTOOLS_DEBUG
# define timespec_to_float(__t) ((double)((__t).tv_sec + (__t).tv_nsec * 1e-9))
# define debug_timer_init(__t1, __t2) struct timespec __t1, __t2
# define debug_timer_start(__t) clock_gettime(CLOCK_REALTIME, &(__t))
# define debug_timer_end(__t1, __t2, __name)                                \
  do {                                                                      \
    clock_gettime(CLOCK_REALTIME, &(__t2));                                 \
    printf("[%ld] Timer: " # __name  ": %f sec\n",                                  \
        pthread_self(), timespec_to_float(__t2) - timespec_to_float(__t1)); \
  } while(0)
#else
# define debug_timer_init(__t1, __t2)
# define timespec_to_float(__t)
# define debug_timer_start(__t)
# define debug_timer_end(__t1, __t2, __name)
#endif


typedef vector<string> errors_t;
extern errors_t error_stack;


inline void push_error(string& msg)
{
  error_stack.push_back(msg);
}


inline void push_error(const char* msg)
{
  error_stack.push_back(string(msg));
}


inline void print_all()
{
  if (!error_stack.empty()) {
    for (errors_t::iterator it = error_stack.begin(); it != error_stack.end(); ++it) {
      error_log("%s\n", (*it).c_str());
    }
    error_stack.clear();
  }
}

} // namespace log

} // namespace imtools

#endif // IMTOOLS_LOG_HXX

// vim: et ts=2 sts=2 sw=2
