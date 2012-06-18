#ifndef __GNUC__
#error "This file requires the GNU compiler."
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#define CUSTOM_PREFIX(n) _custom##n
#include "libhoard.cpp"

extern "C" {

  void * CUSTOM_PREFIX(malloc) (size_t);
  void CUSTOM_PREFIX(free) (void *);
  void * CUSTOM_PREFIX(realloc) (void *, size_t);
  void * CUSTOM_PREFIX(memalign) (size_t, size_t);

  static void my_init_hook (void);

  // New hooks for allocation functions.
  static void * my_malloc_hook (size_t, const void *);
  static void my_free_hook (void *, const void *);
  static void * my_realloc_hook (void *, size_t, const void *);
  static void * my_memalign_hook (size_t, size_t, const void *);

  // Store the old hooks just in case.
  static void * (*old_malloc_hook) (size_t, const void *);
  static void (*old_free_hook) (void *, const void *);
  static void *(*old_realloc_hook)(void *ptr, size_t size, const void *caller);
  static void *(*old_memalign_hook)(size_t alignment, size_t size, const void *caller);

  void (*__malloc_initialize_hook) (void) = my_init_hook;

  static void my_init_hook (void) {
    // Store the old hooks.
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    old_realloc_hook = __realloc_hook;
    old_memalign_hook = __memalign_hook;

    // Point the hooks to the replacement functions.
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
    __realloc_hook = my_realloc_hook;
    __memalign_hook = my_memalign_hook;

  }

  //  static size_t counter = 0;

  static void * my_malloc_hook (size_t size, const void * caller) {
    void * result;
    //    fprintf (stderr, "malloc\n");
#if 0
    counter++;
    if (counter > 900000)
      abort();
#endif
    result = CUSTOM_PREFIX(malloc) (size);
    return result;
  }

  static void my_free_hook (void * ptr, const void * caller) {
    CUSTOM_PREFIX(free) (ptr);
  }

  static void * my_realloc_hook (void * ptr, size_t size, const void * caller) {
    return CUSTOM_PREFIX(realloc) (ptr, size);
  }

  static void * my_memalign_hook (size_t size, size_t alignment, const void * caller) {
    return CUSTOM_PREFIX(memalign) (size, alignment);
  }

#if 0
  void finalizer (void) __attribute__((destructor));

  void finalizer (void) {
    printf ("counter = %d\n", counter);
  }
#endif

}

