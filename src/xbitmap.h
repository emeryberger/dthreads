// -*- C++ -*-
#ifndef _XBITMAP_H_
#define _XBITMAP_H_

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
 * @file   xbitmap.h
 * @brief  Shared pages' bit map. Each page has a corresponding bit.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xdefines.h"

class xbitmap {
	// Now one bitmap for each page is 512 bytes.
	enum {
		BITMAP_SIZE_PER_PAGE = 4096
	};
	enum {
		INITIAL_PAGES = 81920
	};

public:
	xbitmap() {
	}

	static xbitmap& getInstance(void) {
		static char buf[sizeof(xbitmap)];
		static xbitmap * theOneTrueObject = new (buf) xbitmap();
		return *theOneTrueObject;
	}

	void initialize(void) {
		int offset = 0;
		char * ptr;

		ptr = (char *) mmap(NULL, BITMAP_SIZE_PER_PAGE * (INITIAL_PAGES + 10),
				PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

		if (ptr == NULL) {
			fprintf(stderr, "%d fail to initialize bit map: %s\n", getpid(),
					strerror(errno));
			::abort();
		}

		// Assign the address for different variables.
		_cur = (int *) &ptr[offset];
		offset += sizeof(int);

		_versionStart = (int *) &ptr[offset];

		_pageStart = (int*)((intptr_t) ptr + 2 * xdefines::PageSize);

		// We will start as one since we think that bitmapIndex equal to 0 means no bitmap before.
		*_cur = 1;
		_total = INITIAL_PAGES;
	}

	int get(void) {
		int index;

		if ((*_cur + 1) < _total) {
			index = *_cur;
			(*_cur)++;
		} else {
			// we may want to increase the size of bitmap automatically in the future.
			fprintf(stderr, "%d: not enough bitmap _cur is %d????\n", getpid(), *_cur);
			assert(0);
		}
		return index;
	}

	void * getAddress(int index) {
		return ((void *) ((intptr_t) _pageStart + index * BITMAP_SIZE_PER_PAGE));
	}

	void setVersion(int index, int version) {
		_versionStart[index] = version;
	}

	int getVersion(int index) {
		assert(index != 0);
		return (_versionStart[index]);
	}

	// Cleanup will be called for every transaction.
	void cleanup(void) {
		// First, all used bitmap can be zeroed.
		//		memset(_pageStart, 0 , (*_cur) * BITMAP_SIZE_PER_PAGE);
		*_cur = 1;
	}

private:
	// Current index of entry that need to be allocated.
	int _total;
	int * _cur;
	int * _versionStart;
	void * _pageStart;
};

#endif
