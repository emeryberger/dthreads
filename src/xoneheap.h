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

#ifndef _XONEHEAP_H_
#define _XONEHEAP_H_

/**
 * @class xoneheap
 * @brief Wraps a single heap instance.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

template<class SourceHeap>
class xoneheap {
public:
  xoneheap() {
  }

  void initialize(void) {
    getHeap()->initialize();
  }
  void finalize(void) {
    getHeap()->finalize();
  }
  void begin(bool cleanup) {
    getHeap()->begin(cleanup);
  }

#ifdef LAZY_COMMIT
  void finalcommit(bool release) {
    getHeap()->finalcommit(release);
  }

  void forceCommit(int pid, void * end) {
    getHeap()->forceCommitOwnedPages(pid, end);
  }
#endif
  
  void checkandcommit(bool update) {
    getHeap()->checkandcommit(update);
  }

  void * getend(void) {
    return getHeap()->getend();
  }

  void stats(void) {
    getHeap()->stats();
  }

  void openProtection(void *end) {
    getHeap()->openProtection(end);
  }
  void closeProtection(void) {
    getHeap()->closeProtection();
  }

  bool nop(void) {
    return getHeap()->nop();
  }

  bool inRange(void * ptr) {
    return getHeap()->inRange(ptr);
  }
  void handleWrite(void * ptr) {
    getHeap()->handleWrite(ptr);
  }

#ifdef LAZY_COMMIT
  void cleanupOwnedBlocks(void) {
    getHeap()->cleanupOwnedBlocks();
  } 

  void commitOwnedPage(int page_no, bool set_shared) {
    getHeap()->commitOwnedPage(page_no, set_shared);
  }
#endif
  
  bool mem_write(void * dest, void *val) {
    return getHeap()->mem_write(dest, val);
  }

  void setThreadIndex(int index) {
    return getHeap()->setThreadIndex(index);
  }

  void * malloc(size_t sz) {
    return getHeap()->malloc(sz);
  }
  void free(void * ptr) {
    getHeap()->free(ptr);
  }
  size_t getSize(void * ptr) {
    return getHeap()->getSize(ptr);
  }
private:

  SourceHeap * getHeap(void) {
    static char heapbuf[sizeof(SourceHeap)];
    static SourceHeap * _heap = new (heapbuf) SourceHeap;
    //fprintf (stderr, "heapbuf is %p\n", _heap);
    return _heap;
  }

};

#endif // _XONEHEAP_H_
