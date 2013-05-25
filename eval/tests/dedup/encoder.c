#if defined(ENABLE_DMP)
#include "dmp.h"
#endif


#include "util.h"
#include "dedupdef.h"
#include "encoder.h"
#include "debug.h"
#include "hashtable.h"
#include "config.h"
#include "hashtable_private.h"
#include "rabin.h"
#include "queue.h"
#include "binheap.h"
#include "tree.h"

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

#ifdef PARALLEL
#include <pthread.h>
#endif //PARALLEL

#define INT64(x) ((unsigned long long)(x))
#define MSB64 INT64(0x8000000000000000ULL)

#define INITIAL_SIZE 4096

extern config * conf;

/*
 * The pipeline model for Encode is DataProcess->FindAllAnchor->ChunkProcess->Compress->SendBlock
 * Each stage has basically three steps:
 * 1. fetch a group of items from the queue
 * 2. process the items
 * 3. put them in the queue for the next stage.
 */


//We perform global anchoring in the first stage and refine the anchoring
//in the second stage. This array keeps track of the number of chunks in
//a coarse chunk.
int chunks_per_anchor[QUEUE_SIZE];

//The queues between the pipeline stages
struct queue *chunk_que, *anchor_que, *send_que, *compress_que;

int filecount = 0,
  chunkcount = 0;

//header
send_buf_item * headitem;

struct thread_args {
  //thread id, unique within a thread pool (i.e. unique for a pipeline stage)
  int tid;
  //number of queues available, first and last pipeline stage only
  int nqueues;
  //file descriptor, first pipeline stage only
  int fd;
  //configuration, last pipeline stage only
  config * conf;
};

static int write_file(int fd, u_char type, int seq_count, u_long len, u_char * content) {
  if (xwrite(fd, &type, sizeof(type)) < 0){
    perror("xwrite:");
    EXIT_TRACE("xwrite type fails\n");
    return -1;
  }
  if (xwrite(fd, &seq_count, sizeof(seq_count)) < 0){
    EXIT_TRACE("xwrite content fails\n");
  }
  if (xwrite(fd, &len, sizeof(len)) < 0){
    EXIT_TRACE("xwrite content fails\n");
  }
  if (xwrite(fd, content, len) < 0){
    EXIT_TRACE("xwrite content fails\n");
  }
  return 0;
}

int rf_win;
int rf_win_dataprocess;

void sub_Compress(send_buf_item * item) {
    send_body * body = (send_body*)item->str;
    u_long len;
    byte * pstr = NULL;

    //compress the item
    if (compress_way == COMPRESS_GZIP) {
      unsigned long len_32;
      len = body->len + (body->len >> 12) + (body->len >> 14) + 11;
      if(len >> 32) {
        perror("compress");
        EXIT_TRACE("compress() failed\n");
      }
      len_32 = len & 0xFFFFFFFF;
      pstr = (byte *)malloc(len);
      if(pstr == NULL) {
        EXIT_TRACE("Memory allocation failed.\n");
      }
      /* compress the block */
      if (compress(pstr, &len_32, item->content, body->len) != Z_OK) {
        perror("compress");
        EXIT_TRACE("compress() failed\n");
        }
      len = len_32;

      u_char * key = (u_char *)malloc(SHA1_LEN);
      if(key == NULL) {
        EXIT_TRACE("Memory allocation failed.\n");
      }
	  //fprintf(stderr, "process %d : allocated key %p\n", getpid(), key);
      memcpy(key, item->sha1, sizeof(u_char)*SHA1_LEN);
      //Calc_SHA1Sig(item->content, body->len, key);
      struct hash_entry * entry;
      /* search the cache */
#ifdef PARALLEL
      pthread_mutex_t *ht_lock = hashtable_getlock(cache, (void *)key);
      pthread_mutex_lock(ht_lock);
#endif
      if ((entry = hashtable_search(cache, (void *)key)) == NULL) {
        //if cannot find the entry, error
        printf("Error: Compress hash error\n");
        exit(1);
      } else {
        //if cache hit, put the compressed data in the hash
        //There is a memory leak here. 
        //free(item->sha1);
        item->sha1 = NULL;
        struct pContent * value = ((struct pContent *)entry->v);
        value->len = len;
        value->content = pstr;
        value->tag = TAG_DATAREADY;

        hashtable_change(entry, (void *)value);
#ifdef PARALLEL
        pthread_cond_signal(&value->empty);
#endif
      }

#ifdef PARALLEL
    pthread_mutex_unlock(ht_lock);
#endif

      body->len = SHA1_LEN;
      MEM_FREE(item->content);
      item->content = key;    
    }
    
   return;
}

/*
 * Compress the chunks
 */
void *Compress(void * targs) {
  struct thread_args *args = (struct thread_args *)targs;
  const int qid = args->tid / MAX_THREADS_PER_QUEUE;
  send_buf_item * item;
  send_buf_item ** fetchbuf;

  fprintf(stderr, "%d : Compress\n", getpid()); 
  fetchbuf = (send_buf_item **)malloc(sizeof(send_buf_item *)*ITEM_PER_FETCH);
  if(fetchbuf == NULL) {
    EXIT_TRACE("Memory allocation failed.\n");
  }
  int fetch_count = 0, fetch_start = 0;
  
  send_buf_item * tmpbuf[ITEM_PER_INSERT];
  int tmp_count = 0;  

  while(1) {
    //get items from the queue
    if (fetch_count == fetch_start) {
      int r = dequeue(&compress_que[qid], &fetch_count, (void **)fetchbuf);
      if (r < 0) break;

      fetch_start = 0;
    }
    
    //fetch one item
    item = fetchbuf[fetch_start];
    fetch_start ++;

    if (item == NULL) break;

    sub_Compress(item);

    tmpbuf[tmp_count] = item;
    tmp_count ++;
    
    //put the item in the next queue for the write thread
    if (tmp_count >= ITEM_PER_INSERT) {      
      enqueue(&send_que[qid], &tmp_count, (void **)tmpbuf);
    }
  }
 
  if (tmp_count > 0) {
    enqueue(&send_que[qid], &tmp_count, (void **)tmpbuf);
  }
  
  //free(fetchbuf);


  //count the number of compress threads that have finished
  queue_signal_terminate(&send_que[qid]);
  return NULL;
}

