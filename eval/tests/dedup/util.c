#if defined(ENABLE_DMP)
#include "dmp.h"
#endif


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "queue.h"
#include "dedupdef.h"
#include "util.h"

void Calc_SHA1Sig(const byte* buf, int32 num_bytes, u_char * digest)
{
  SHA_CTX sha;
 
  SHA1_Init(&sha);
  SHA1_Update(&sha, buf, num_bytes);
  SHA1_Final(digest, &sha);
}

int
xread(int sd, void *buf, size_t len)
{
  char *p = (char *)buf;
  size_t nrecv = 0;
  size_t rv;
  
  while (nrecv < len) {
    rv = read(sd, p, len - nrecv);
    //write(stdout, p, len - nrecv);
    if (0 > rv && errno == EINTR)
      continue;
    if (0 > rv)
      return -1;
    if (0 == rv)
      return 0;
    nrecv += rv;
    p += rv;
  }
  return nrecv;
}

int
xwrite(int sd, const void *buf, size_t len)
{
  char *p = (char *)buf;
  size_t nsent = 0;
  ssize_t rv;
  
  while (nsent < len) {
    rv = write(sd, p, len - nsent);
    if (0 > rv && (errno == EINTR || errno == EAGAIN))
      continue;
    if (0 > rv)
      return -1;
    nsent += rv;
    p += rv;
  }
  return nsent;
}

unsigned int
hash_from_key_fn( void *k ){
  int i = 0;
  int hash = 0;
  unsigned char* sha1name = ((CacheKey*)k)->sha1name;
  for (i = 0; i < SHA1_LEN; i++) {
    hash += *(((unsigned char*)sha1name) + i);
  }
  return hash;
}

//for 64-bit data types, remove if instrumentation no longer needed
#include <stdint.h>
#include <inttypes.h>
static uint64_t l1_num = 0;
static uint64_t l1_scans = 0;
static uint64_t l2_num = 0;
static uint64_t l2_scans = 0;

void dump_scannums() {
  /*printf("Number L1 scans: %"PRIu64"\n", l1_num);
  printf("Number L1 elements touched on average: %"PRIu64"\n", l1_scans/l1_num);
  printf("Number L2 scans: %"PRIu64"\n", l2_num);
  printf("Number L2 elements touched on average: %"PRIu64"\n", l2_scans/l2_num);
  */
}

struct write_list * writelist_insert(struct write_list * list_head, u_int32 aid, u_int32 cid) {
  struct write_list * p, * pnext = NULL, *panchornext = NULL, *prenew, *tmp;
  if (list_head == NULL) {
    list_head = (struct write_list *) malloc(sizeof(struct write_list));
    pnext = NULL;
    p = list_head;
    panchornext = p;
  } else {
    if (aid < list_head->anchorid || (aid == list_head->anchorid && cid < list_head->cid)) {
      p = (struct write_list *) malloc(sizeof(struct write_list));
      p->next = list_head;
      if (aid < list_head->anchorid) {
        p->anchornext = p;
      } else {
        p->anchornext = list_head->anchornext;
      }
      return p;
    } else {
      p = list_head;

      l1_num++;
      while (p->anchornext->next != NULL && aid > p->anchornext->next->anchorid) {
        p = p->anchornext->next;
        l1_scans++;
      }
      panchornext = p->anchornext;
      l2_num++;
      while (p->next != NULL && (aid > p->next->anchorid || (aid == p->next->anchorid && cid > p->next->cid))) {
        p = p->next;
        l2_scans++;
      }
      tmp = p;
      pnext = p->next;   
      p->next = (struct write_list *) malloc(sizeof(struct write_list));
      p = p->next;
      if (aid == tmp->anchorid && cid > tmp->cid && (pnext == NULL || pnext->anchorid >aid)) {
        prenew = list_head;
        while (aid > 0 && prenew->anchorid < aid-1) {
          prenew= prenew->anchornext->next;
        }
        while (prenew != NULL && prenew != p && prenew->anchorid == aid) {
          prenew->anchornext = p;
          prenew = prenew->next;
        } 
        panchornext = p;
      }
      if (pnext != NULL && pnext->anchorid == aid) {
        panchornext = pnext->anchornext;
      }
      if (aid > tmp->anchorid && (pnext == NULL || pnext->anchorid > aid)) {
        panchornext = p;
      }
    }
  }

  p->next = pnext;
  p->anchornext = panchornext;

  return p;
} 
