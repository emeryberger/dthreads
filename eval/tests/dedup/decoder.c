#include "util.h"
#include "debug.h"
#include "dedupdef.h"
#include "hashtable.h"
#include "config.h"
#include "hashtable_private.h"
#include "queue.h"
#include "decoder.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

send_buf_item * buf_checkcache[NUM_BUF_CHECKCACHE];
send_buf_item * buf_decompress[NUM_BUF_DECOMPRESS];
send_buf_item * buf_reassemble[NUM_BUF_REASSEMBLE];

//for receiver
#ifdef PARALLEL
pthread_mutex_t mutex_decompress = PTHREAD_MUTEX_INITIALIZER,
  mutex_checkcache = PTHREAD_MUTEX_INITIALIZER,
  mutex_reassemble = PTHREAD_MUTEX_INITIALIZER,
  mutex_end_decompress = PTHREAD_MUTEX_INITIALIZER,
  mutex_end_checkcache = PTHREAD_MUTEX_INITIALIZER,
  mutex_end_recvblock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty_decompress = PTHREAD_COND_INITIALIZER,
  empty_checkcache = PTHREAD_COND_INITIALIZER,
  empty_reassemble = PTHREAD_COND_INITIALIZER,
  full_decompress = PTHREAD_COND_INITIALIZER,
  full_checkcache = PTHREAD_COND_INITIALIZER,
  full_reassemble = PTHREAD_COND_INITIALIZER;
#endif 

int head_decompress = 0,
  tail_decompress = 0,
  head_checkcache = 0,
  tail_checkcache = 0,
  head_reassemble = 0,
  tail_reassemble = 0;
int end_Decompress = 0,
  end_RecvBlock = 0,
  end_CheckCache = 0;

/*
 * Given SHA-1 key, check whether the corresponding compressed data is available
 */
void * 
CheckCache(void * args) {
  while (1) {
    send_buf_item * item;
    if (tail_checkcache == head_checkcache && end_RecvBlock) {
      break;     
    }
    item = buf_checkcache[tail_checkcache];
    tail_checkcache ++;

    if (tail_checkcache == NUM_BUF_CHECKCACHE) tail_checkcache = 0;

    data_chunk * value;
    struct hash_entry * ent;
    /* search the cache */
    //if data not available, wait
    while ((ent = hashtable_search(cache, item->content)) == NULL) {
    }

    value = ent->v;
    
    /* cache hit */
    //get the data(SHA-1), put it into the reassemble queue
    MEM_FREE(item->content);
    item->content = value->start;      
    send_body * body = (send_body *)item->str;
    body->len = value->len;

    buf_reassemble[head_reassemble] = item;
    head_reassemble ++;

    if (head_reassemble == NUM_BUF_REASSEMBLE) head_reassemble = 0;
  }

  end_CheckCache +=1;

  return NULL;
}

/*
 * Decompress the data
 */
void * 
Decompress(void * args) {
  u_char tmpstr[UNCOMPRESS_BOUND];
  while (1) {
    send_buf_item * item;
    //get one item
    if (tail_decompress == head_decompress && end_RecvBlock) {
      break;     
    }
    
    item = buf_decompress[tail_decompress];
    tail_decompress ++;
    if (tail_decompress == NUM_BUF_DECOMPRESS) tail_decompress = 0;
        
    send_body * body = (send_body*)item->str;

    u_char * pstr = NULL;
    u_long len = 0;
    int t = 0;
    unsigned long len_32;

    //uncompress the item
    switch (compress_way) {
    case COMPRESS_GZIP:
      len_32 = UNCOMPRESS_BOUND;
      t = uncompress(tmpstr, &len_32, item->content, body->len);

      len = len_32;
      pstr= (u_char *)malloc(len);
      body->len = len;
      memcpy(pstr, tmpstr, len);
      MEM_FREE(item->content);
      item->content = pstr;
      break;
    case COMPRESS_BZIP2:
      break;
    case COMPRESS_NONE:
      break;
    }
    u_char * key = (u_char * )malloc(SHA1_LEN);
    Calc_SHA1Sig(item->content, body->len, key);
    
    data_chunk * value = (data_chunk *)malloc(sizeof(data_chunk));
    value->start = item->content;
    value->len = body->len;

    if (hashtable_insert(cache, key, value) == 0) {
      EXIT_TRACE("hashtable_insert failed");
    }  
    //put it into the reassemble queue
    buf_reassemble[head_reassemble] = item;
    head_reassemble ++;
    if (head_reassemble == NUM_BUF_REASSEMBLE) head_reassemble = 0;
  }

  end_Decompress += 1;
  return NULL;
}

