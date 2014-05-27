/* Copyright © 2014 - Ruslan Osmanov <rrosmanov@gmail.com>
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


#ifndef IMTOOLS_IMMERGE_HXX
#define IMTOOLS_IMMERGE_HXX

#include "imtools.hxx"
#include "immerge-vars.hxx"

#define IMTOOLS_THREAD_FAILURE_VAL() g_thread_failure
#define IMTOOLS_THREAD_FAILURE_SET(v)  \
  do {                                 \
    IT_LOCK(g_thread_failure_mutex);   \
    g_thread_failure = (v);            \
    IT_UNLOCK(g_thread_failure_mutex); \
  } while (0)

#define IMTOOLS_THREAD_FAILURE_CHECK(__ret) \
  do {                                       \
    IT_LOCK(g_thread_failure_mutex);         \
    if (g_thread_failure) {                  \
      IT_UNLOCK(g_thread_failure_mutex);     \
      return __ret;                          \
    }                                        \
    IT_UNLOCK(g_thread_failure_mutex);       \
  } while (0)


#endif // IMTOOLS_IMMERGE_HXX

// vim: et ts=2 sts=2 sw=2
