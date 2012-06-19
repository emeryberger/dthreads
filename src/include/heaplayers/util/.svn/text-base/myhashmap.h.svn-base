// -*- C++ -*-

#ifndef _HL_MYHASHMAP_H_
#define _HL_MYHASHMAP_H_

#include <assert.h>
#include <new>

#include "mallocheap.h"
#include "hash.h"

namespace HL {

template <typename Key,
	  typename Value,
	  class Allocator = mallocHeap>
class MyHashMap {

public:

  MyHashMap (int size = INITIAL_NUM_BINS)
    : num_bins (size)
  {
    bins = new (alloc.malloc (sizeof(ListNodePtr) * num_bins)) ListNodePtr;
    for (int i = 0 ; i < num_bins; i++) {
      bins[i] = NULL;
    }
  }

  void clear (void) {
    for (int i = 0; i < num_bins; i++) {
      ListNode * p = bins[i];
      while (p) {
	ListNode * n = p;
	p = p->next;
	alloc.free ((void *) n);
      }
      bins[i] = NULL;
    }
  }

  void set (Key k, Value v) {
    int binIndex = (unsigned int) hash(k) % num_bins;
    ListNode * l = bins[binIndex];
    while (l != NULL) {
      if (l->key == k) {
	l->value = v;
	return;
      }
      l = l->next;
    }
    // Didn't find it.
    insert (k, v);
  }

  bool get (Key k, Value& v) {
    int binIndex = (unsigned int) hash(k) % num_bins;
    ListNode * l = bins[binIndex];
    while (l != NULL) {
      if (l->key == k) {
	v = l->value;
	return true;
      }
      l = l->next;
    }
    // Didn't find it.
    return false;
  }

  bool find_bool (Key k) {
    int binIndex = (unsigned int) hash(k) % num_bins;
    ListNode * l = bins[binIndex];
    while (l != NULL) {
      if (l->key == k) {
	return true;
      }
      l = l->next;
    }
    // Didn't find it.
    return false;
  }

  void erase (Key k) {
    int binIndex = (unsigned int) hash(k) % num_bins;
    ListNode * curr = bins[binIndex];
    ListNode * prev = NULL;
    while (curr != NULL) {
      if (curr->key == k) {
	// Found it.
	if (curr != bins[binIndex]) {
	  assert (prev->next == curr);
	  prev->next = prev->next->next;
	  alloc.free (curr);
	} else {
	  ListNode * n = bins[binIndex]->next;
	  alloc.free (bins[binIndex]);
	  bins[binIndex] = n;
	}
	return;
      }
      prev = curr;
      curr = curr->next;
    }
  }

private:

  class ListNode {
  public:
    ListNode (void)
      : next (NULL)
    {}
    Key key;
    Value value;
    ListNode * next;
  };

public:

  class iterator {
  public:

    iterator (void)
      : _theNode (NULL),
	_index (0)
    {}
    
    explicit iterator (ListNode * l, int index)
      : _theNode (l),
	_index (index)
    {}

    Value operator*(void) {
      return _theNode->value;
    }
   
    bool operator!=(iterator other) const {
      return (this->_theNode != other._theNode);
    }
    
    bool operator==(iterator other) const {
      return (this->_theNode == other._theNode);
    }
    
    iterator& operator++ (void) {
      if (_theNode) {
	if (_theNode->next) {
	  _theNode = _theNode->next;
	} else {
	  _theNode = NULL;
	  for (_index++; (_index < num_bins) && (bins[_index] == NULL); _index++)
	    ;
	  if (_index < num_bins) {
	    _theNode = bins[_index];
	  }
	}
      }
      return *this;
    }
    
    iterator& operator=(iterator& it) {
      this->_theNode = it->_theNode;
      this->_index = it->_index;
      return *this;
    }

  private:
    ListNode * _theNode;
    int _index;
  };
  
  iterator begin (void) {
    iterator it (bins[0], 0);
    return it;
  }
  
  iterator end (void) {
    iterator it (NULL, num_bins - 1);
    return it;
  }

  iterator find (Key k) {
    int binIndex = (unsigned int) hash(k) % num_bins;
    ListNode * l = bins[binIndex];
    while (l != NULL) {
      if (l->key == k) {
	return iterator (l, binIndex);
      }
      l = l->next;
    }
    // Didn't find it.
    return end();
  }


private:

  void insert (Key k, Value v) {
    int binIndex = (unsigned int) hash(k) % num_bins;
    ListNode * l = new (alloc.malloc (sizeof(ListNode))) ListNode;
    l->key = k;
    l->value = v;
    l->next = bins[binIndex];
    bins[binIndex] = l;
  }

  enum { INITIAL_NUM_BINS = 511 };

  const int num_bins;

  typedef ListNode * ListNodePtr;
  ListNodePtr * bins;
  Allocator alloc;
};

};

#endif