//This process is trying to calculate the hash value at first.
// Then it will search the hash table to find whether this chunk has been
// in process. If it is not in hash table, put it in the hashtable and the queue for
// the compress thread.
// If current chunk has already in the hash table, put the item into the queue for the write thread.
// But before that, we should know that there is one more count, so we modify value->count and put it back?
send_buf_item * sub_ChunkProcess(data_chunk chunk) {
  send_buf_item * item;
  send_body * body = NULL;
  u_char * key;

  key = (u_char *)malloc(SHA1_LEN);
  if(key == NULL) {
    EXIT_TRACE("Memory allocation failed.\n");
  }
  
  Calc_SHA1Sig(chunk.start, chunk.len, key);


  struct hash_entry * entry;
  /* search the cache */
#ifdef PARALLEL
  pthread_mutex_t *ht_lock = hashtable_getlock(cache, (void *)key);
  pthread_mutex_lock(ht_lock);
#endif
  if ((entry = hashtable_search(cache, (void *)key)) == NULL) {
    // cache miss: put it in the hashtable and the queue for the compress thread
    struct pContent * value;

    value = (struct pContent *)malloc(sizeof(struct pContent));
    if(value == NULL) {
      EXIT_TRACE("Memory allocation failed.\n");
    }
    value->len = 0;
    value->count = 1;
    value->content = NULL;
    value->tag = TAG_OCCUPY;
    if (hashtable_insert(cache, key, value) == 0) {
      EXIT_TRACE("hashtable_insert failed");
    }
//	fprintf(stderr, "%d: insert key %p\n", getpid(), key);
#ifdef PARALLEL
    pthread_cond_init(&value->empty, NULL);
#endif
      
    item = (send_buf_item *)malloc(sizeof(send_buf_item));
    body = (send_body *)malloc(sizeof(send_body));
    if(item == NULL || body == NULL) {
      EXIT_TRACE("Memory allocation failed.\n");
    }
    body->fid = filecount;
    body->cid = chunk.cid;
    body->anchorid = chunk.anchorid;
    body->len = chunk.len;
    item->content = (u_char * )malloc(body->len + 1);
    if(item->content == NULL) {
      EXIT_TRACE("Memory allocation failed.\n");
    }
    memcpy(item->content, chunk.start, body->len);
    item->content[body->len] = 0;
    item->sha1 = (u_char *)malloc(SHA1_LEN);
    if(item->sha1 == NULL) {
      EXIT_TRACE("Memory allocation failed.\n");
    }
    memcpy(item->sha1, key, sizeof(u_char)*SHA1_LEN);

    item->type = TYPE_COMPRESS;
    // In fact, body is just one field in the item, that includes the total information.
    item->str = (u_char *)body;

  } else {
    // cache hit: put the item into the queue for the write thread 
    struct pContent * value = ((struct pContent *)entry->v);
    value->count += 1;
    // LTP, it is weird since value is pointed to v. unnecessary at all.
    hashtable_change(entry, (void *)value);

    item = (send_buf_item *)malloc(sizeof(send_buf_item));
    body = (send_body *)malloc(sizeof(send_body));
    if(item == NULL || body == NULL) {
      EXIT_TRACE("Memory allocation failed.\n");
    }
    body->fid = filecount;
    body->cid = chunk.cid;
    body->anchorid = chunk.anchorid;
    body->len = SHA1_LEN;
    // What??? content is a key now but previous is the chunk itself?
    // Also, if the chunk is already met, why we still need to handle this?
    item->content = key;
	//sha1 is NULL now, not the key anymore?
    item->sha1 = NULL;

    item->type = TYPE_FINGERPRINT;
    item->str = (u_char *)body;

  }

#ifdef PARALLEL
  pthread_mutex_unlock(ht_lock);
#endif

  if (chunk.start) MEM_FREE(chunk.start);
    
  return item;
}

/*
 * Check whether a chunk is in the hash table or not.
 * If it is in the hash table, send only the SHA1 sum to the write thread
 * If not, send the data to the compress thread to get the output data
 */
