#ifndef __DETERM_H__
#define __DETERM_H__

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
 * @file   determ.h
 * @brief  Main file for determinism management.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie>
 */

#include <map>

#if !defined(_WIN32)
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include "xdefines.h"
#include "list.h"
#include "xbitmap.h"
#include "xdefines.h"
#include "internalheap.h"
#include "real.h"
#include "prof.h"

#define MAX_THREADS 2048
//#define fprintf(...) 

// We are using a circular double linklist to manage those alive threads.
class determ {
private:
   
  // Different status of one thread.
  enum threadStatus {
    STATUS_COND_WAITING = 0, STATUS_BARR_WAITING, STATUS_READY, STATUS_EXIT, STATUS_JOINING
  };

  // Each thread has a thread entry in the system, it is used to control the thread. 
  // For example, when one thread is cond_wait, the corresponding thread entry will be taken out of the token
  // queue and putted into corresponding conditional variable queue.
  class ThreadEntry {
  public:
    inline ThreadEntry() {

    }

    inline ThreadEntry(int tid, int threadindex) {
      this->tid = tid;
      this->threadindex = threadindex;
      this->wait = 0;
    }

    Entry * prev;
    Entry * next;
    volatile int tid; // pid of this thread.
    volatile int threadindex; // thread index 
    volatile int status;
    int tid_parent; // parent's pid
    void * cond; 
    void * barrier;
    size_t wait;
    int joinee_thread_index;
  };

  class LockEntry {
    public:
      // How many users of this lock. When the total_uses is larger than 2,
      // the lock is thought to be shared and it will stop updating last_thread.
      volatile int total_users;

      // last thread to use this lock.
      volatile int last_thread;

      // Status of lock, aquired or not.
      volatile bool is_acquired;

      volatile int lock_budget;
  };

  // condition variable entry
  class CondEntry {
  public:
    size_t waiters; // How many waiters on this cond.
    void * cond;    // original cond address
    pthread_cond_t realcond;
    Entry * head;   // pointing to the waiting queue
  };

  // barrier entry
  class BarrierEntry {
  public:
    volatile size_t maxthreads;
    volatile size_t threads;
    volatile bool arrival_phase;
    void * orig_barr;
    pthread_barrier_t real_barr;
    Entry * head;
  };

  // Shared mutex and condition variable for all threads.
  // They are useful to synchronize among all threads.
  pthread_mutex_t _mutex;
  pthread_cond_t cond;
  pthread_condattr_t _condattr;
  pthread_mutexattr_t _mutexattr;

  // When one thread is created, it will wait until all threads are created.
  // The following two flag are used to indentify whether one thread can move on or not.
  volatile bool _childregistered;
  volatile bool _parentnotified;

  // Some conditional variable used for thread creation and joining.
  pthread_cond_t _cond_children;
  pthread_cond_t _cond_parent;
  pthread_cond_t _cond_join;

  // All threads should be putted into this active list.
  Entry *_activelist;

  // Currently, we can support how many threads.
  // When one thread is exited, the thread index can be reclaimed.
  ThreadEntry _entries[2048];

  // how much active thread in the system.
  size_t _maxthreadentries;

  // How many conditional variables in this system.
  // In fact, maybe we don't need this.
  size_t _condnum;
  size_t _barriernum;

  size_t _coresNumb;
  
  // Variables related to token pass and fence control
  volatile ThreadEntry *_tokenpos;
  volatile size_t _maxthreads;
  volatile size_t _currthreads;
  volatile bool _is_arrival_phase;
  volatile size_t _alivethreads;

  determ():
    _condnum(0),
    _barriernum(0),
    _maxthreads(0),
    _currthreads(0),
    _is_arrival_phase(false),
    _alivethreads(0),
    _maxthreadentries(MAX_THREADS),
    _activelist(NULL),
    _tokenpos(NULL), 
    _parentnotified(false), 
    _childregistered(false) 
    {  }

public:

