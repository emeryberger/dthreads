// -*- C++ -*-

/*

  Heap Layers: An Extensible Memory Allocation Infrastructure
  
  Copyright (C) 2000-2003 by Emery Berger
  http://www.cs.umass.edu/~emery
  emery@cs.umass.edu
  
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

#ifndef _DLLIST_H_
#define _DLLIST_H_

/**
 *
 * @class DLList
 * @brief A "memory neutral" doubly-linked list.
 * @author Emery Berger
 */

#include <assert.h>
#include <stdlib.h>

#if 1

namespace HL {

class DLList {
public:

  DLList (void)
    : _head (NULL)
  {}

  void clear (void) {
    _head = NULL;
  }

  bool isEmpty (void) const {
    return (_head == NULL);
  }

  class Entry {
  public:
    Entry * _prev;
    Entry * _next;
  };

  Entry * get (void) {
    if (_head == NULL) {
      return NULL;
    }
    Entry * e = _head;
    _head = _head->_next;
    if (_head) {
      _head->_prev = NULL;
    }
    return e;
  }

  void remove (Entry * e) {
    if (e != _head) {
      e->_prev->_next = e->_next;
      if (e->_next) {
	e->_next->_prev = e->_prev;
      }
    } else {
      _head = _head->_next;
      _head->_next->_prev = NULL;
    }
  }

  void insert (Entry * e) {
    if (_head) {
      e->_next = _head;
      _head->_prev = e;
      e->_prev = NULL;
      _head = e;
    } else {
      e->_prev = NULL;
      e->_next = NULL;
      _head = e;
    }
  }
   
private:
  Entry * _head;
};

}
#else

namespace HL {

class DLList {
public:

  inline DLList (void) {
    clear();
  }

  class Entry;

  /// Clear the list.
  inline void clear (void) {
    head.setPrev (&head);
    head.setNext (&head);
    sanityCheck();
  }

  /// Is the list empty?
  inline bool isEmpty (void) const {
    sanityCheck();
    return (head.getNext() == &head);
  }

  /// Get the head of the list.
  inline Entry * get (void) {
    sanityCheck();
    Entry * e = head.next;
    if (e == &head) {
      sanityCheck();
      return NULL;
    }
    e->remove();
    sanityCheck();
    return (Entry *) e;
  }

  /// Remove one item from the list.
  inline void remove (Entry * e) {
    assert (e != &head);
    sanityCheck();
    e->remove();
    sanityCheck();
  }

  /// Insert an entry into the head of the list.
  inline void insert (Entry * e) {
    assert (e != NULL);
    sanityCheck();
    e->insert (&head, head.next);
    sanityCheck();
  }

  /// An entry in the list.
  class Entry {
  public:
    //  private:
    inline void setPrev (Entry * p) { assert (p != NULL); prev = p; }
    inline void setNext (Entry * p) { assert (p != NULL); next = p; }
    inline Entry * getPrev (void) const { return prev; }
    inline Entry * getNext (void) const { return next; }
    inline void remove (void) {
      prev->setNext(next);
      next->setPrev(prev);
      next = NULL;
      prev = NULL;
    }
    inline void insert (Entry * p, Entry * n) {
      prev = p;
      next = n;
      p->setNext (this);
      n->setPrev (this);
    }
    Entry * prev;
    Entry * next;
  };


private:

  void sanityCheck (void) const {
    assert (head.next != NULL);
    assert (head.prev != NULL);
#if 0
    Entry * p = head.next;
    while (p != &head) {
      Entry * n = p->next;
      assert (n->prev == p);
      p = n;
    }
#endif
  }

  /// The head of the list.
  Entry head;

};

}

#endif



#endif