void * 
ChunkProcess(void * targs) {
  struct thread_args *args = (struct thread_args *)targs;
  const int qid = args->tid / MAX_THREADS_PER_QUEUE;
  data_chunk chunk;
  send_buf_item * item;
  
  fprintf(stderr, "%d : ChunkProcess\n", getpid()); 

  data_chunk * fetchbuf[CHUNK_ANCHOR_PER_FETCH];
  int fetch_count = 0, fetch_start = 0;
  
  send_buf_item * tmpbuf[ITEM_PER_INSERT];
  int tmp_count = 0;
  send_buf_item * tmpsendbuf[ITEM_PER_INSERT];
  int tmpsend_count = 0;

  // The input for this threads just come from the previous stage only. 
  // Each thread just need to remember its qid only.
  while (1) {
    //if no items existing, fetch a group of items from the pipeline
    if (fetch_count == fetch_start) {
	  //dequeue will cause current thread to hang on the queue. 
      // When r < 0, means no data anymore. Current thread can exit now.
      int r = dequeue(&chunk_que[qid], &fetch_count, (void **)fetchbuf);
      if (r < 0) break;

      fetch_start = 0;
    }
    
    //get one item
	//fprintf(stderr, "%d: %d item %p\n", getpid(), args->tid, chunk.start);
    chunk.start = fetchbuf[fetch_start]->start;
    chunk.len = fetchbuf[fetch_start]->len;    
    chunk.cid = fetchbuf[fetch_start]->cid;
    chunk.anchorid = fetchbuf[fetch_start]->anchorid;
    fetch_start ++;

    if (chunk.start == NULL) break;

    item = sub_ChunkProcess(chunk);

	// After sub_chunkprocess, we just got the item. 
    // Start to handle the item according different type now.
    if (item->type == TYPE_COMPRESS) {

      tmpbuf[tmp_count] = item;
      tmp_count ++;
     
	  // We won't do insert every time, we will wait for ITEM_PER_INSERT.
      // That means we can improve the performance???? Not sure, but we will
      // decrease the number of communication since we use local buffer to cache all items. 
      if (tmp_count >= ITEM_PER_INSERT) {      
        enqueue(&compress_que[qid], &tmp_count, (void **)tmpbuf);
      }
    } else {
      tmpsendbuf[tmpsend_count] = item;
      tmpsend_count ++;
      
      if (tmpsend_count >= ITEM_PER_INSERT) {
        enqueue(&send_que[qid], &tmpsend_count, (void **)tmpsendbuf);
      }
    }
  }
 
  // There is no previous worker here, we should add remainning work to corresponding queue anyway. 
  if (tmp_count > 0) {      
    enqueue(&compress_que[qid], &tmp_count, (void **)tmpbuf);
  }
    
  if (tmpsend_count > 0) {      
    enqueue(&send_que[qid], &tmpsend_count, (void **)tmpsendbuf);
  }

  //Used to tell next stage that one more chunkProcess thread ends
  queue_signal_terminate(&compress_que[qid]);
    return NULL;
}

/* 
 * Use rabin fingerprint to find all the anchors.
 */
void *
FindAllAnchors(void * targs) {
  struct thread_args *args = (struct thread_args *)targs;
  const int qid = args->tid / MAX_THREADS_PER_QUEUE;
  data_chunk * fetchbuf[MAX_PER_FETCH];
  int fetch_count = 0;
  int fetch_start = 0;
  
  fprintf(stderr, "%d : FindAllAnchors\n", getpid()); 
  u32int * rabintab = malloc(256*sizeof rabintab[0]);
  u32int * rabinwintab = malloc(256*sizeof rabintab[0]);
  if(rabintab == NULL || rabinwintab == NULL) {
    EXIT_TRACE("Memory allocation failed.\n");
  }

  data_chunk * tmpbuf[CHUNK_ANCHOR_PER_INSERT];
  int tmp_count = 0;

    while (1) {
      data_chunk item;
      //if no item for process, get a group of items from the pipeline
      if (fetch_count == fetch_start) {
		// Fetch some items from anchor_queue
        int r = dequeue(&anchor_que[qid], &fetch_count, (void **)fetchbuf);
	    // If r is -1, that means no item anymore??? 
        if (r < 0) {
          break;
        }
        
        fetch_start = 0;
      }
         
      // Fetch_start is used to identify first item that we can process. Fetch_count is 
      // the number of items that we fetch from the queue 
      if (fetch_start < fetch_count) {
          //get one item
          item.start = fetchbuf[fetch_start]->start;
          item.len = fetchbuf[fetch_start]->len;
          item.anchorid = fetchbuf[fetch_start]->anchorid;
		  // What? We don't need to do this, since it is not a circular buffer. But there is no harm to do this???
          fetch_start = (fetch_start + 1)%MAX_PER_FETCH;
               
          rabininit(rf_win, rabintab, rabinwintab);
          
          u_char * p;
          
          u_int32 chcount = 0;
          // First, anchor and p will have the same value in the beginning.
          u_char * anchor = item.start;
          p = item.start;
		  // n is 60K in the beginning
          int n = MAX_RABIN_CHUNK_SIZE;
          while(p < item.start+item.len) {
			// Check whether current item is smaller than specified size?
            if (item.len + item.start - p < n) {
              n = item.len +item.start -p;
            }
            //find next anchor
            p = p + rabinseg(p, n, rf_win, rabintab, rabinwintab);
          
            //insert into anchor queue: (anchor, p-src+1)
            tmpbuf[tmp_count] = (data_chunk *)malloc(sizeof(data_chunk));
            if(tmpbuf[tmp_count] == NULL) {
              EXIT_TRACE("Memory allocation failed.\n");
            }

			// Report some race condition here.
            tmpbuf[tmp_count]->start = (u_char *)malloc(p - anchor + 1);
            //fprintf(stderr, "anchor %p to %p\n", tmpbuf[tmp_count]->start, tmpbuf[tmp_count]->start+(p - anchor + 1));

            if(tmpbuf[tmp_count]->start == NULL) {
              EXIT_TRACE("Memory allocation failed.\n");
            }
            tmpbuf[tmp_count]->len = p-anchor;
            tmpbuf[tmp_count]->anchorid = item.anchorid;
            tmpbuf[tmp_count]->cid = chcount;
              
            chcount++;              
            chunks_per_anchor[item.anchorid] = chcount;
            memcpy(tmpbuf[tmp_count]->start, anchor, p-anchor);
            tmpbuf[tmp_count]->start[p-anchor] = 0;
            tmp_count ++;                            

            if (tmp_count >= CHUNK_ANCHOR_PER_INSERT) {
              enqueue(&chunk_que[qid], &tmp_count, (void **)tmpbuf);
            }
              
            anchor = p;
          } 

          //insert the remaining item to the anchor queue
          if (item.start + item.len - anchor > 0) {
            
            tmpbuf[tmp_count] = (data_chunk*)malloc(sizeof(data_chunk));
            if(tmpbuf[tmp_count] == NULL) {
              EXIT_TRACE("Memory allocation failed.\n");
            }

            tmpbuf[tmp_count]->start = (u_char *)malloc(item.start + item.len - anchor + 1);
			
            if(tmpbuf[tmp_count] == NULL) {
              EXIT_TRACE("Memory allocation failed.\n");
            }
            tmpbuf[tmp_count]->len = item.start + item.len -anchor;
            tmpbuf[tmp_count]->cid = chcount;
            tmpbuf[tmp_count]->anchorid = item.anchorid;
            chcount ++;
            chunks_per_anchor[item.anchorid] = chcount;
            memcpy(tmpbuf[tmp_count]->start, anchor, item.start + item.len -anchor);
            tmpbuf[tmp_count]->start[item.start + item.len -anchor] = 0;
            tmp_count ++;
            
            if (tmp_count >= CHUNK_ANCHOR_PER_INSERT) {
              enqueue(&chunk_que[qid], &tmp_count, (void **)tmpbuf);
            }
          }

          MEM_FREE(item.start);
        }
    }  

    if (tmp_count > 0) {
      enqueue(&chunk_que[qid], &tmp_count, (void **)tmpbuf);
    }

    /*  for (i = 0; i < MAX_PER_FETCH; i ++) {
    MEM_FREE(fetchbuf[i]);
  }
  for (i = 0; i < CHUNK_ANCHOR_PER_INSERT; i ++) {
    MEM_FREE(tmpbuf[i]);
    }*/

    //count the end thread
    queue_signal_terminate(&chunk_que[qid]);
  return NULL;
}

