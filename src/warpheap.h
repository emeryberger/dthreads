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
 * @file   warpheap.h
 * @brief  A heap optimized to reduce the likelihood of false sharing.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _WARPHEAP_H_
#define _WARPHEAP_H_

#include "xdefines.h"
#include "heaplayers/util/sassert.h"
#include "xadaptheap.h"
#include "real.h"

#include "heaplayers/ansiwrapper.h"
#include "heaplayers/kingsleyheap.h"
#include "heaplayers/adapt.h"
//#include "heaplayers/adaptheap.h"
#include "heaplayers/util/sllist.h"
#include "heaplayers/util/dllist.h"
#include "heaplayers/sanitycheckheap.h"
#include "heaplayers/zoneheap.h"
#include "objectheader.h"

#define ALIGN_TO_PAGE 0 // doesn't work...
template<class SourceHeap>
class NewSourceHeap: public SourceHeap {
public:
	void * malloc(size_t sz) {

#if ALIGN_TO_PAGE
		if (sz >= 4096 - sizeof(objectHeader)) {
			sz += 4096;
		}
#endif

		//void * ptr = SourceHeap::malloc (size);
		//fprintf(stderr, "%d in newsourceHeap\n", getpid());
		void * ptr = SourceHeap::malloc(sz + sizeof(objectHeader));
		if (!ptr) {
			return NULL;
		}
#if 0 // ALIGN_TO_PAGE
		if (sz >= 4096 - sizeof(objectHeader)) {
			ptr = (void *) (((((size_t) ptr) + 4095) & ~4095) - sizeof(objectHeader));
		}
#endif
		objectHeader * o = new (ptr) objectHeader(sz);
		void * newptr = getPointer(o);

		assert (getSize(newptr) >= sz);
		return newptr;
	}
	void free(void * ptr) {
		SourceHeap::free((void *) getObject(ptr));
	}
	size_t getSize(void * ptr) {
		objectHeader * o = getObject(ptr);
		size_t sz = o->getSize();
		if (sz == 0) {
			fprintf(stderr, "%d : getSize error, with ptr = %p. sz %Zd \n", getpid(), ptr, sz);
		}
		return sz;
	}
private:

	static objectHeader * getObject(void * ptr) {
		objectHeader * o = (objectHeader *) ptr;
		return (o - 1);
	}

	static void * getPointer(objectHeader * o) {
		return (void *) (o + 1);
	}
};

template<class SourceHeap, int chunky>
class KingsleyStyleHeap: public HL::ANSIWrapper<HL::StrictSegHeap<
		Kingsley::NUMBINS, Kingsley::size2Class, Kingsley::class2Size,
		HL::AdaptHeap<HL::SLList, NewSourceHeap<SourceHeap> >, NewSourceHeap<
				HL::ZoneHeap<SourceHeap, chunky> > > > {
private:

	typedef HL::ANSIWrapper<HL::StrictSegHeap<Kingsley::NUMBINS,
			Kingsley::size2Class, Kingsley::class2Size, HL::AdaptHeap<
					HL::SLList, NewSourceHeap<SourceHeap> >, NewSourceHeap<
					HL::ZoneHeap<SourceHeap, chunky> > > > SuperHeap;

public:
	KingsleyStyleHeap(void) {
		//     printf ("this kingsley = %p\n", this);
	}

	void * malloc(size_t sz) {
		void * ptr = SuperHeap::malloc(sz);
		return ptr;
	}

private:
	// char buf[4096 - (sizeof(SuperHeap) % 4096) - sizeof(int)];
//  char buf[4096 - (sizeof(SuperHeap) % 4096)];
};

template<int NumHeaps, class TheHeapType>
class PPHeap: public TheHeapType {
public:

	PPHeap(void) {
		/// The lock's attributes.
		pthread_mutexattr_t attr;

		// Set up the lock with a shared attribute.
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

		// Instantiate the lock structure inside a shared mmap.
		char * base;

		// Allocate a share page to hold all heap metadata.
		base = (char *) mmap(NULL, sizeof(pthread_mutex_t) * NumHeaps,
				PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (base == NULL) {
			fprintf(stderr, "PPheap initialize failed.\n");
			exit(0);
		}
		for (int i = 0; i < NumHeaps; i++) {
			_lock[i] = (pthread_mutex_t *) ((intptr_t) base + sizeof(pthread_mutex_t) * i);
			pthread_mutex_init(_lock[i], &attr);
		}
	}

	void * malloc(int ind, size_t sz) {
		lock(ind);
		// Try to get memory from the local heap first.
		void * ptr = _heap[ind].malloc(sz);
		
		unlock(ind);
		return ptr;
	}

	void free(int ind, void * ptr) {
		lock(ind);
		// Put the freed object onto this thread's heap.  Note that this
		// policy is essentially pure private heaps, (see Berger et
		// al. ASPLOS 2000), and so suffers from numerous known problems.
		_heap[ind].free(ptr);
		unlock(ind);
	}

	void lock(int ind) {
		WRAP(pthread_mutex_lock)(_lock[ind]);
	}

	void unlock(int ind) {
		WRAP(pthread_mutex_unlock)(_lock[ind]);
	}
	
private:
	pthread_mutex_t * _lock[NumHeaps];
	TheHeapType _heap[NumHeaps];
};

template<class SourceHeap, int ChunkSize>
class PerThreadHeap: public PPHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap<
		SourceHeap, ChunkSize> > {
};

template<int NumHeaps, int ChunkSize, class SourceHeap>
class warpheap: public xadaptheap<PerThreadHeap, SourceHeap, ChunkSize> {
};

#endif // _WARPHEAP_H_
