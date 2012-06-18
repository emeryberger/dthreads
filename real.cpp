#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   real.cpp
 * @brief  Used for profiling purpose.
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie>
 */

#include <pthread.h>

#include "debug.h"
#include "real.h"

// libc functions
void* (*WRAP(mmap))(void*, size_t, int, int, int, off_t);
void* (*WRAP(malloc))(size_t);
void  (*WRAP(free))(void *);
void* (*WRAP(realloc))(void *, size_t);
void* (*WRAP(memalign))(size_t, size_t);
size_t (*WRAP(malloc_usable_size))(void *);
ssize_t (*WRAP(read))(int, void*, size_t);
ssize_t (*WRAP(write))(int, const void*, size_t);
int (*WRAP(sigwait))(const sigset_t*, int*);

// pthread basics
int (*WRAP(pthread_create))(pthread_t*, const pthread_attr_t*, void *(*)(void*), void*);
int (*WRAP(pthread_cancel))(pthread_t);
int (*WRAP(pthread_join))(pthread_t, void**);
int (*WRAP(pthread_exit))(void*);

// pthread mutexes
int (*WRAP(pthread_mutexattr_init))(pthread_mutexattr_t*);
int (*WRAP(pthread_mutex_init))(pthread_mutex_t*, const pthread_mutexattr_t*);
int (*WRAP(pthread_mutex_lock))(pthread_mutex_t*);
int (*WRAP(pthread_mutex_unlock))(pthread_mutex_t*);
int (*WRAP(pthread_mutex_trylock))(pthread_mutex_t*);
int (*WRAP(pthread_mutex_destroy))(pthread_mutex_t*);

// pthread condition variables
int (*WRAP(pthread_condattr_init))(pthread_condattr_t*);
int (*WRAP(pthread_cond_init))(pthread_cond_t*, pthread_condattr_t*);
int (*WRAP(pthread_cond_wait))(pthread_cond_t*, pthread_mutex_t*);
int (*WRAP(pthread_cond_signal))(pthread_cond_t*);
int (*WRAP(pthread_cond_broadcast))(pthread_cond_t*);
int (*WRAP(pthread_cond_destroy))(pthread_cond_t*);

// pthread barriers
int (*WRAP(pthread_barrier_init))(pthread_barrier_t*, pthread_barrierattr_t*, unsigned int);
int (*WRAP(pthread_barrier_wait))(pthread_barrier_t*);
int (*WRAP(pthread_barrier_destroy))(pthread_barrier_t*);

#ifndef assert
#define assert(x) 
#endif

#define SET_WRAPPED(x, handle) WRAP(x) = (typeof(WRAP(x)))dlsym(handle, #x); assert(#x)

void init_real_functions() {
	DEBUG("initializing references to replaced functions in libc and libpthread");

	SET_WRAPPED(mmap, RTLD_NEXT);
	SET_WRAPPED(malloc, RTLD_NEXT);
	SET_WRAPPED(free, RTLD_NEXT);
	SET_WRAPPED(realloc, RTLD_NEXT);
	SET_WRAPPED(memalign, RTLD_NEXT);
	SET_WRAPPED(malloc_usable_size, RTLD_NEXT);
	SET_WRAPPED(read, RTLD_NEXT);
	SET_WRAPPED(write, RTLD_NEXT);
	SET_WRAPPED(sigwait, RTLD_NEXT);

	void *pthread_handle = dlopen("libpthread.so.0", RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD);
	if (pthread_handle == NULL) {
		fprintf(stderr, "Unable to load libpthread.so.0\n");
		_exit(2);
	}

	SET_WRAPPED(pthread_create, pthread_handle);
	SET_WRAPPED(pthread_cancel, pthread_handle);
	SET_WRAPPED(pthread_join, pthread_handle);
	SET_WRAPPED(pthread_exit, pthread_handle);

	SET_WRAPPED(pthread_mutex_init, pthread_handle);
	SET_WRAPPED(pthread_mutex_lock, pthread_handle);
	SET_WRAPPED(pthread_mutex_unlock, pthread_handle);
	SET_WRAPPED(pthread_mutex_trylock, pthread_handle);
	SET_WRAPPED(pthread_mutex_destroy, pthread_handle);
	SET_WRAPPED(pthread_mutexattr_init, pthread_handle);

	SET_WRAPPED(pthread_condattr_init, pthread_handle);
	SET_WRAPPED(pthread_cond_init, pthread_handle);
	SET_WRAPPED(pthread_cond_wait, pthread_handle);
	SET_WRAPPED(pthread_cond_signal, pthread_handle);
	SET_WRAPPED(pthread_cond_broadcast, pthread_handle);
	SET_WRAPPED(pthread_cond_destroy, pthread_handle);

	SET_WRAPPED(pthread_barrier_init, pthread_handle);
	SET_WRAPPED(pthread_barrier_wait, pthread_handle);
	SET_WRAPPED(pthread_barrier_destroy, pthread_handle);
}
