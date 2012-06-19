// -*- C++ -*-

#ifndef _INTERNALHEAP_H_
#define _INTERNALHEAP_H_

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

#include "warpheap.h"
#include "xdefines.h"

/**
 * @file InternalHeap.h
 * @brief A share heap for Grace's internal allocation needs.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *
 */
class SourceInternalHeap{
public:
  SourceInternalHeap (void)
    : _alreadyMalloced (false)
  {}

  void * malloc (size_t) {
    if (_alreadyMalloced) {
      return NULL;
    } else {
      void * start;
      // Create a MAP_SHARED memory
      start = mmap (NULL, xdefines::INTERNALHEAP_SIZE+xdefines::PageSize, PROT_READ | PROT_WRITE, 
			              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
   
      if(start == NULL) {
		    fprintf(stderr, "Failed to create a internal share heap.\n");
		    exit(1);
      }
      
      _alreadyMalloced = true;
      return(start);
    }
  }
  
  void free (void * addr) {}

private:

  bool _alreadyMalloced;  

};

// Note: we will assign all memory to KingsleyStyleHeap at one time.
// In previous version, KingsleyStyleHeap will ask for memory by chunk, that is, only 1048576 (CHUNKY in warpheap.h) 
// memory can be allocated from SourceShareHeap. Second time allocation will always return NULL. 
// To fix this, we add a 'chunky' template parameter to KingsleyStyleHeap. Now SourceShareHeap will assign all 
// memory to KingsleyStyleHeap at one time now. 
class InternalHeap : public KingsleyStyleHeap<SourceInternalHeap, xdefines::INTERNALHEAP_SIZE> {
public:
  typedef KingsleyStyleHeap<SourceInternalHeap, xdefines::INTERNALHEAP_SIZE> SuperHeap;

  InternalHeap() {}
  
  void initialize() {
    pthread_mutexattr_t attr;

    // Set up the lock with a shared attribute.
    WRAP(pthread_mutexattr_init)(&attr);
    pthread_mutexattr_setpshared (&attr, PTHREAD_PROCESS_SHARED);

	  // Allocate a lock to use internally
    _lock = new (mmap (NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) pthread_mutex_t;
  	if(_lock == NULL) {
  		printf("Fail to initialize an internal lock for monitor map\n");
  		_exit(-1);
  	}
  
  	WRAP(pthread_mutex_init) (_lock, &attr);
  
  	// Since SourceInternalHeap only initializes when there is some malloc,
  	// try to malloc something in order to make the "sharedheap" actually shared by all threads.
  	void *  testptr = malloc(8);
  }
 
  ~InternalHeap (void) {}
  
  // Just one accessor.  Why? We don't want more than one (singleton)
  // and we want access to it neatly encapsulated here.
  static InternalHeap& getInstance (void) {
	  static bool internalHeapAllocated = false;
    static InternalHeap * internalHeapObject;
    if(!internalHeapAllocated) {
        void *buf = mmap(NULL, sizeof(InternalHeap), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        internalHeapObject = new(buf) InternalHeap();
        internalHeapAllocated = true;
    }

    return *internalHeapObject;
  }
  
  void * malloc (size_t sz) {
    void * ptr = NULL;
    lock();	
    ptr = SuperHeap::malloc (sz);
    unlock();
  	if(!ptr) {
	  	fprintf(stderr, "%d : SHAREHEAP is exhausted, exit now!!!\n", getpid());
		  assert(ptr != NULL);
	  }
	
    return ptr;
  }
  
  void free (void * ptr) {
    lock();	
    SuperHeap::free (ptr);
    unlock();
  }
  
private:
  
  // The lock is used to protect the update on global _monitors
  void lock(void) {
    WRAP(pthread_mutex_lock) (_lock);
  }
  
  void unlock(void) {
    WRAP(pthread_mutex_unlock) (_lock);
  }
  
  // Internal lock
  pthread_mutex_t * _lock; 
};


class InternalHeapAllocator {
public:
  void * malloc (size_t sz) {
    return InternalHeap::getInstance().malloc(sz);
  }
  
  void free (void * ptr) {
    return InternalHeap::getInstance().free(ptr);
  }
};

#endif