/* 
 * Integrate all computationally intensive pipeline
 * stages to improve cache efficiency.
 */
void *
SerialIntegratedPipeline(void * targs) {
  struct thread_args *args = (struct thread_args *)targs;
  const int qid = args->tid / MAX_THREADS_PER_QUEUE;
  data_chunk * fetchbuf[MAX_PER_FETCH];
  int fetch_count = 0;
  int fetch_start = 0;

  u32int * rabintab = malloc(256*sizeof rabintab[0]);
  u32int * rabinwintab = malloc(256*sizeof rabintab[0]);
  if(rabintab == NULL || rabinwintab == NULL) {
    EXIT_TRACE("Memory allocation failed.\n");
  }

  data_chunk * tmpbuf;
  int tmpsend_count = 0;
  send_buf_item * senditem;
  send_buf_item * tmpsendbuf[ITEM_PER_INSERT];

    while (1) {
      data_chunk item;
      //if no item for process, get a group of items from the pipeline
      if (fetch_count == fetch_start) {
        int r = dequeue(&anchor_que[qid], &fetch_count, (void **)fetchbuf); 
        if (r < 0) break;
        
        fetch_start = 0;
      }
          
        if (fetch_start < fetch_count) {
          //get one item
          item.start = fetchbuf[fetch_start]->start;
          item.len = fetchbuf[fetch_start]->len;
          item.anchorid = fetchbuf[fetch_start]->anchorid;
          fetch_start = (fetch_start + 1)%MAX_PER_FETCH;
    
          rabininit(rf_win, rabintab, rabinwintab);
          
          u_char * p;
          
          u_int32 chcount = 0;
          u_char * anchor = item.start;
          p = item.start;
          int n = MAX_RABIN_CHUNK_SIZE;
          while(p < item.start+item.len) {
            if (item.len + item.start - p < n) {
              n = item.len +item.start -p;
            }
            //find next anchor
            p = p + rabinseg(p, n, rf_win, rabintab, rabinwintab);
          
            //insert into anchor queue: (anchor, p-src+1)
            tmpbuf = (data_chunk *)malloc(sizeof(data_chunk));
            if(tmpbuf == NULL) {
              EXIT_TRACE("Memory allocation failed.\n");
            }

            tmpbuf->start = (u_char *)malloc(p - anchor + 1);
            if(tmpbuf->start == NULL) {
              EXIT_TRACE("Memory allocation failed.\n");
            }
            tmpbuf->len = p-anchor;
            tmpbuf->anchorid = item.anchorid;
            tmpbuf->cid = chcount;
              
            chcount++;              
            chunks_per_anchor[item.anchorid] = chcount;
            memcpy(tmpbuf->start, anchor, p-anchor);
            tmpbuf->start[p-anchor] = 0;
              
            senditem = sub_ChunkProcess(*tmpbuf);

            if (senditem->type == TYPE_COMPRESS) {
              sub_Compress(senditem);
            } 

            tmpsendbuf[tmpsend_count] = senditem;
            tmpsend_count ++;
              
            if (tmpsend_count >= ITEM_PER_INSERT) {
              enqueue(&send_que[qid], &tmpsend_count, (void **)tmpsendbuf);
            }

            anchor = p;
          } 

          //insert the remaining item to the anchor queue
          if (item.start + item.len - anchor > 0) {
            
            tmpbuf = (data_chunk*)malloc(sizeof(data_chunk));
            if(tmpbuf == NULL) {
              EXIT_TRACE("Memory allocation failed.\n");
            }

            tmpbuf->start = (u_char *)malloc(item.start + item.len - anchor + 1);
            if(tmpbuf->start == NULL) {
              EXIT_TRACE("Memory allocation failed.\n");
            }
            tmpbuf->len = item.start + item.len -anchor;
            tmpbuf->cid = chcount;
            tmpbuf->anchorid = item.anchorid;
            chcount ++;
            chunks_per_anchor[item.anchorid] = chcount;
            memcpy(tmpbuf->start, anchor, item.start + item.len -anchor);
            tmpbuf->start[item.start + item.len -anchor] = 0;
            
  
            senditem = sub_ChunkProcess(*tmpbuf);
            
            if (senditem->type == TYPE_COMPRESS) {
              sub_Compress(senditem);
            } 
            
            tmpsendbuf[tmpsend_count] = senditem;
            tmpsend_count ++;
            
            if (tmpsend_count >= ITEM_PER_INSERT) {
              enqueue(&send_que[qid], &tmpsend_count, (void **)tmpsendbuf);
            }
          }

          MEM_FREE(item.start);
        }
    }  

    if (tmpsend_count > 0) {
      enqueue(&send_que[qid], &tmpsend_count, (void **)tmpsendbuf);
    }
  
  //count the number of compress threads that have finished
    queue_signal_terminate(&send_que[qid]);
    
  return NULL;
}

