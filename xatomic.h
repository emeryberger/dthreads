// -*- C++ -*-

/*
 Author: Emery Berger, http://www.cs.umass.edu/~emery
 
 Copyright (c) 2007-8 Emery Berger, University of Massachusetts Amherst.

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

/**
 * @class xatomic
 * @brief A wrapper class for basic atomic hardware operations.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XATOMIC_H_
#define _XATOMIC_H_

class xatomic {
public:

  inline static unsigned long exchange(volatile unsigned long * oldval,
      unsigned long newval) {
#if defined(__i386__)
    asm volatile ("lock; xchgl %0, %1"
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
#elif defined(__sparc)
    asm volatile ("swap [%1],%0"
        :"=r" (newval)
        :"r" (oldval), "0" (newval)
        : "memory");
#elif defined(__x86_64__)
    // Contributed by Kurt Roeckx.
    asm volatile ("lock; xchgq %0, %1"
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
#else
#error "Not supported on this architecture."
#endif
    return newval;
  }

  // Atomic increment 1 and return the original value.
  static inline int increment_and_return(volatile unsigned long * obj) {
    int i = 1;
    asm volatile("lock; xaddl %0, %1"
        : "+r" (i), "+m" (*obj)
        : : "memory");
    return i;
  }

  static inline void increment(volatile unsigned long * obj) {
    asm volatile("lock; incl %0"
        : "+m" (*obj)
        : : "memory");
  }

  static inline void add(int i, volatile unsigned long * obj) {
    asm volatile("lock; addl %0, %1"
        : "+r" (i), "+m" (*obj)
        : : "memory");
  }

  static inline void decrement(volatile unsigned long * obj) {
    asm volatile("lock; decl %0;"
        : :"m" (*obj)
        : "memory");
  }

  // Atomic decrement 1 and return the original value.
  static inline int decrement_and_return(volatile unsigned long * obj) {
    int i = -1;
    asm volatile("lock; xaddl %0, %1"
        : "+r" (i), "+m" (*obj)
        : : "memory");
    return i;
  }

  static inline void atomic_set(volatile unsigned long * oldval,
      unsigned long newval) {
    asm volatile ("lock; xchgl %0, %1"
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
    return;
  }

  static inline int atomic_read(const volatile unsigned long *obj) {
    return (*obj);
  }

  static inline void memoryBarrier(void) {
    // Memory barrier: x86 only for now.
    __asm__ __volatile__ ("mfence": : :"memory");
  }

};

#endif
