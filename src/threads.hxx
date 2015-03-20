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
#ifndef USE_OPENMP
# error "OpenMP required"
#endif
#include <omp.h>
#include <thread> // std::thread::hardware_concurrency()
// OmpGuard is not the fastest way to lock in OpenMP. The `critical` sections
// are faster, for instance. However, OmpGuard is exception-safe (unlike
// `critical` sections). Plus we are using for debug/verbose/error messages
// only. Other kinds of critical sections shouldn't use IT_IO_SCOPED_LOCK
// macro!
#define IT_IO_SCOPED_LOCK(l) imtools::threads::OmpGuard l(imtools::threads::io_lock)
#define IT_SCOPED_LOCK(l,v) imtools::threads::OmpGuard l((v))
#define IT_INIT_OPENMP(__num_threads) do { \
  /* Disable setting threads number through environment variable */ \
  if (omp_get_dynamic()) omp_set_dynamic(0); \
  omp_set_num_threads((__num_threads)); \
} while (0)

namespace imtools { namespace threads {
/////////////////////////////////////////////////////////////////////

typedef int        it_thread_id_t;
typedef omp_lock_t it_lock_t;

extern it_lock_t io_lock;

it_thread_id_t get_id() noexcept;

/////////////////////////////////////////////////////////////////////

/// Returns number of concurrent threads supported.
inline unsigned
max_threads()
{
  return std::thread::hardware_concurrency();
}

/////////////////////////////////////////////////////////////////////
class OmpGuard
{
  public:
    OmpGuard(omp_lock_t& lock) noexcept;
    ~OmpGuard();

    OmpGuard() = delete;
    OmpGuard& operator=(const OmpGuard&) = delete;
    OmpGuard(const OmpGuard&) = delete;

  private:
    omp_lock_t& mLock;
};

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::threads

#else // no threads
# define IT_IO_SCOPED_LOCK(l)
# define IT_SCOPED_LOCK(l,v)
# define IT_INIT_OPENMP(__num_threads)
#endif // IMTOOLS_THREADS
#endif // IMTOOLS_THREADS_HXX
// vim: et ts=2 sts=2 sw=2
