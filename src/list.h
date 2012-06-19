#ifndef __LIST_H__
#define __LIST_H__
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
 * @file   list.h
 * @brief  Link list, in order to avoid the usage of STL. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <map>

#if !defined(_WIN32)
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
struct Entry {
  Entry * prev;
  Entry * next;
};

inline bool isEmpty(Entry ** head) {
  return (*head == NULL ? true : false);
}

inline Entry * nextEntry(Entry * current) {
  return current->next;
}

inline void moveWholeList(Entry * prev, Entry ** src) {
  Entry * next;
  Entry * srchead, *srctail;

  if (*src == NULL) {
    // Nothing to move
    return;
  }

  // In our case, _activelist can not be empty.
  assert(prev != NULL);
  next = prev->next;

  srchead = *src;
  srctail = srchead->prev;

  // We move the whole list to the tail of destined list.
  next->prev = srctail;

  srchead->prev = prev;
  srctail->next = next;
  prev->next = srchead;

  //If we move out all whole list, now *src is point to NULL.
  *src = NULL;
  return;
}

// Insert one entry to specified list.
inline void insertTail(Entry * entry, Entry ** head) {
  int i = 0;

  // If it is empty, simply set head pointer to current node. 
  if (*head == NULL) {
    *head = entry;
    entry->prev = entry;
    entry->next = entry;
  } else {
    Entry * header = *head;
    Entry * tail = header->prev;

    // Modify the next and previous entry.  
    tail->next = entry;
    header->prev = entry;

    // Modify the global head pointer.
    entry->prev = tail;
    entry->next = header;
  }
  return;
}

// Insert one entry to specified list. Here, we assume that 
// the target list is not empty (at least one element inside).
inline void insertHead(Entry * entry, Entry ** head) {
  int i = 0;

  // If it is empty, simply set head pointer to current node. 
  Entry * header = *head;
  Entry * next = header->next;

  // Modify the next and previous entry.  
  header->next = entry;
  next->prev = entry;

  // Modify the global head pointer.
  entry->prev = header;
  entry->next = next;
  return;
}

// Insert one entry to specified list.
inline void insertNext(Entry * entry, Entry * prev) {
  Entry * next = prev->next;

  // Modify correponding pointers.
  next->prev = entry;
  entry->prev = prev;
  entry->next = next;
  prev->next = entry;
  return;
}

// Remove one entry from specified list.
inline void removeEntry(Entry * entry, Entry ** head) {
  Entry * prev, *next;

  assert(*head != NULL);

  prev = entry->prev;
  next = entry->next;

  if (prev == entry && next == entry) {
    // This is the last entry in this list.
    *head = NULL;
  } else {
    // Modify correponding pointer.
    prev->next = next;
    next->prev = prev;

    // If we are removing the head entry, trying to modify that.
    if (*head == entry) {
      *head = next;
    }
  }
}

inline void printEntries(Entry ** head) {
  Entry * first, * entry;
  int i = 0;
  entry = *head;
  first = entry;

  fprintf(stderr, "%d: PRINTENTY first %p\n", getpid(), first);
  do {
    if(!entry)
      break;
    fprintf(stderr, "%d: PRINTENTRY %d, entry %p\n", getpid(), i++, entry);
    entry = entry->next;
  }while(entry != first && i <10);

  return;
}

// Remove the head entry from specified list.
// We also return the address for this entry.
inline Entry * removeHeadEntry(Entry ** head) {
  // If list is empty.
  if (*head == NULL) {
    return NULL;
  }

  Entry * header = *head;
  removeEntry(header, head);
  return (header);
}

};
#endif /* __ALIVETHREADS_H__ */
