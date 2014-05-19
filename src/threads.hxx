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

#ifndef IMTOOLS_THREADS_HXX
#define IMTOOLS_THREADS_HXX

#ifdef IMTOOLS_THREADS
# include <pthread.h>
#endif


#if defined(IMTOOLS_THREADS)

# if (__linux || __linux__) && defined (PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP)
#   define IT_MUTEX_CREATE(mutex)                                \
  do {                                                           \
    pthread_mutexattr_t attr;                                    \
    pthread_mutexattr_init(&attr);                               \
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP); \
    pthread_mutex_init(&(mutex), &attr);                         \
  } while (0)
# else // not linux or PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP is not defined
#   define IT_MUTEX_CREATE(mutex) pthread_mutex_init (&(mutex), 0)
# endif

# define IT_LOCK(mutex) pthread_mutex_lock(&(mutex))
# define IT_UNLOCK(mutex) pthread_mutex_unlock(&(mutex))

# define IT_COND_INIT PTHREAD_COND_INITIALIZER
# define IT_COND_CREATE(cond) pthread_cond_init(&(cond), 0)
# define IT_COND_SIGNAL(cond) pthread_cond_signal(&(cond))
# define IT_COND_WAIT(cond,mutex) pthread_cond_wait (&(cond), &(mutex))
# define IT_COND_TIMEDWAIT(cond,mutex,to) pthread_cond_timedwait (&(cond), &(mutex), &(to))

#else // no threads support
#   define IT_MUTEX_CREATE(mutex)
#   define IT_LOCK(mutex)
#   define IT_UNLOCK(mutex)
#   define IT_COND_INIT
#   define IT_COND_CREATE(cond)
#   define IT_COND_SIGNAL(cond)
#   define IT_COND_WAIT(cond,mutex)
#   define IT_COND_TIMEDWAIT(cond,mutex,to)
#endif

#endif // IMTOOLS_THREADS_HXX
// vim: et ts=2 sts=2 sw=2
