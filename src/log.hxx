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

#ifdef IMTOOLS_DEBUG
# define debug_log(...) printf("Debug: " __VA_ARGS__)
#else
# define debug_log(...)
#endif

#define verbose_log(...)                            \
  do {                                              \
    if (imtools::verbose) printf("* " __VA_ARGS__); \
  } while (0)

#define error_log(...) fprintf(stderr, "!! Error: " __VA_ARGS__)
#define warning_log(...) fprintf(stderr, "** Warning: " __VA_ARGS__)

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
# define debug_timer_end(__t1, __t2, __name)                                           \
  do {                                                                                 \
    clock_gettime(CLOCK_REALTIME, &(__t2));                                            \
    printf(# __name  ": %f sec\n", timespec_to_float(__t2) - timespec_to_float(__t1)); \
  } while(0)
#else
# define debug_timer_init(__t1, __t2)
# define timespec_to_float(__t)
# define debug_timer_start(__t)
# define debug_timer_end(__t1, __t2, __name)
#endif

#endif // IMTOOLS_LOG_HXX
// vim: et ts=2 sts=2 sw=2
