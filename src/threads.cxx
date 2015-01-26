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

#include "threads.hxx"

#ifdef IMTOOLS_THREADS
namespace imtools { namespace threads
{

it_lock_t io_lock;


it_thread_id_t
get_id()
{
#ifdef USE_OPENMP
  return omp_get_thread_num();
#else
  return boost::this_thread::get_id();
#endif
}

}} // namespace imtools::threads

#endif // IMTOOLS_THREADS

// vim: et ts=2 sts=2 sw=2
