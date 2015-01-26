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
#include "OmpGuard.hxx"

namespace imtools { namespace threads
{


OmpGuard::OmpGuard(omp_lock_t& lock)
: mLock(lock)
{
  omp_init_lock(&mLock);
  omp_set_lock(&mLock);
}


OmpGuard::~OmpGuard()
{
  omp_unset_lock(&mLock);
  omp_destroy_lock(&mLock);
}


}} // namespace imtools::threads
// vim: et ts=2 sts=2 sw=2
