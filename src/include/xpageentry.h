// -*- C++ -*-
#ifndef _XPAGEENTRY_H_
#define _XPAGEENTRY_H_
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
 * @file   xpageentry.h
 * @brief  Private page entry to be used by different threads.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xdefines.h"
#include "xpageinfo.h"

/* This class is used to manage the page entries.
 * Page fault handler will ask for one page entry here.
 * Normally, we will keep 256 pages entries. If the page entry 
 * is not enough, then we can allocate 64 more, but those will be
 * freed in atomicEnd() to avoid unnecessary memory blowup.
 * Since one process will have one own copy of this and it is served
 * for one process only, memory can be allocated from private heap.
 */
class xpageentry {
	enum {
		PAGE_ENTRY_NUM = 800000
	};
public:
	xpageentry() {
		_start = NULL;
		_cur = 0;
		_total = 0;
	}

	static xpageentry& getInstance(void) {
		static char buf[sizeof(xpageentry)];
		static xpageentry * theOneTrueObject = new (buf) xpageentry();
		return *theOneTrueObject;
	}

	void initialize(void) {
		void * start;
		struct xpageinfo * cur;
		int i = 0;
		unsigned long pagestart;

		// We don't need to allocate all pages, only the difference between newnum and oldnum.
		start = mmap(NULL, PAGE_ENTRY_NUM * sizeof(xpageinfo), PROT_READ
				| PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (start == NULL) {
			fprintf(stderr, "%d fail to allocate page entries : %s\n",
					getpid(), strerror(errno));
			::abort();
		}

		// start to initialize it.
		_cur = 0;
		_total = PAGE_ENTRY_NUM;
		_start = (struct xpageinfo *) start;
		return;
	}

	struct xpageinfo * alloc(void) {
		struct xpageinfo * entry = NULL;
		if (_cur < _total) {
			entry = &_start[_cur];
			_cur++;
		} else {
			// There is no enough entry now, re-allocate new entries now.
			fprintf(stderr,
					"NO enough page entry, now _cur %x, _total %x!!!\n", _cur,
					_total);
			::abort();
		}
		return entry;
	}

	void cleanup(void) {
		_cur = 0;
	}

private:
	// How many entries in total.
	int _total;

	// Current index of entry that need to be allocated.
	int _cur;

	struct xpageinfo * _start;
};

#endif
