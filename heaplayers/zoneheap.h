/* -*- C++ -*- */

/**
 * @class ZoneHeap
 * @brief A zone (or arena, or region) based allocator.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @date June 2000, May 2008
 *
 * Uses the superclass to obtain large chunks of memory that are only
 * returned when the heap itself is destroyed.
 *
*/

#ifndef _ZONEHEAP_H_
#define _ZONEHEAP_H_

#include <assert.h>

namespace HL {

  template <class Super, size_t ChunkSize>
  class ZoneHeap : public Super {
  public:

    ZoneHeap (void)
      : _currentArena (NULL),
	_pastArenas (NULL)
    {}

    ~ZoneHeap (void)
    {
      // Delete all of our arenas.
      
      Arena * ptr = _pastArenas;
      while (ptr != NULL) {
	void * oldPtr = (void *) ptr;
	ptr = ptr->getNextArena();
	//printf ("deleting %x\n", ptr);
	Super::free (oldPtr);
      }
      if (_currentArena != NULL)
	//printf ("deleting %x\n", currentArena);
	Super::free (_currentArena);
    }

    inline void * malloc (size_t sz) {
      void * ptr = zoneMalloc (sz);
      return ptr;
    }

    /// Free in a zone allocator is a no-op.
    inline void free (void *) {}

    /// Remove in a zone allocator is a no-op.
    inline int remove (void *) { return 0; }


  private:

    inline static size_t align (int sz) {
      return (sz + (sizeof(double) - 1)) & ~(sizeof(double) - 1);
    }

    inline void * zoneMalloc (size_t sz) {
      // Round up size to an aligned value.
      sz = align (sz);
      if (_currentArena && (_currentArena->sizeRemaining() >= sz)) {
	void * ptr = _currentArena->malloc(sz);
	return ptr;
      }
      return slowMalloc (sz);
    }

    void * slowMalloc (size_t sz) {
      // Get more space in our arena since there's not enough room in
      // this one.
      assert ((_currentArena == NULL) ||
	      (_currentArena->sizeRemaining() < sz));

      // First, add this arena to our past arena list.
      if (_currentArena != NULL) {
	_currentArena->setNextArena (_pastArenas);
	_pastArenas = _currentArena;
	_currentArena = NULL;
      }

      // Now get more memory.
      size_t allocSize = ChunkSize;
      if (allocSize < sz) {
	allocSize = sz;
      }
      void * buf = Super::malloc (sizeof(Arena) + allocSize);
      if (buf == NULL) {
	return NULL;
      }
      _currentArena = new (buf) Arena (allocSize);

      void * ptr = _currentArena->malloc (sz);
      return ptr;
    }

  
    class Arena {
    public:
      Arena (size_t sz)
	: _remaining (sz),
	  _nextArena (NULL),
	  _arenaSpace ((char *) (this + 1))
      {}
      inline void * malloc (size_t sz) {
	if (sz > _remaining) {
	  return NULL;
	}
	// Bump the pointer and update the amount of memory remaining.
	_remaining -= sz;
	void * ptr = (void *) _arenaSpace;
	_arenaSpace += sz;
	return ptr;
      }
      size_t  sizeRemaining (void) const { return _remaining; }
      void    setNextArena (Arena * n) { _nextArena = n; }
      Arena * getNextArena (void) const { return _nextArena; }

    private:
      size_t _remaining;
      Arena * _nextArena;
      union {
	char * _arenaSpace;
	double _dummy; // For alignment.
      };
    };
    
    /// The current arena.
    Arena * _currentArena;

    /// A linked list of past arenas.
    Arena * _pastArenas;
  };

}

#endif