/*
 * read file and send it to FindAllAnchor thread
 */
void * 
DataProcess(void * targs){
  struct thread_args *args = (struct thread_args *)targs;
  int qid = 0;
  int fd = args->fd;
  u_long tmp;
  u_char * p;
  //int avg_bsize = 0;
  fprintf(stderr, "%d : DataProcess\n", getpid()); 
  u_char * anchor;
  
  u_char * src = (u_char *)malloc(MAXBUF*2);
  u_char * left = (u_char *)malloc(MAXBUF);
  u_char * new = (u_char *) malloc(MAXBUF);
  if(src == NULL || left == NULL || new == NULL) {
    EXIT_TRACE("Memory allocation failed.\n");
  }
  int srclen, left_bytes = 0;
  char more = 0;

  data_chunk * tmpbuf[ANCHOR_DATA_PER_INSERT];
  int tmp_count = 0;

  int anchorcount = 0;

  u32int * rabintab = malloc(256*sizeof rabintab[0]);
  u32int * rabinwintab = malloc(256*sizeof rabintab[0]);
  if(rabintab == NULL || rabinwintab == NULL) {
    EXIT_TRACE("Memory allocation failed.\n");
  }

  rf_win_dataprocess = 0;
  rabininit(rf_win_dataprocess, rabintab, rabinwintab);
  int n = MAX_RABIN_CHUNK_SIZE;


  //read from the file
  while ((srclen = read(fd, new, MAXBUF)) >= 0) {    
    if (srclen) more = 1;
    else {
      if (!more) break;
      more =0;
    }
    
    memset(src, 0, sizeof(u_char)*MAXBUF);

    if (left_bytes > 0){ 
      memcpy(src, left, left_bytes* sizeof(u_char));
      memcpy(src+left_bytes, new, srclen *sizeof(u_char));
      srclen+= left_bytes;
      left_bytes = 0;
    } else {
      memcpy(src, new, srclen * sizeof(u_char));
    }
    tmp = 0; 
    p = src;
 
    while (tmp < srclen) {
      anchor = src + tmp;          
      p = anchor + ANCHOR_JUMP;
      if (tmp + ANCHOR_JUMP >= srclen) {
        if (!more) {
          p = src + srclen;
        } else {
          //move p to the next 00 point
          n = MAX_RABIN_CHUNK_SIZE;
          memcpy(left, src+tmp, (srclen - tmp) * sizeof(u_char));
          left_bytes= srclen -tmp;
          break;
        }
      } else {
        if (srclen - tmp < n) 
          n = srclen - tmp;
        p = p + rabinseg(p, n, rf_win_dataprocess, rabintab, rabinwintab);        
      }

      tmpbuf[tmp_count] = (data_chunk *)malloc(sizeof(data_chunk));
      if(tmpbuf[tmp_count] == NULL) {
        EXIT_TRACE("Memory allocation failed.\n");
      }

      tmpbuf[tmp_count]->start = (u_char *)malloc(p - anchor + 1);
      if(tmpbuf[tmp_count]->start == NULL) {
        EXIT_TRACE("Memory allocation failed.\n");
      }
      tmpbuf[tmp_count]->len = p-anchor;
      tmpbuf[tmp_count]->anchorid = anchorcount;
      anchorcount ++;
      memcpy(tmpbuf[tmp_count]->start, anchor, p-anchor);
      tmpbuf[tmp_count]->start[p-anchor] = 0;
      tmp_count ++;

	  fprintf(stderr, "%d : try to enqueue\n", getpid());

      //send a group of items into the next queue in round-robin fashion
      if (tmp_count >= ANCHOR_DATA_PER_INSERT) { 
        enqueue(&anchor_que[qid], &tmp_count, (void **)tmpbuf);
	  	fprintf(stderr, "%d : after enqueue 1\n", getpid());
        qid = (qid+1) % args->nqueues;
      }

      tmp += p - anchor;//ANCHOR_JUMP;
    }
  } 

  free(src);

  if (tmp_count >= 0) {
    enqueue(&anchor_que[qid], &tmp_count, (void **)tmpbuf);
    qid = (qid+1) % args->nqueues;
  }

  //terminate all output queues
  for(int i=0; i<args->nqueues; i++) {
    queue_signal_terminate(&anchor_que[i]);
  }

  return 0;
}

/*
 * write blocks to the file
 */
