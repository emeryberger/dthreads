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

/*
 * @file   xheap.h
 * @brief  A basic bump pointer heap, used as a source for other heap layers.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XHEAP_H_
#define _XHEAP_H_

#include "xpersist.h"
#include "xdefines.h"
#include "xplock.h"
#include "debug.h"

template<int Size>
class xheap: public xpersist<char, Size> {
	typedef xpersist<char, Size> parent;

public:

	// It is very very important to put the first page in a separate area since it will invoke a
	// lot of un-necessary rollbacks, which can affect the performance greately and also can affect
	// the correctness of race detection, since different threads can end up getting the same memory
	// block from class "xheap".
	// Although we can rely on "rollback" mechanism to avoid the problem,
	// but we still need to differentiate those races caused by this and actual races in user application.
	// It is not good to do this.
	// It is possible that we don't need sanityCheck any more, but we definitely need a lock to avoid the race on "metatdata".
	xheap(void) {
		// Since we don't know whether shareheap has been initialized here, just use mmap to assign
		// one page to hold all data. We are pretty sure that one page is enough to hold all contents.
		char * base;

		// Allocate a share page to hold all heap metadata.
		base = (char *) mmap(NULL, xdefines::PageSize, PROT_READ | PROT_WRITE,
				MAP_SHARED | MAP_ANONYMOUS, -1, 0);

		// Put all "heap metadata" in this page.
		_position = (volatile char **) base;
		_remaining = (volatile size_t *) (base + 1 * sizeof(char *));
		_magic = (size_t *) (base + 2 * sizeof(void *));
#ifdef DETERM_MEMORY_ALLOC
		_mutex = (pthread_mutex_t *)(base + 3 * sizeof(void *));
		pthread_mutex_init(_mutex, NULL);
		//*((void **)_mutex) = NULL;
#else
		_lock = new (base + 3 * sizeof(void *)) xplock;
#endif
		// Initialize the following content according the values of xpersist class.
		//_start =(char*) ((intptr_t)parent::base() - 0x4000);
		_start = parent::base();
		_end = _start + parent::size();
		*_position = (char *) _start;
		*_remaining = parent::size();
		*_magic = 0xCAFEBABE;

		DEBUG("xheap initializing: _position %p, _remaining %p, _magic %p, _start %p, _end %p\n", _position, _remaining, _magic, _start, _end);
	}

	inline void * getend(void) {
		return (void *)*_position;
	}

	// We need to page-aligned size, we don't need two different threads are using the same page here.
	inline void * malloc(size_t sz) {
		sanityCheck();

		// Roud up the size to page aligned.
		sz = xdefines::PageSize * ((sz + xdefines::PageSize - 1)
				/ xdefines::PageSize);

#ifdef DETERM_MEMORY_ALLOC
		pthread_mutex_lock(_mutex);
#else
		_lock->lock();
#endif

		if (*_remaining == 0) {
			fprintf(stderr, "FOOP: something very bad has happened: _start = %x, _position = %p, _remaining = %Zx.\n", *_start, *_position, *_remaining);
		}

		if (*_remaining < sz) {
			fprintf(stderr, "CRAP: remaining[%Zx], sz[%Zx] thread[%d]\n", *_remaining, sz, (int) pthread_self());
			exit(-1);
		}
		void * p = (void *)*_position;

		// Increment the bump pointer and drop the amount of memory.
		*_remaining -= sz;
		*_position += sz;

		void * newptr = (void*)*_position;
		//__asm__ __volatile__ ("mfence": : :"memory");
		
//		fprintf(stderr, "%d: xheapmalloc %p ~ %p. sz %x\n", getpid(),p, newptr, sz);
#ifdef DETERM_MEMORY_ALLOC
		pthread_mutex_unlock(_mutex);
#else
		_lock->unlock();
#endif
		
#ifdef LAZY_COMMIT
		parent::setOwnedPage(p, sz);
#endif

		return p;
	}

	void initialize(void) {
#ifdef DETERM_MEMORY_ALLOC
		pthread_mutex_init(_mutex, NULL);
#endif
		parent::initialize();
	}

	/// @brief Call this before every transaction begins.
	void begin(bool cleanup) {
		sanityCheck();
		parent::begin(cleanup);
	}

	/// @brief Call this when a transaction commits.
	void commit(bool update) {
		sanityCheck();
		parent::commit(update);
	}

	// These should never be used.
	inline void free(void * ptr) {
		sanityCheck();
	}
	inline size_t getSize(void * ptr) {
		sanityCheck();
		return 0;
	} 

private:

	void sanityCheck(void) {
		if (*_magic != 0xCAFEBABE) {
			fprintf(stderr, "%d : WTF with magic %Zx!\n", getpid(), *_magic);
			::abort();
		}
	}

	/// The start of the heap area.
	volatile char * _start;

	/// The end of the heap area.
	volatile char * _end;

	/// Pointer to the current bump pointer.
	volatile char ** _position;

	/// Pointer to the amount of memory remaining.
	volatile size_t* _remaining;

	size_t* _magic;

#ifdef DETERM_MEMORY_ALLOC
	pthread_mutex_t * _mutex;
#else
	// We will use a lock to protect the allocation request from different threads.
	xplock* _lock;
#endif
};

#endif
