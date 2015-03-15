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
#ifndef IMTOOLS_META_HXX
#define IMTOOLS_META_HXX
/////////////////////////////////////////////////////////////////////

#define IMTOOLS_VERSION "1.3.0"


#ifdef IMTOOLS_DEBUG
# define IMTOOLS_BUILD_TYPE "debug"
#else
# define IMTOOLS_BUILD_TYPE "release"
#endif


#ifdef IMTOOLS_THREADS
# define IMTOOLS_THREADS_BACKEND "OpenMP"
# define IMTOOLS_SUFFIX "threaded" "(" IMTOOLS_THREADS_BACKEND ")"
#else
# define IMTOOLS_SUFFIX "non-threaded"
#endif


#define IMTOOLS_FULL_NAME "ImTools " IMTOOLS_VERSION \
  "-" IMTOOLS_BUILD_TYPE " " IMTOOLS_SUFFIX
#define IMTOOLS_COPYRIGHT "Copyright (C) 2014,2015 - Ruslan Osmanov <rrosmanov@gmail.com>"

/////////////////////////////////////////////////////////////////////
#endif // IMTOOLS_HXX
// vim: et ts=2 sts=2 sw=2
