// -*- C++ -*-
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
 * @file   xmemory.h
 * @brief  Manage different kinds of memory, main entry for memory.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie>
 */

#ifndef _XMEMORY_H_
#define _XMEMORY_H_

#include <signal.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <set>

#include "xglobals.h"

// Heap Layers
#include "heaplayers/stlallocator.h"
#include "warpheap.h"

#include "xoneheap.h"
#include "xheap.h"

#include "xpageentry.h"
#include "objectheader.h"
#include "internalheap.h"
// Encapsulates all memory spaces (globals & heap).

class xmemory {
private:
  /// The globals region.
  static xglobals _globals;

  /// Protected heap.
  static warpheap<xdefines::NUM_HEAPS, xdefines::PROTECTEDHEAP_CHUNK, xoneheap<xheap<xdefines::PROTECTEDHEAP_SIZE> > > _pheap;

  /// A signal stack, for catching signals.
  static stack_t _sigstk;

  static int _heapid;

  // Private on purpose
  xmemory(void) {}

public:

  static void initialize(void) {
    DEBUG("initializing xmemory");
    // Intercept SEGV signals (used for trapping initial reads and
    // writes to pages).
    installSignalHandler();

    // Call _pheap so that xheap.h can be initialized at first and then can work normally.
    _pheap.initialize();
    _globals.initialize();
    xpageentry::getInstance().initialize();

    // Initialize the internal heap.
    InternalHeap::getInstance().initialize(); 
  }

  static void finalize(void) {
    _globals.finalize();
    _pheap.finalize();
  }

  static void setThreadIndex(int id) {
    _globals.setThreadIndex(id);
    _pheap.setThreadIndex(id);

    // Calculate the sub-heapid by the global thread index.
    _heapid = id % xdefines::NUM_HEAPS;
  }

  static inline void *malloc(size_t sz) {
    void * ptr = _pheap.malloc(_heapid, sz);
    return ptr;
  }

  static inline void * realloc(void * ptr, size_t sz) {
    size_t s = getSize(ptr);
    void * newptr = malloc(sz);
    if (newptr) {
      size_t copySz = (s < sz) ? s : sz;
      memcpy(newptr, ptr, copySz);
    }
    free(ptr);
    return newptr;
  }

  static inline void free(void * ptr) {
    return _pheap.free(_heapid, ptr);
  }

  /// @return the allocated size of a dynamically-allocated object.
  static inline size_t getSize(void * ptr) {
    // Just pass the pointer along to the heap.
    return _pheap.getSize(ptr);
  }

  static void openProtection(void) {
    _globals.openProtection(NULL);
    _pheap.openProtection(_pheap.getend());
  }

  static void closeProtection(void) {
    _globals.closeProtection();
    _pheap.closeProtection();
  }

  static inline void begin(bool cleanup) {
    // Reset global and heap protection.
    _globals.begin(cleanup);
    _pheap.begin(cleanup);
  }

  static void mem_write(void * dest, void *val) {
    if(_pheap.inRange(dest)) {
      _pheap.mem_write(dest, val);
    } else if(_globals.inRange(dest)) {
      _globals.mem_write(dest, val);
    }
  }

  static inline void handleWrite(void * addr) {
    if (_pheap.inRange(addr)) {
      _pheap.handleWrite(addr);
    } else if (_globals.inRange(addr)) {
      _globals.handleWrite(addr);
    }
    else {
      // None of the above - something is wrong.
      fprintf(stderr, "%d: wrong faulted address\n", getpid()); 
      assert(0);
    }
  }

  static inline void commit(bool update) {
    _pheap.checkandcommit(update);
    _globals.checkandcommit(update);
  }

#ifdef LAZY_COMMIT
  static inline void forceCommit(int pid) {
    _pheap.forceCommit(pid, _pheap.getend());
  }

  // Since globals will not owned by one thread, there is no need
  // to cleanup and commit.
  static inline void cleanupOwnedBlocks(void) {
    _pheap.cleanupOwnedBlocks();
  }
  
  static inline void commitOwnedPage(int page_no, bool set_shared) {
    _pheap.commitOwnedPage(page_no, set_shared);
  }

  // Commit every page owned by me. 
  static inline void finalcommit(bool release) {
    _pheap.finalcommit(release);
  }

#endif

public:

  /* Signal-related functions for tracking page accesses. */

  /// @brief Signal handler to trap SEGVs.
  static void segvHandle(int signum, siginfo_t * siginfo, void * context) {
    void * addr = siginfo->si_addr; // address of access

    // Check if this was a SEGV that we are supposed to trap.
    if (siginfo->si_code == SEGV_ACCERR) {
      xmemory::handleWrite(addr);
    } else if (siginfo->si_code == SEGV_MAPERR) {
      fprintf (stderr, "%d : map error with addr %p!\n", getpid(), addr);
      ::abort();
    } else {
      fprintf (stderr, "%d : other access error with addr %p.\n", getpid(), addr);
      ::abort();
    }
  }

#ifdef LAZY_COMMIT
  static void signalHandler(int signum, siginfo_t * siginfo, void * context) {
    union sigval signal = siginfo->si_value;
    int page_no;
    
    page_no = signal.sival_int;
    xmemory::commitOwnedPage(page_no, true);
  }
#endif

  /// @brief Install a handler for SEGV signals.
  static void installSignalHandler(void) {
#if defined(linux)
    // Set up an alternate signal stack.
    _sigstk.ss_sp = mmap(NULL, SIGSTKSZ, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON, -1, 0);
    _sigstk.ss_size = SIGSTKSZ;
    _sigstk.ss_flags = 0;
    sigaltstack(&_sigstk, (stack_t *) 0);
#endif
    // Now set up a signal handler for SIGSEGV events.
    struct sigaction siga;
    sigemptyset(&siga.sa_mask);

    // Set the following signals to a set
    sigaddset(&siga.sa_mask, SIGSEGV);
    sigaddset(&siga.sa_mask, SIGALRM);
#ifdef LAZY_COMMIT
    sigaddset(&siga.sa_mask, SIGUSR1);
#endif
    sigprocmask(SIG_BLOCK, &siga.sa_mask, NULL);

    // Point to the handler function.
#if defined(linux)
    siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_NODEFER;
#else
    siga.sa_flags = SA_SIGINFO | SA_RESTART;
#endif

    siga.sa_sigaction = xmemory::segvHandle;
    if (sigaction(SIGSEGV, &siga, NULL) == -1) {
      perror("sigaction(SIGSEGV)");
      exit(-1);
    }

#ifdef LAZY_COMMIT
    // We set the signal handler
    siga.sa_sigaction = xmemory::signalHandler;
    if (sigaction (SIGUSR1, &siga, NULL) == -1) {
      perror("sigaction(SIGUSR1)");
      exit(-1);
    }
#endif

    sigprocmask(SIG_UNBLOCK, &siga.sa_mask, NULL);
  }
};

#endif
