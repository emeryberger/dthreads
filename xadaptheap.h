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

#ifndef _XADAPTHEAP_H_
#define _XADAPTHEAP_H_

/**
 * @class xadaptheap
 * @brief Manages a heap whose metadata is allocated from a given source.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

template<template<class S, int Size> class Heap, class Source, int ChunkSize>
class xadaptheap: public Source {
public:

	xadaptheap(void) {
		int metasize = sizeof(Heap<Source, ChunkSize> );
		//void * buf = Source::malloc (metasize);
		void * buf = mmap(NULL, metasize, PROT_READ | PROT_WRITE, MAP_SHARED
				| MAP_ANONYMOUS, -1, 0);
		if (buf == NULL) {
			fprintf(stderr, "Failed to get memory to store the metadata\n");
			exit(-1);
		}
		_heap = new (buf) Heap<Source, ChunkSize> ;
		DEBUG("xadaptheap _heap %p size %x", _heap, metasize);
	}

	virtual ~xadaptheap(void) {
		Source::free(_heap);
	}

	void * malloc(int heapid, size_t sz) {
		return _heap->malloc(heapid, sz);
	}

	void free(int heapid, void * ptr) {
		_heap->free(heapid, ptr);
	}

	size_t getSize(void * ptr) {
		return _heap->getSize(ptr);
	}

private:

	Heap<Source, ChunkSize> * _heap;
};

#endif // _XADAPTHEAP_H_
