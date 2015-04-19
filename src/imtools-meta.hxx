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
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#define IMTOOLS_VERSION "2.0.1-dev"


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
// Features

#ifdef IMTOOLS_THREADS_BACKEND
# define IMTOOLS_THREADS_FEATURE IMTOOLS_THREADS_BACKEND
#else
# define IMTOOLS_THREADS_FEATURE "Non-threaded"
#endif

#ifdef IMTOOLS_EXTRA
# define IMTOOLS_EXTRA_FEATURE "ExtraTools"
#else
# define IMTOOLS_EXTRA_FEATURE ""
#endif

#ifdef IMTOOLS_DEBUG
# define IMTOOLS_DEBUG_FEATURE "Debug"
#else
# define IMTOOLS_DEBUG_FEATURE "NoDebug"
#endif

#ifdef IMTOOLS_DEBUG_PROFILER
# define IMTOOLS_DEBUG_PROFILER_FEATURE "DebugProfiler"
#else
# define IMTOOLS_DEBUG_PROFILER_FEATURE "NoDebugProfiler"
#endif

#define IMTOOLS_FEATURES \
  IMTOOLS_THREADS_FEATURE " " \
  IMTOOLS_EXTRA_FEATURE " " \
  IMTOOLS_DEBUG_FEATURE " " \
  IMTOOLS_DEBUG_PROFILER_FEATURE

/////////////////////////////////////////////////////////////////////
#endif // IMTOOLS_HXX
// vim: et ts=2 sts=2 sw=2
