// -*- C++ -*-

#ifndef _XTHREAD_H_
#define _XTHREAD_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xdefines.h"

// Heap Layers

#include "heaplayers/freelistheap.h"
#include "heaplayers/mmapheap.h"
#include "heaplayers/util/cpuinfo.h"

extern "C" {
// The type of a pthread function.
typedef void * threadFunction(void *);
}

class xrun;

class xthread {
private:

	/// @class ThreadStatus
	/// @brief Holds the thread id and the return value.
	class ThreadStatus {
	public:
		ThreadStatus(void * r, bool f) : retval(r), forked(f) {}

		ThreadStatus(void) {}

		volatile int tid;

		int threadIndex;
		void * retval;

		bool forked;
	};

	/// Current nesting level (i.e., how deep we are in recursive threads).
	static unsigned int _nestingLevel;

	/// What is this thread's PID?
	static int _tid;

public:

	//xthread(void) : _nestingLevel(0), _protected(false) {}

	static void * spawn(threadFunction * fn, void * arg, int threadindex);

	static void join(void * v, void ** result);

	static int cancel(void * v);
	static int thread_kill(void * v, int sig);

	// The following functions are trying to get id or thread index of specified thread.
	static int getThreadIndex(void *v); 
	static int getThreadPid(void *v); 

	// getId is trying to get id for current thread, the function 
	// is called by current thread.
	static inline int getId(void) {
		return _tid;
	}

	static inline void setId(int id) {
		_tid = id;
	}

private:

	static void * forkSpawn(threadFunction * fn, ThreadStatus * t, void * arg, int threadindex);

	static void run_thread(threadFunction * fn, ThreadStatus * t, void * arg);

	/// @return a chunk of memory shared across processes.
	static void * allocateSharedObject(size_t sz) {
		return mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	}

	static void freeSharedObject(void * ptr, size_t sz) {
		munmap(ptr, sz);
	}
};

#endif