void *
SendBlock(void * targs) 
{
  struct thread_args *args = (struct thread_args *)targs;
  config * conf = args->conf;
  //NOTE: We *must* start with the first queue in order to get the header first
  int qid = 0;
  int fd = 0;
  struct hash_entry * entry; 

  fd = open(conf->outfile, O_CREAT|O_TRUNC|O_WRONLY|O_TRUNC);
  if (fd < 0) {
    perror("SendBlock open");
    return NULL;
  }
  fchmod(fd, S_IRGRP | S_IWUSR | S_IRUSR | S_IROTH);

  send_buf_item * fetchbuf[ITEM_PER_FETCH];
  int fetch_count = 0, fetch_start = 0;
  send_buf_item * item;

  SearchTree T;
  T = TreeMakeEmpty(NULL);
  Position pos;
  struct tree_element * tele;
  struct heap_element * hele;

  int reassemble_count = 0, seq_count = 0, anchor_count = 0;

  send_body * body = NULL;
  send_head * head = NULL;
  
  head = (send_head *)headitem->str;
  if (xwrite(fd, &headitem->type, sizeof(headitem->type)) < 0){
    perror("xwrite:");
    EXIT_TRACE("xwrite type fails\n");
    return NULL;
  }
  int checkbit = CHECKBIT;
  if (xwrite(fd, &checkbit, sizeof(int)) < 0){
    EXIT_TRACE("xwrite head fails\n");
  }
  if (xwrite(fd, head, sizeof(send_head)) < 0){
    EXIT_TRACE("xwrite head fails\n");
  }
  MEM_FREE(head);

  while(1) {
    //get a group of items
    if (fetch_count == fetch_start) {
      //process queues in round-robin fashion
      int r;
      int i=0;
      do {
        r = dequeue(&send_que[qid], &fetch_count, (void **)fetchbuf);
        qid = (qid+1) % args->nqueues;
        i++;
      } while(r<0 && i<args->nqueues);
      if (r<0) {
        break;
      }
      fetch_start = 0;
    }
    item = fetchbuf[fetch_start];
    fetch_start ++;

    if (item == NULL) break;

    switch (item->type) {
    case TYPE_FINGERPRINT:
    case TYPE_COMPRESS:
      //process one item
      body = (send_body *)item->str;
      if (body->cid == reassemble_count && body->anchorid == anchor_count) {
        //the item is the next block to write to file, write it.
#ifdef PARALLEL
        pthread_mutex_t *ht_lock = hashtable_getlock(cache, (void *)item->content);
        pthread_mutex_lock(ht_lock);
#endif
        if ((entry = hashtable_search(cache, (void *)item->content)) != NULL) {
          struct pContent * value = ((struct pContent *)entry->v);
          if (value->tag == TAG_WRITTEN) {
            //if the data has been written, just write SHA-1
#ifdef PARALLEL
            pthread_mutex_unlock(ht_lock);
#endif
            write_file(fd, TYPE_FINGERPRINT, seq_count, body->len, item->content);
          } else {            
            //if the data has not been written, write the compressed data
#ifdef PARALLEL
            while (value->tag == TAG_OCCUPY) {
              pthread_cond_wait(&value->empty, ht_lock);
            }
#endif
            if (value->tag == TAG_DATAREADY) {
              write_file(fd, TYPE_COMPRESS, seq_count, value->len, value->content);
              value->len = seq_count;
              value->tag = TAG_WRITTEN;
              hashtable_change(entry, (void *)value);      
            } else {
              printf("Error: Illegal tag\n");
            }
#ifdef PARALLEL
            pthread_mutex_unlock(ht_lock);
#endif
          }
        } else {
		  //FIXME: LTP, if this happens, whether that means a race condition????? LTP
          printf("Error: Cannot find entry\n");
#ifdef PARALLEL
          pthread_mutex_unlock(ht_lock);
#endif
        }  
        MEM_FREE(body);
        reassemble_count ++;
        if (reassemble_count == chunks_per_anchor[anchor_count]) {
          reassemble_count = 0;
          anchor_count ++;
        }
        seq_count ++;
        //check whether there are more data in order in the queue that can be written to the file        
        pos = TreeFindMin(T);
        //while (p != NULL && p->cid == reassemble_count && p->anchorid == anchor_count) {
        if (pos != NULL && (pos->Element)->aid == anchor_count) {
          tele = pos->Element;
          hele = FindMin(tele->queue);
          while (hele!= NULL && hele->cid == reassemble_count && tele->aid == anchor_count) {
#ifdef PARALLEL
            pthread_mutex_t *ht_lock = hashtable_getlock(cache, (void *)hele->content);
            pthread_mutex_lock(ht_lock);
#endif
            if ((entry = hashtable_search(cache, (void *)hele->content)) != NULL) {
              struct pContent * value = ((struct pContent *)entry->v);
              if (value->tag == TAG_WRITTEN) {
#ifdef PARALLEL
                pthread_mutex_unlock(ht_lock);
#endif
                write_file(fd, TYPE_FINGERPRINT, seq_count, hele->len, hele->content);
              } else {            
#ifdef PARALLEL
                while (value->tag == TAG_OCCUPY) {
                  pthread_cond_wait(&value->empty, ht_lock);
                }
#endif
                if (value->tag == TAG_DATAREADY) {
                  write_file(fd, TYPE_COMPRESS, seq_count, value->len, value->content);
                  value->len = seq_count;
                  value->tag = TAG_WRITTEN;
                  hashtable_change(entry, (void *)value);      
                } else {
                  printf("Error: Illegal tag\n");
                }
#ifdef PARALLEL
                pthread_mutex_unlock(ht_lock);
#endif
              }
            } else {
              printf("Error: Cannot find entry\n");
#ifdef PARALLEL
              pthread_mutex_unlock(ht_lock);
#endif
            }  
            MEM_FREE(body);
            if (hele->content) MEM_FREE(hele->content);
            
            seq_count ++;
            DeleteMin(tele->queue);
            
            reassemble_count ++;
            if (reassemble_count == chunks_per_anchor[anchor_count]) {
              reassemble_count = 0;
              anchor_count ++;
            }
            
            if (IsEmpty(tele->queue)) {
              T = TreeDelete(tele, T);
              pos = TreeFindMin(T);
              if (pos == NULL) break;
              tele = pos->Element;     
            }       
            
            hele = FindMin(tele->queue);
          }
        }
      } else {
        // the item is not the next block to write, put it in a queue.
        struct heap_element * p = (struct heap_element*)malloc(sizeof(struct heap_element));
        if(p == NULL) {
          EXIT_TRACE("Memory allocation failed.\n");
        }

        p->content = item->content;
        p->cid = body->cid;
        p->len = body->len;
        p->type = item->type;
        pos = TreeFind(body->anchorid, T);
        if (pos == NULL) {
          struct tree_element * tree = (struct tree_element*)malloc(sizeof(struct tree_element));
          if(tree == NULL) {
            EXIT_TRACE("Memory allocation failed.\n");
          }

          tree->aid = body->anchorid;
          tree->queue = Initialize(INITIAL_SIZE);
          Insert(p, tree->queue);
          T = TreeInsert(tree, T);
        } else {
          Insert(p, pos->Element->queue);
        }
      }
      break;
    case TYPE_HEAD:
      //write the header
      /* head = (send_head *)item->str;
      if (xwrite(fd, &item->type, sizeof(item->type)) < 0){
        perror("xwrite:");
        EXIT_TRACE("xwrite type fails\n");
        return NULL;
      }
      int checkbit = CHECKBIT;
      if (xwrite(fd, &checkbit, sizeof(int)) < 0){
        EXIT_TRACE("xwrite head fails\n");
      }
      if (xwrite(fd, head, sizeof(send_head)) < 0){
        EXIT_TRACE("xwrite head fails\n");
      }
      MEM_FREE(head);*/
      break;
    case TYPE_FINISH:
      break;
    }
  }

  //write the blocks left in the queue to the file
  pos = TreeFindMin(T);        
  if (pos != NULL) {
    tele = pos->Element;
    hele = FindMin(tele->queue);
    while (hele!= NULL) {
#ifdef PARALLEL
      pthread_mutex_t *ht_lock = hashtable_getlock(cache, (void *)hele->content);
      pthread_mutex_lock(ht_lock);
#endif
      if ((entry = hashtable_search(cache, (void *)hele->content)) != NULL) {
        struct pContent * value = ((struct pContent *)entry->v);
        if (value->tag == TAG_WRITTEN) {
#ifdef PARALLEL
          pthread_mutex_unlock(ht_lock);
#endif
          write_file(fd, TYPE_FINGERPRINT, seq_count, hele->len, hele->content);
        } else {            
#ifdef PARALLEL
          while (value->tag == TAG_OCCUPY) {
            pthread_cond_wait(&value->empty, ht_lock);
          }
#endif
          if (value->tag == TAG_DATAREADY) {
            write_file(fd, TYPE_COMPRESS, seq_count, value->len, value->content);
            value->len = seq_count;
            value->tag = TAG_WRITTEN;
            hashtable_change(entry, (void *)value);      
          } else {
            printf("Error: Illegal tag\n");
          }
#ifdef PARALLEL
          pthread_mutex_unlock(ht_lock);
#endif
        }
      } else {
        printf("Error: Cannot find entry\n");
#ifdef PARALLEL
        pthread_mutex_unlock(ht_lock);
#endif
      }  
      seq_count ++;
      DeleteMin(tele->queue);
      if (IsEmpty(tele->queue)) {
        T = TreeDelete(tele, T);
        pos = TreeFindMin(T);
        if (pos == NULL) break;
        tele = pos->Element;
      }
      hele = FindMin(tele->queue);    
    }
  }
 
  u_char type = TYPE_FINISH;
  if (xwrite(fd, &type, sizeof(type)) < 0){
    perror("xwrite:");
    EXIT_TRACE("xwrite type fails\n");
    return NULL;
  }

  close(fd);

  return NULL;
}

