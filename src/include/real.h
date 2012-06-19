#ifndef _REAL_H_
#define _REAL_H_

#include <sys/types.h>
#include <pthread.h>

#define WRAP(x) _real_##x

// libc functions
extern void* (*WRAP(mmap))(void*, size_t, int, int, int, off_t);
extern void* (*WRAP(malloc))(size_t);
extern void  (*WRAP(free))(void *);
extern void* (*WRAP(realloc))(void *, size_t);
extern void* (*WRAP(memalign))(size_t, size_t);
extern size_t (*WRAP(malloc_usable_size))(void *);
extern ssize_t (*WRAP(read))(int, void*, size_t);
extern ssize_t (*WRAP(write))(int, const void*, size_t);
extern int (*WRAP(sigwait))(const sigset_t*, int*);

// pthread basics
extern int (*WRAP(pthread_create))(pthread_t*, const pthread_attr_t*, void *(*)(void*), void*);
extern int (*WRAP(pthread_cancel))(pthread_t);
extern int (*WRAP(pthread_join))(pthread_t, void**);
extern int (*WRAP(pthread_exit))(void*);

// pthread mutexes
extern int (*WRAP(pthread_mutexattr_init))(pthread_mutexattr_t*);
extern int (*WRAP(pthread_mutex_init))(pthread_mutex_t*, const pthread_mutexattr_t*);
extern int (*WRAP(pthread_mutex_lock))(pthread_mutex_t*);
extern int (*WRAP(pthread_mutex_unlock))(pthread_mutex_t*);
extern int (*WRAP(pthread_mutex_trylock))(pthread_mutex_t*);
extern int (*WRAP(pthread_mutex_destroy))(pthread_mutex_t*);

// pthread condition variables
extern int (*WRAP(pthread_condattr_init))(pthread_condattr_t*);
extern int (*WRAP(pthread_cond_init))(pthread_cond_t*, pthread_condattr_t*);
extern int (*WRAP(pthread_cond_wait))(pthread_cond_t*, pthread_mutex_t*);
extern int (*WRAP(pthread_cond_signal))(pthread_cond_t*);
extern int (*WRAP(pthread_cond_broadcast))(pthread_cond_t*);
extern int (*WRAP(pthread_cond_destroy))(pthread_cond_t*);

// pthread barriers
extern int (*WRAP(pthread_barrier_init))(pthread_barrier_t*, pthread_barrierattr_t*, unsigned int);
extern int (*WRAP(pthread_barrier_wait))(pthread_barrier_t*);
extern int (*WRAP(pthread_barrier_destroy))(pthread_barrier_t*);

void init_real_functions();

#endif
