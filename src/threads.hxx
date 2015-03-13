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
#ifndef IMTOOLS_THREADS_HXX
#define IMTOOLS_THREADS_HXX

#ifdef IMTOOLS_THREADS
# ifdef USE_OPENMP
#  include <omp.h>
#  include "OmpGuard.hxx"
// OmpGuard is not the fastest way to lock in OpenMP. The `critical` sections
// are faster, for instance. However, OmpGuard is exception-safe (unlike
// `critical` sections). Plus we are using for debug/verbose/error messages
// only. Other kinds of critical sections shouldn't use IT_IO_SCOPED_LOCK
// macro!
#  define IT_IO_SCOPED_LOCK(l) imtools::threads::OmpGuard l(imtools::threads::io_lock)
#  define IT_SCOPED_LOCK(l,v) imtools::threads::OmpGuard l((v))

#  define IT_INIT_OPENMP(__num_threads) do { \
  /* Disable setting threads number through environment variable */ \
  if (omp_get_dynamic()) omp_set_dynamic(0); \
  omp_set_num_threads((__num_threads)); \
} while (0)

# else // ! USE_OPENMP = use boost
#  include <boost/thread/mutex.hpp>
#  include <boost/bind.hpp>
#  include <boost/threadpool.hpp>
#  define IT_IO_SCOPED_LOCK(l) boost::mutex::scoped_lock l(imtools::threads::io_lock)
#  define IT_SCOPED_LOCK(l,v) boost::mutex::scoped_lock l((v))
# endif // USE_OPENMP

namespace imtools { namespace threads
{

# ifdef USE_OPENMP
typedef int        it_thread_id_t;
typedef omp_lock_t it_lock_t;
# else // ! USE_OPENMP = use boost
typedef boost::thread::id it_thread_id_t;
typedef boost::mutex      it_lock_t;
# endif

it_thread_id_t get_id();
extern it_lock_t io_lock;

}} // namespace imtools::threads

#else // no threads
# define IT_IO_SCOPED_LOCK(l)
#endif // IMTOOLS_THREADS
#endif // IMTOOLS_THREADS_HXX
// vim: et ts=2 sts=2 sw=2