  void initialize(void) {
    // Get cores number
    _coresNumb = sysconf(_SC_NPROCESSORS_ONLN);
    if(_coresNumb < 1) {
      fprintf(stderr, "cores number isnot correct. Exit now.\n");
      exit(-1);
    }
    
    // Set up with a shared attribute.
    WRAP(pthread_mutexattr_init)(&_mutexattr);
    pthread_mutexattr_setpshared(&_mutexattr, PTHREAD_PROCESS_SHARED);
    WRAP(pthread_condattr_init)(&_condattr);
    pthread_condattr_setpshared(&_condattr, PTHREAD_PROCESS_SHARED);

    // Initialize the mutex.
    WRAP(pthread_mutex_init)(&_mutex, &_mutexattr);
    WRAP(pthread_cond_init)(&cond, &_condattr);
    WRAP(pthread_cond_init)(&_cond_parent, &_condattr);
    WRAP(pthread_cond_init)(&_cond_children, &_condattr);
    WRAP(pthread_cond_init)(&_cond_join, &_condattr);
  }

  static determ& getInstance(void) {
    static determ * determObject = NULL;
    if(!determObject) {
      void *buf = mmap(NULL, sizeof(determ), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      determObject = new(buf) determ();
    }
    return * determObject;
  }

  void finalize(void) {
    WRAP(pthread_mutex_destroy)(&_mutex);
    WRAP(pthread_cond_destroy)(&cond);
    assert(_currthreads == 0);
  }

  // Increment the fence when all threads has been created by current thread.
  void startFence(int threads) {
    lock();
    _maxthreads += threads;
    _alivethreads += threads;
    _is_arrival_phase = true;

    // Because all threads are waiting when one thread is spawning, 
    // Now time to wake up them.
    WRAP(pthread_cond_broadcast)(&cond);
    unlock();
  }

  // Increase the fence, not need to hold lock!
  void incrFence(int num) {
    _maxthreads += num;
  }

  // Decrease the fence when one thread exits.
  // We assume that the gobal lock is held now.
  void decrFence(void) {
    _maxthreads--;
    if (_currthreads >= _maxthreads) {
      // Change phase if necessary
      if (_is_arrival_phase && _maxthreads != 0) {
        _is_arrival_phase = false;
        __asm__ __volatile__ ("mfence");
      }
      WRAP(pthread_cond_broadcast)(&cond);
    }
  }

  // pthread_cancel implementation, we are relying threadindex to find corresponding entry.
  bool cancel(int threadindex) {
    ThreadEntry * entry;
    bool isFound = false;

    entry = (ThreadEntry *) &_entries[threadindex];

    // Checking corresponding status.
    switch (entry->status) {
    case STATUS_EXIT:
      // If the thread has exited, do nothing.
      isFound = false;
      break;

    case STATUS_COND_WAITING: 
      // If the thread is waiting on condition variable, remove it from corresponding list. 
      {
      CondEntry * condentry = (CondEntry *) entry->cond;
      removeEntry((Entry *) entry, &condentry->head);
      assert(condentry->waiters == 0 || condentry->head != NULL);
      isFound = true;
      }
      break;

    case STATUS_BARR_WAITING: 
    default:
      // In fact, this case is almost impossible. But just in case, we put code here.
      assert(0);
      isFound = false;
      break;
    }

    if (isFound) {
      // Adjust the fence and active threads number.
      _alivethreads--;
      if (entry->wait == 1) {
        _currthreads--;
        _maxthreads--;
      }
      if (_maxthreads == 1) {
        _is_arrival_phase = true;
      }

      __asm__ __volatile__ ("mfence");
      freeThreadEntry(entry);
    }

    // If we can't find the entry, that means this thread has exited successfully.
    // Then we don't need to do anything.
    return isFound;
  }

  // There is only one thread which are doing those synchronizations.
  // Hence, there is no need to do those transaction start and end operations.
  bool isSingleWorkingThread(void) {
    return (_maxthreads == 1);
  }

  // All threads are exited now, there is only one thread left in the system.
  bool isSingleAliveThread(void) {
    return (_maxthreads == 1 && _alivethreads == 1);
  }

  // main function of waitFence and waitToken
  // Here, we are defining two different paths.
  // If the alive threads is smaller than the coresNumb, then we don't
  // use those condwait, but busy waiting instead.
  void waitFence(int threadindex, bool keepBitmap) {
    int rv;
    bool lastThread = false;

    ThreadEntry * entry = &_entries[threadindex];

    lock();

    // Check whether all threads has passed previous arrival phase.
    if(_maxthreads <= _coresNumb && _is_arrival_phase != 1) {
      unlock();
      while(_is_arrival_phase != true) { }
      lock();
    } else {
      while(_is_arrival_phase != true) {
        rv = WRAP(pthread_cond_wait)(&cond, &_mutex);
        if(rv != 0) {
          fprintf(stderr, "waitFence problem, can't issue condwait sucessfully\n");
          unlock();
          return;
        }
      }
    }

    // Now in an arrival phase, proceed with barrier synchronization
    _currthreads++;
   
    // Whenever all threads arrived in the barrier, wakeup everyone on the barrier. 
    if(_maxthreads <= _coresNumb) {
      if(_currthreads >= _maxthreads) {
        _is_arrival_phase = false;
        WRAP(pthread_cond_broadcast)(&cond);
      } else {
        unlock();
        entry->wait = 1;
        while(_is_arrival_phase == true) { }
        entry->wait = 0;
        lock();
      }
    } else {
      if(_currthreads >= _maxthreads) {
        _is_arrival_phase = false;
        WRAP(pthread_cond_broadcast)(&cond);
      } else {
        while(_is_arrival_phase == 1) {
          entry->wait = 1;
          WRAP(pthread_cond_wait)(&cond, &_mutex);
        }
      }
    }
   
    // Mark one thread is leaving the barrier. 
    _currthreads--;
  
    // When all threads leave the barrier, entering into the new arrival phase.  
    if (_currthreads == 0) {
      _is_arrival_phase = true;

      // Cleanup the bitmap here.
      if(!keepBitmap)
        xbitmap::getInstance().cleanup();

      WRAP(pthread_cond_broadcast)(&cond);
    }

    unlock();

    return;
  }
  
  void getToken(void) {
    while (_tokenpos->tid != getpid()) {
      sched_yield();
      __asm__ __volatile__ ("mfence");
    }
    DEBUG("%d: Got token after waitFence", _tokenpos->threadindex);
    PRINT_SCHEDULE("%d: Got token after waitFence", _tokenpos->threadindex);
    START_TIMER(serial);
    return;
  }

  void putToken(int threadindex) {
    ThreadEntry * next;
    STOP_TIMER(serial);

//    fprintf(stderr, "%d : putToken \n", threadindex);
    lock();

    // Sanity check, whether I have to right to call putToken.
    // Only token owner can put token.
    if (threadindex != _tokenpos->threadindex) {
      unlock();
      fprintf(stderr, "%d : ERROR to putToken, pointing to pid %d index %d, while my index %d\n", getpid(), _tokenpos->tid, _tokenpos->threadindex, threadindex);
      assert(0);
    }
    next = (ThreadEntry *)(_tokenpos->next);

    if(next != NULL) {
      DEBUG("%d : thread %d put token. Now token is passed to thread %d\n", getpid(), threadindex, next->threadindex);
      PRINT_SCHEDULE("thread %d put token and pass token to thread %d\n", threadindex, next->threadindex);
    }
    if(next != NULL) {
      _tokenpos = next;
    }
    unlock();
  }

  // No need lock since the register is done before any spawning.
  void registerMaster(int threadindex, int pid) {
    registerThread(threadindex, pid, 0);
    
    _maxthreads = 1;
    _alivethreads = 1;
    _is_arrival_phase = true; 
  }

  // Add this thread to the list.
  // When one thread is registered, no one else is running,
  // there is no need to hold the lock.
  void registerThread(int threadindex, int pid, int parentindex) {
    ThreadEntry * entry;

    // Allocate memory to hold corresponding information.
    void * ptr = allocThreadEntry(threadindex);

    // Here, header is not one node in the circular link list.
    // _activelist->next is the first node in the link list, while _activelist->prev is
    // the last node in the link list.
    entry = new (ptr) ThreadEntry(pid, threadindex);

    // fprintf(stderr, "%d: with threadindex %d\n", getpid(), threadindex);

    // Record the parent's thread index.
    entry->tid_parent = parentindex;

    // Add this thread to the list.
    if (_tokenpos == NULL) {
      _tokenpos = entry;
    }

    entry->status = STATUS_READY;

    // Add one entry according to their threadindex.
    insertTail((Entry *)entry, &_activelist);
  }

  // Those children are waiting on _cond_children when the parent is still
  // spawning new threads in order to guarantee the determinism.
  inline void waitParentNotify(void) {
    lock();

    _childregistered = true;
    WRAP(pthread_cond_signal)(&_cond_parent);

    WRAP(pthread_cond_wait)(&_cond_children, &_mutex);
    unlock();
  }

  // Parent should call this function. 
  inline void waitChildRegistered(void) {
    lock();
    if (!_childregistered) {
      WRAP(pthread_cond_wait)(&_cond_parent, &_mutex);
      if(!_childregistered) {
        fprintf(stderr, "Child should be registered!!!!\n");
      }
    }
    _childregistered = false;
    unlock();
  }

  // Now wakeup all threads waiting on _cond_children when 
  // parent finishs the spawning. In fact, parent met some synchronizations points now.
  inline void notifyWaitingChildren(void) {
    lock();
    WRAP(pthread_cond_broadcast)(&_cond_children);
    unlock();
  }

  // Deterministic pthread_join
  inline bool join(int guestindex, int myindex, bool wakeup) {
    // Check whether I am holding the lock or not.
    assert (myindex == _tokenpos->threadindex);

    ThreadEntry * joinee;
    ThreadEntry * myentry;
    bool toWaitToken = false;

    lock();

    // Get next entry.
    myentry = (ThreadEntry *)&_entries[myindex];
    joinee = (ThreadEntry *)&_entries[guestindex];

    // If join is the first synchronization point after spawning, 
    // wakeup all children waiting for the parent's notification.
    if(wakeup) {
      WRAP(pthread_cond_broadcast)(&_cond_children);
    }
  
    // When the joinee is still alive, we should wait for the joinee to wake me up 
    if(joinee->status != STATUS_EXIT) {
      // Remove myself from the token queue.
      removeEntry((Entry *)myentry, &_activelist);
      
      // Set my status to joinning.
      myentry->status = STATUS_JOINING;
      myentry->joinee_thread_index = guestindex;
    }
  
    
    while(joinee->status != STATUS_EXIT) {    
      decrFence();

      // Pass the token to next thread if I am holding the token.
      if(_tokenpos->threadindex == myindex && _activelist != NULL) {
        _tokenpos = (ThreadEntry *) (_tokenpos->next);
      } 
    
      // Waiting for the children's exit now.
      WRAP(pthread_cond_wait)(&_cond_join, &_mutex);

      // When the parent is waken, it should get token immediately then it could 
      // put token later. For simplicity, all pthread_join should hold the token.
      toWaitToken = true;
      
      // Increase total threads since current threads is waken.
      // Whenever this thread cannot run, ti will decrease fence immediately.
      incrFence(1);
    }
  
    // Cleanup the status.
    myentry->status = STATUS_READY;
  
    DEBUG("%d: pthread_join, pass token to %d before unlock\n", _tokenpos->threadindex);
    PRINT_SCHEDULE("%d: pthread_join, pass token to %d before unlock\n", _tokenpos->threadindex);

    unlock(); 
       
    if(toWaitToken) {
      // Wait for the token. 
          while(_tokenpos->threadindex != myindex) {
              sched_yield();
              __asm__ __volatile__ ("mfence");
          }

          START_TIMER(serial);
    }

    return toWaitToken;
  }


  void deregisterThread(int threadindex) {
    ThreadEntry * entry = &_entries[threadindex];
    ThreadEntry * parent = &_entries[entry->tid_parent];
    ThreadEntry * nextentry;

    lock();
    DEBUG("%d: Deregistering", getpid());

    // Whether the parent is trying to join current thread now??
    if(parent->status == STATUS_JOINING && parent->joinee_thread_index == threadindex) {
      // Move parent to the next, so that the parent can get the token immediately.
      // Otherwise, if the token is passed to one thread (already finished the token), 
      // then the waken up thread cannot move on since it will wait for the 
      // token to move on, however, the thread cannot pass the token and it is waiting for fence now.
          insertHead((Entry *)parent, (Entry **)&_tokenpos);

      // Waken up all threads waiting on _cond_join, but only the one waiting on this thread
      // can be waken up. Other thread will goto sleep again immediately.
      WRAP(pthread_cond_broadcast)(&_cond_join);
    } 

    // Decrease number of alive threads and fence.
    if(_alivethreads > 0) {
      // Since this thread is running with the token, no need to modify 
      // _currthreads.
           _alivethreads--;
           decrFence();
        }

    nextentry = (ThreadEntry *)entry->next;
    assert(nextentry != entry);

    // Remove this thread entry from activelist.  
    removeEntry((Entry *) entry, &_activelist);
    freeThreadEntry(entry);

    // Passing the token to next thread in the activelist.
    // It is almost impossible that nextentry will be NULL, that means that
    // no one is active.
        _tokenpos = nextentry;
    
    DEBUG("%d: deregistering. Token is passed to %d\n", getpid(), (ThreadEntry *)_tokenpos->threadindex);
    PRINT_SCHEDULE("%d: deregistering. Token is passed to %d\n", threadindex, (ThreadEntry *)_tokenpos->threadindex);
    
    unlock();
  }

  LockEntry * lock_init(void * mutex) {
//    fprintf(stderr, "%d: lockinit with mutex %p\n", getpid(), mutex);
    LockEntry * entry = allocLockEntry();
    entry->total_users = 0;
    entry->last_thread = 0;
    
    //No one acquire the lock in the beginning.
    entry->is_acquired = false;
    
    // No one is the owner.
    setSyncEntry(mutex, (void *)entry); 
    return entry;
  }

  void lock_destroy(void * mutex) {
    LockEntry * entry =(LockEntry*)getSyncEntry(mutex);
    clearSyncEntry(mutex);
    freeSyncEntry(entry);
  }

  // Only there is only one thread to use this lock,
  // function can return true. 
  // Since it is called without the token,
  // if no one uses this lock before, the caller cannot 
  // own the lock in order to guarantee the determinism. 
  inline bool lock_isowner(void * mutex) {
    //fprintf(stderr, "%d: lock_isowner\n", getpid());  
    LockEntry * entry = (LockEntry *)getSyncEntry(mutex);
    //fprintf(stderr, "%d: lock_isowner with entry %p\n", getpid(), entry); 

    if(entry == NULL)
      return false;
  
    //fprintf(stderr, "%d: lock_isowner with usrs %d\n", getpid(), entry->total_users); 
    if(entry->total_users == 1) {
      // If only one user uses this lock, check whether 
      // current user is the owner.
      int pid = getpid();
      if(entry->last_thread != pid) {
        entry->total_users++;
        return false;
      } 
      return true;
    }
    return false;
  }

    
  // Check whether lock is acquired.
  // Whenever the lock is owned by someone, we don't need to acquire the lock.
  inline bool lock_isacquired(void * mutex) {
    LockEntry * entry = (LockEntry *)getSyncEntry(mutex);
    assert(entry != NULL);
    return entry->is_acquired;
  }

  // The function is only called when the thread owns the token.
  // So it is safe to change status without the use the locks. 
  // The function is to avoid the problem caused by turning multiple locks 
  // into one big lock. The idea is that when one lock is not released,
  // next thread to acquire this should not move on.
  inline bool lock_acquire(void * mutex) {
    LockEntry * entry = (LockEntry *)getSyncEntry(mutex);
    bool result = true;

    if(entry == NULL) {
       // fprintf(stderr, "%d: lock acquire 3 with mutex %p, entry %p\n", getpid(), mutex, entry);  
      entry = lock_init(mutex);
    }
  
  //  fprintf(stderr, "%d: lock acquire, with last thread %d, total users %d, is_acquire %d\n", getpid(), entry->last_thread, entry->total_users, entry->is_acquired);  
    if(entry->is_acquired == true)  
      return false;

    entry->is_acquired = true;
    if(entry->total_users == 0) { 
      // Change the owner of this lock.
      entry->last_thread = getpid();
      entry->total_users = 1;
            entry->lock_budget = xdefines::LOCK_OWNER_BUDGET;
    }
    else if(entry->total_users == 1) {
      if(entry->last_thread != getpid()) {
        entry->total_users++;
      }
      else {
        --entry->lock_budget;
        if(entry->lock_budget == 0) {
                entry->lock_budget = xdefines::LOCK_OWNER_BUDGET;
          if(isSingleWorkingThread() != true) {
            result = false;
            // Sorry, if current owner has no budget, it cannot get
            // the lock now.
            entry->is_acquired = false;
          }
        }
      }
    }
  //  fprintf(stderr, "%d: lock acquire in the end, with last thread %d, total users %d and is_acquire %d\n", getpid(), entry->last_thread, entry->total_users, entry->is_acquired);  
    return result;
  }

  inline void lock_release(void * mutex) {
    LockEntry * entry = (LockEntry *)getSyncEntry(mutex);
    entry->is_acquired = false;
  }

  CondEntry * cond_init(void * cond) {
    CondEntry * entry = allocCondEntry();
    _condnum++;
    entry->waiters = 0;
    entry->head = NULL;
    entry->cond = cond;
  
  //  xmemory::begin(true);
    //Set corresponding entry.
    setSyncEntry(cond, entry);

    // Initialize the real conditional entry.
    WRAP(pthread_cond_init)(&entry->realcond, &_condattr);

    //xmemory::commit(false);
    assert(entry->waiters == 0 || entry->head != NULL);
    return entry;
  }

  void cond_destroy(void * cond) {
    CondEntry * entry;
    int offset;

    lock();
    _condnum--;
    entry = (CondEntry*)getSyncEntry(cond);
    clearSyncEntry(cond);
    freeSyncEntry(entry);
    unlock();
  }

  void cond_wait(int threadindex, void * cond, void * thelock) {
    ThreadEntry * entry = &_entries[threadindex];
    CondEntry * condentry = (CondEntry*)getSyncEntry(cond);
    ThreadEntry * next;
    if (condentry == NULL) {
      condentry = cond_init(cond);
    }

    lock();

    assert(_tokenpos == entry);

    // Get next entry.
    next = (ThreadEntry *) entry->next;

    // Remove this thread from activelist.
    removeEntry((Entry *) entry, &_activelist);

    // Add to the tail of corresponding cond list.
    insertTail((Entry *) entry, &condentry->head);

    // One more waiter for this condvar.
    condentry->waiters++;
    assert(condentry->waiters == 0 || condentry->head != NULL);
    // Set current thread status.
    entry->cond = condentry;
    entry->status = STATUS_COND_WAITING;

    decrFence();

    // Release token to next active thread.
    _tokenpos = next;

    // Wait until it is signaled (status are changed to STATUS_READY)
    // We are using busy wait method to avoid un-determinism caused by OS.
    // Linux cann't guarantee the FIFO order.
    while (entry->status != STATUS_READY) {
      __asm__ __volatile__ ("mfence");
      
      // Release current lock.
      lock_release(thelock);
      WRAP(pthread_cond_wait)(&condentry->realcond, &_mutex);
    }
    //fprintf(stderr, "%d: cond_wait after wakingup\n", getpid());

    // Here, we don't need to wait on fence anymore because this can put current
    // thread to next round, which is really bad.
    // We just need check whether all threads are inside the critical area or not.
    // That is, no one is outside the critical area. Since we 
    // have the token to control the running inside the criical area.
    unlock();

    // Check the token. 
    while (_tokenpos->threadindex != threadindex) {
      sched_yield();
      __asm__ __volatile__ ("mfence");
    }
    
  //  fprintf(stderr, "%d: cond_wait after getting token\n", getpid());
      
    // Now means we have already acquired the lock.
    lock_acquire(thelock);

    START_TIMER(serial);
  }

  // Current thread are going to send out signal.
  void cond_signal(void * cond) {
    CondEntry * condentry = (CondEntry*)getSyncEntry(cond);

    //fprintf(stderr, "%d: cond_signal cond %p\n", getpid(), condentry);
    if(condentry == NULL) {
      condentry = cond_init(cond);
    }

    // No need to wakeup if no one is waiting.
    if (condentry->waiters == 0)
      return;

    lock();

    // Remove the head entry in cond variable.
    ThreadEntry * entry = (ThreadEntry *) removeHeadEntry(&condentry->head);
    assert(entry != NULL);

    // It is important to add the thread to the next.
    // If the thread wakenup is added to the tail, then the thread cannot get token before all other threads
    // can get token. If one thread A before current thread has already passed the token and are waiting on the fence
    // then both thread A and current thread cannot move on.
    insertHead((Entry *) entry, (Entry **)&_tokenpos);

    // Set the status to ready so that the waiting thread can move on.
    entry->cond = NULL;
    entry->status = STATUS_READY;

    // We can increase the fence.
    incrFence(1);

    // One less waiters for this condentry.
    condentry->waiters--;

    // Wakeup all waiters on this condentry.
    WRAP(pthread_cond_broadcast)(&condentry->realcond);

    unlock();
  }

  void cond_broadcast(void * cond) {
    CondEntry * condentry = (CondEntry*)getSyncEntry(cond);

    if(condentry == NULL) {
      condentry = cond_init(cond);
    }

    assert(condentry != NULL);

    // No need to wakeup if no one is waiting.
    if (condentry->waiters == 0)
      return;

    int waiters = condentry->waiters;

    lock();

    ThreadEntry * entry = (ThreadEntry *) condentry->head;
    // Set status for these threads.
    while (waiters-- != 0) {
      // Set the status to ready.
      entry->cond = NULL;
      entry->status = STATUS_READY;
      entry = (ThreadEntry *) entry->next;
    }

    // Move the whole list to runnable queue.
    moveWholeList((Entry *)_tokenpos, &condentry->head);

    // Increment the fence
    incrFence(condentry->waiters);

    // Now no waiters in this cond var. 
    condentry->waiters = 0;

    // Wakeup all waiters on this condentry.
    WRAP(pthread_cond_broadcast)(&condentry->realcond);

    unlock();
  }
    
  int sig_wait(const sigset_t *set, int *sig, int threadindex) {
    ThreadEntry * entry = &_entries[threadindex];
    ThreadEntry * next;
    int ret;
    
    lock();
    
    // Get next entry.
    next = (ThreadEntry *)entry->next;
    
    // Remove this thread from activelist.
    removeEntry((Entry *)entry, &_activelist);
    
    decrFence();
    
    // Release token to next active thread.
    _tokenpos = next;
    
    unlock();
    
    ret = WRAP(sigwait)(set, sig);
  
    if(ret != 0) {
      return ret;
    } 

    // Now I am waken up because I need to handle those signals now.  
    lock();
  
    if(_tokenpos == NULL) {
      _tokenpos = entry;
    }
    else {
      // IMPORTANT: Add it to next of activelist, we still honor the previous existing order.
      insertHead((Entry *)entry, (Entry **)&_tokenpos);
    }
  
    // Increment the fence.
    incrFence(1);
    
    unlock();
  
    return 0;
  }

  // Functions related to barrier.
  void barrier_init(void * bar, int count) {
    BarrierEntry * entry = allocBarrierEntry();
    pthread_barrierattr_t attr;

    if (entry == NULL) {
      assert(0);
    }

    entry->maxthreads = count;
    entry->threads = 0;
    entry->arrival_phase = true;
    entry->orig_barr = bar;
    entry->head = NULL;

    // Set up with a shared attribute.
    pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    WRAP(pthread_barrier_init)(&entry->real_barr, &attr, count);

    //Set corresponding entry.
    setSyncEntry(bar, entry);

    _barriernum++;
  }

  // Here, we are using a different mechanism with cond_wait.
  void barrier_wait(void * bar, int threadindex) {
    BarrierEntry * barentry;
    bool lastThread = false;

    barentry = (BarrierEntry*)getSyncEntry(bar);
    assert(barentry != NULL);

    ThreadEntry * entry = &_entries[threadindex];
    pthread_mutex_t * mutex = &_mutex;
    volatile size_t * threads = &barentry->threads;
    volatile size_t * maxthreads = &barentry->maxthreads;
    // Get next entry.
    ThreadEntry * nextentry;

    WRAP(pthread_mutex_lock)(mutex);

    // Check whether I am holding the token. If not, wrong!
    assert(_tokenpos == entry);

    (*threads)++;
    // Whether I am the last one to enter into barrier?
    if (*threads == *maxthreads) {
      // Then we don't to remove this thread from activelist
      // In stead, we should move the whole list back to _activelist.
      moveWholeList((Entry *) _tokenpos, &barentry->head);

      // Increment the fence.
      incrFence(*maxthreads - 1);

      // since normally I am the only one in the token ring,
      // we don't want to pass the token backto myself for fairness.
      // So we will get the next entry here but not before the moveWholeList.
      nextentry = (ThreadEntry *) entry->next;
      lastThread = true;
      (*threads) = 0;
      xbitmap::getInstance().cleanup();
    } else {
      // Get next entry in the "token" ring.
      nextentry = (ThreadEntry *) entry->next;

      // Remove this thread from activelist.
      removeEntry((Entry *) entry, &_activelist);

      // Add to the tail of corresponding barrier list.
      insertTail((Entry *) entry, &barentry->head);

      // Now we are waiting on the barrier.
      entry->status = STATUS_BARR_WAITING;
      entry->barrier = barentry;
      decrFence();
    }

    // Release token to next active thread.
    _tokenpos = nextentry;
  
    STOP_TIMER(serial);

    unlock();

    // If I am not the last thread to enter the barrier,
    // Then I should let others to get the lock (in order to do other stuff).
    // If I am the last thread, don't do cleanup until after barrier.
    if (!lastThread) {
      // We can do the atomicCommit safely. 
      // First, The token has been passed to others
      // Second, I am not in the token ring (can't be passed the token).
      // Then it won't cause deadlock anymore.
      xmemory::begin(true);
    }
    WRAP(pthread_barrier_wait)(&barentry->real_barr);
    if (lastThread)
      xmemory::begin(true);

  }

  void barrier_destroy(void * bar) {
    BarrierEntry * entry;
    int offset;

    entry = (BarrierEntry*)getSyncEntry(bar);
    freeSyncEntry(entry);
    clearSyncEntry(bar);

    _barriernum--;
  }

private:
  inline void * allocThreadEntry(int threadindex) {
    assert(threadindex < _maxthreadentries);
    return (&_entries[threadindex]);
  }

  inline void freeThreadEntry(void *ptr) {
    ThreadEntry * entry = (ThreadEntry *) ptr;
    entry->status = STATUS_EXIT;
    // Do nothing now.
    return;
  }
  
  inline void * allocSyncEntry(int size) {
    return InternalHeap::getInstance().malloc(size);
  }

  inline void freeSyncEntry(void * ptr) {
    if (ptr != NULL) {
      InternalHeap::getInstance().free(ptr);
    }
  }

  inline LockEntry *allocLockEntry(void) {
    //fprintf(stderr, "%d: alloc lock entry with size %d\n", getpid(), sizeof(LockEntry));  
    return ((LockEntry *) allocSyncEntry(sizeof(LockEntry)));
  }

  inline CondEntry *allocCondEntry(void) {
    return ((CondEntry *) allocSyncEntry(sizeof(CondEntry)));
  }

  inline BarrierEntry *allocBarrierEntry(void) {
    return ((BarrierEntry *) allocSyncEntry(sizeof(BarrierEntry)));
  }

  void * getSyncEntry(void * entry) {
    void ** ptr = (void **)entry;
//    fprintf(stderr, "%d: entry %p and synentry 0x%x\n", getpid(), entry, ((int *)entry));   
    return(*ptr);
  }

  // Here, we set the first word of cond to entry to avoid the search on cond.
  // First, it is faster to avoid the search on cond if multiple cond.
  // Second, it is safe to do so, since no one will change the entry until it is destroyed.
  void setSyncEntry(void * origentry, void * newentry) {
    void **dest = (void**)origentry;

    *dest = newentry;

    //fprintf(stderr, "origentry %p dest %p *dest %p newentry %p\n", origentry, dest, *dest, newentry);
    // Update the shared copy in the same time. 
    xmemory::mem_write(dest, newentry);
  }

  void clearSyncEntry(void * origentry) {
    void **dest = (void**)origentry;

    *dest = NULL;

    // Update the shared copy in the same time. 
    xmemory::mem_write(*dest, NULL);
  }
  inline void lock(void) {
    WRAP(pthread_mutex_lock)(&_mutex);
  }

  inline void unlock(void) {
    WRAP(pthread_mutex_unlock)(&_mutex);
  }
};

#endif