/*--------------------------------------------------------------------------*/
/* Encode
 * Compress an input stream
 *
 * Arguments:
 *   conf:		Configuration parameters
 *
 */
void 
Encode(config * conf)
{
  int32 fd;
  struct stat filestat;

  //queue allocation & initialization
  // Here MAX_THREADS_PER_QUEUE is 4, that means we don't want too many threads 
  // are contending for a queue, so we limit the number of threads that one queue can serve 
  const int nqueues = (conf->nthreads / MAX_THREADS_PER_QUEUE) +
                      ((conf->nthreads % MAX_THREADS_PER_QUEUE != 0) ? 1 : 0);
  chunk_que = malloc(sizeof(struct queue) * nqueues);
  anchor_que = malloc(sizeof(struct queue) * nqueues);
  send_que = malloc(sizeof(struct queue) * nqueues);
  compress_que = malloc(sizeof(struct queue) * nqueues);
  if( (chunk_que == NULL) || (anchor_que == NULL) || (send_que == NULL) || (compress_que == NULL)) {
    printf("Out of memory\n");
    exit(1);
  }
  int threads_per_queue;
  for(int i=0; i<nqueues; i++) {
    if (i < nqueues -1 || conf->nthreads %MAX_THREADS_PER_QUEUE == 0) {
      //all but last queue
      threads_per_queue = MAX_THREADS_PER_QUEUE;
    } else {
      //remaining threads work on last queue
      threads_per_queue = conf->nthreads %MAX_THREADS_PER_QUEUE;
    }    

    //call queue_init with threads_per_queue
    queue_init(&chunk_que[i], QUEUE_SIZE, threads_per_queue);
    queue_init(&anchor_que[i], QUEUE_SIZE, 1);
    queue_init(&send_que[i], QUEUE_SIZE, threads_per_queue);
    queue_init(&compress_que[i], QUEUE_SIZE, threads_per_queue);
  }

  memset(chunks_per_anchor, 0, sizeof(chunks_per_anchor));

  //initialize output file header
  headitem = (send_buf_item *)malloc(sizeof(send_buf_item));
  if(headitem == NULL) {
    EXIT_TRACE("Memory allocation failed.\n");
  }

  headitem->type = TYPE_HEAD;
  send_head * head = (send_head *)malloc(sizeof(send_head));
  if(head == NULL) {
    EXIT_TRACE("Memory allocation failed.\n");
  }

  strcpy(head->filename, conf->infile);
  filecount ++;
  head->fid = filecount;
  chunkcount = 0;
  headitem->str = (u_char * )head;

  //send header to output thread
  //FIXME: This is a hack which will break if the sender doesn't start with the first queue
  /*int fetch_count_tmp = -9999;
  send_buf_item * frombuf[1];
  frombuf[0] = item;
  enqueue(&send_que[0], &fetch_count_tmp, (void **)frombuf);
  */
   /* src file stat */
  if (stat(conf->infile, &filestat) < 0) 
      EXIT_TRACE("stat() %s failed: %s\n", conf->infile, strerror(errno));

    if (!S_ISREG(filestat.st_mode)) 
      EXIT_TRACE("not a normal file: %s\n", conf->infile);

    /* src file open */
    if((fd = open(conf->infile, O_RDONLY | O_LARGEFILE)) < 0) 
      EXIT_TRACE("%s file open error %s\n", conf->infile, strerror(errno));

#ifdef PARALLEL
  /* Variables for 3 thread pools and 2 pipeline stage threads.
   * The first and the last stage are serial (mostly I/O).
   */
  pthread_t threads_anchor[MAX_THREADS],
    threads_chunk[MAX_THREADS],
    threads_compress[MAX_THREADS],
    threads_send, threads_process;

  struct thread_args data_process_args;
  data_process_args.tid = 0;
  data_process_args.nqueues = nqueues;
  data_process_args.fd = fd;
  if (conf->preloading) {
    //Preload entire file if selected by user to reduce I/O during parallel phase
    DataProcess(&data_process_args);
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif
  } else {
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif
    //thread for first pipeline stage (input)
    pthread_create(&threads_process, NULL, DataProcess, &data_process_args);
  }

  fprintf(stderr, "%d: Check dataprocessing's memory now\n", getpid());
  int i;

  //Create 3 thread pools for the intermediate pipeline stages
  struct thread_args chunk_thread_args[conf->nthreads];
  for (i = 0; i < conf->nthreads; i ++) {
    chunk_thread_args[i].tid = i;
    pthread_create(&threads_chunk[i], NULL, ChunkProcess, &chunk_thread_args[i]);
  }

  fprintf(stderr, "%d: Creating chunkprocess\n", getpid());
  struct thread_args anchor_thread_args[conf->nthreads];
  for (i = 0; i < conf->nthreads; i ++) {
     anchor_thread_args[i].tid = i;
     pthread_create(&threads_anchor[i], NULL, FindAllAnchors, &anchor_thread_args[i]);
  }
  fprintf(stderr, "%d: Find all anchors' process\n", getpid());

  struct thread_args compress_thread_args[conf->nthreads];
  for (i = 0; i < conf->nthreads; i ++) {
    compress_thread_args[i].tid = i;
    pthread_create(&threads_compress[i], NULL, Compress, &compress_thread_args[i]);
  }
  fprintf(stderr, "%d: compress process\n", getpid());

  //thread for last pipeline stage (output)
  struct thread_args send_block_args;
  send_block_args.tid = 0;
  send_block_args.nqueues = nqueues;
  send_block_args.conf = conf;
  if (!conf->preloading) {
    pthread_create(&threads_send, NULL, SendBlock, &send_block_args);
  }
  
  fprintf(stderr, "%d: sending block process\n", getpid());

  /*** parallel phase ***/

  //join all threads 
  if (!conf->preloading) {
    pthread_join(threads_process, NULL);
  }

 for (i = 0; i < conf->nthreads; i ++)
    pthread_join(threads_anchor[i], NULL);

  for (i = 0; i < conf->nthreads; i ++)
    pthread_join(threads_chunk[i], NULL);

  for (i = 0; i < conf->nthreads; i ++)
    pthread_join(threads_compress[i], NULL);

  if (conf->preloading) {
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif
    SendBlock(&send_block_args);
  } else {
    pthread_join(threads_send, NULL);
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif
  }
  fprintf(stderr, "doing process\n");

#else //serial version
  struct thread_args generic_args;
  generic_args.tid = 0;
  generic_args.nqueues = nqueues;
  generic_args.fd = fd;
  generic_args.conf = conf;
  if (conf->preloading) {
    DataProcess(&generic_args);
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif
  } else {
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif
    DataProcess(&generic_args);
  }

  //do the processing
  SerialIntegratedPipeline(&generic_args);

  if (conf->preloading) {
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif
    SendBlock(&generic_args);
  } else {
    SendBlock(&generic_args);
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif
  }
  
#endif //PARALLEL

  fprintf(stderr, "dumping scan numbers\n");
  void dump_scannums();
  dump_scannums();

  /* clean up with the src file */
  if (conf->infile != NULL)
    close(fd);
}