struct chunk_list * list_insert(struct chunk_list * list_head, u_int32 cid) {
  struct chunk_list * p, * pnext;
  if (list_head == NULL) {
    list_head = (struct chunk_list *) malloc(sizeof(struct chunk_list));
    pnext = NULL;
    p = list_head;
  } else {
    if (cid < list_head->cid) {
      p = (struct chunk_list *) malloc(sizeof(struct chunk_list));
      p->next = list_head;
      return p;
    } else {
      p = list_head;
      while (p->next != NULL && cid > p->next->cid)
        p = p->next;
      pnext = p->next;   
      p->next = (struct chunk_list *) malloc(sizeof(struct chunk_list));
      p = p->next;
    }
  }

  p->next = pnext;
  return p;
} 

/*
 * Reassemble the chunks together, and write it to the file
 */
void * 
Reassemble(void * args) {
  int chunkcount = 0;

  int fd = -1;
  struct chunk_list * list_head, * p;
  list_head = NULL;

  if (args != NULL) {
    fd = open((char *)args, O_CREAT|O_WRONLY|O_TRUNC);
    if (fd < 0) 
      perror("Reassemble open");
    fchmod(fd, ~(S_ISUID | S_ISGID |S_IXGRP | S_IXUSR | S_IXOTH));
  }

  while (1) {
    int f = 0;
    f = 0;
    send_buf_item * item;
    if (tail_reassemble == head_reassemble) {
      break;     
    }
    item = buf_reassemble[tail_reassemble];
    tail_reassemble ++;
    if (tail_reassemble == NUM_BUF_REASSEMBLE) tail_reassemble = 0;

    send_body * body = NULL;
    send_head * head;
    
    switch (item->type) {
    case TYPE_FINGERPRINT:
    case TYPE_COMPRESS:
      body = (send_body *)item->str;
      if (fd == -1) {
        //if havn't get the file header, insert current chunk into the queue
        p = list_insert(list_head, body->cid);
        if (list_head ==NULL || list_head->cid > body->cid) {
          list_head = p;
        }
        p->content = item->content;
        p->cid = body->cid;
        p->len = body->len;
        MEM_FREE(body);
      } else {
          if (body->cid == chunkcount) {
            //if this chunk is in the right order, write it into the file
                          
            if (0 > xwrite(fd, item->content, body->len)) {
              perror("xwrite");
              EXIT_TRACE("xwrite\n");
            }
            chunkcount ++;
            p = list_head;
            while (p != NULL && p->cid == chunkcount) {
              if (0 > xwrite(fd, p->content, p->len)) {
                perror("xwrite");
                EXIT_TRACE("xwrite\n");
              }
              chunkcount ++;
              p = p->next;
            }
            list_head = p;
          } else {
            //if this chunk is out of order, insert it into the list
            p = list_insert(list_head, body->cid);
            if (list_head ==NULL || list_head->cid > body->cid) {
              list_head = p;
            }
            p->content = item->content;
            p->cid = body->cid;
            p->len = body->len;
          }        
          // }
      }
      break;
    case TYPE_HEAD:
      //get file header
      head = (send_head *)item->str;
      if (fd == -1) {
        fd = open(head->filename, O_CREAT|O_WRONLY|O_TRUNC);
        if (fd < 0)
          perror("head_open");
      }

      MEM_FREE(head);
      chunkcount = 0;

      p = list_head;
      while (p != NULL && p->cid == chunkcount) {
        if (0 > xwrite(fd, p->content, p->len)) {
          EXIT_TRACE("xwrite\n");
        }
        if (p->content) MEM_FREE(p->content);
        chunkcount ++;
        p = p->next;
      }
      list_head = p;
      
      break;
    }
    
    MEM_FREE(item);
  }

  //write the remaining chunks into the file
  p = list_head;
  while (p != NULL) {
    if (0 > xwrite(fd, p->content, p->len)) {
      EXIT_TRACE("xwrite\n");
    }

    if (p->cid == chunkcount) 
      chunkcount ++;
    else {
      chunkcount = p->cid+1;
    }
    p = p->next;    
  }

  close(fd);
  return NULL;
}

