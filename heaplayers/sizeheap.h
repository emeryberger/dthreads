/* -*- C++ -*- */

/*

  Heap Layers: An Extensible Memory Allocation Infrastructure
  
  Copyright (C) 2000-2007 by Emery Berger
  http://www.cs.umass.edu/~emery
  emery@cs.umass.edu
  
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

#ifndef _SIZEHEAP_H_
#define _SIZEHEAP_H_

/**
 * @file sizeheap.h
 * @brief Contains UseSizeHeap and SizeHeap.
 */

#include <assert.h>
#include <new>

#include "addheap.h"

/**
 * @class UseSizeHeap
 * @brief Adds a getSize method to access the size of an allocated object.
 * @see SizeHeap
 */

namespace HL {

template <class Super>
class UseSizeHeap : public Super {
public:
  
  inline static size_t getSize (const void * ptr) {
    return ((freeObject *) ptr - 1)->getSize();
  }

protected:
  class freeObject {
  public:
    freeObject (size_t sz)
      : _size (sz)
    {}
    size_t getSize (void) const {
      return _size;
    }
  private:
    union {
      size_t _size;
      double _dummy; // for alignment.
    };
  };
};

/**
 * @class SizeHeap
 * @brief Allocates extra room for the size of an object.
 */

template <class SuperHeap>
class SizeHeap : public UseSizeHeap<SuperHeap> {
  typedef typename UseSizeHeap<SuperHeap>::freeObject freeObject;
public:
  inline void * malloc (const size_t sz) {
    freeObject * ptr = new (SuperHeap::malloc (sz + sizeof(freeObject)))
			    freeObject (sz);
    void * obj = (void *) (ptr + 1);
    size_t size = UseSizeHeap<SuperHeap>::getSize(obj);
    assert (size >= sz);
    return obj;
  }
  inline void free (void * ptr) {
    SuperHeap::free (((freeObject *) ptr) - 1);
  }
};

};

#endif