/*
 * Read the input file
 */
void *
RecvBlock(void * args) 
{
  config * conf = (config *)args;
  int fd = 0;
  
  int check_count = 0;

  fd = open(conf->infile, O_RDONLY|O_LARGEFILE);
  if (fd < 0) {
    perror("infile open");
    return NULL;
  }

  send_buf_item * item;

  while(1) {  
    item = (send_buf_item*)malloc(sizeof(send_buf_item));

    if (xread(fd, &item->type, sizeof(item->type)) < 0){
      perror("xread:");
      EXIT_TRACE("xread type fails\n");
      return NULL;
    }
    send_body * body;
    send_head * head;

    if (item->type == TYPE_FINISH) {
      break;
    }

    int checkbit;
      
    //get the item
    switch (item->type) {
    case TYPE_FINGERPRINT:
      //if fingerprint, put it into sendcache thread
      body = (send_body *)malloc(sizeof(send_body));
      if (xread(fd, &body->cid, sizeof(body->cid)) < 0){
        EXIT_TRACE("xread body fails\n");
      }
      if (body->cid == check_count) {
        check_count ++;
      } else {
        printf("error check-count %d, body->cid %d\n", check_count, body->cid);
      }
      if (xread(fd, &body->len, sizeof(body->len)) < 0){
        EXIT_TRACE("xread body fails\n");
      }
      item->str = (u_char *)body;
      if (body->len > 0) {        

        item->content = (u_char *)malloc(body->len);
        if (item->content == NULL) {
          printf ("Error: Content is NULL\n");
          perror("malloc");
        }
      
        if (xread(fd, item->content, body->len) < 0){
          perror("xread:");
          EXIT_TRACE("fp xread content fails\n");
        }    
        //send to checkcache
        buf_checkcache[head_checkcache] = item;
        head_checkcache ++;
        if (head_checkcache == NUM_BUF_CHECKCACHE) head_checkcache = 0;
      }
      break;
    case TYPE_COMPRESS:
      //if compressed data, send it to decompress thread
      body = (send_body *)malloc(sizeof(send_body));
      if (xread(fd, &body->cid, sizeof(body->cid)) < 0){
        EXIT_TRACE("xread body fails\n");
      }
      if (body->cid == check_count) {
        check_count ++;
      } else {
        printf("error check-count %d, body->cid %d\n", check_count, body->cid);
      }
      if (xread(fd, &body->len, sizeof(body->len)) < 0){
        EXIT_TRACE("xread body fails\n");
      }
      item->str = (u_char *)body;
      if (body->len > 0) {
        item->content = (u_char *)malloc(body->len*sizeof(u_char));
        if (xread(fd, item->content, body->len) < 0){
          perror("xread");
          EXIT_TRACE("compress: xread content fails\n");
        }
        //send to decompress
        buf_decompress[head_decompress] = item;
        head_decompress ++;
        if (head_decompress == NUM_BUF_DECOMPRESS) head_decompress = 0;
      }
      break;
    case TYPE_HEAD:
      //if file header, initialize
      if (xread(fd, &checkbit, sizeof(int)) < 0){
        EXIT_TRACE("xwrite head fails\n");
      }
      if (checkbit != CHECKBIT) {
        printf("format error!\n");
        exit(1);
      }
      head = (send_head *)malloc(sizeof(send_head));
      if (xread(fd, head, sizeof(send_head)) < 0){
        EXIT_TRACE("xread head fails\n");
      }
      item->str = (u_char *)head;
      buf_reassemble[head_reassemble] = item;
      head_reassemble ++;
      if (head_reassemble == NUM_BUF_REASSEMBLE) head_reassemble = 0;
      break;
    }    
  }

  end_RecvBlock = TRUE;
  close(fd);
  return NULL;
}

void 
Decode(config * conf){
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif
  RecvBlock(conf);
  Decompress(NULL);
  CheckCache(NULL);
  if (strcmp(conf->outfile, "") == 0)
    Reassemble(NULL);
  else Reassemble(conf->outfile);
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif
}
