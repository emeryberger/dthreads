#ifndef __QUEUE__
#define __QUEUE__

#define QUEUE_HEAD(node_t) \
	struct { \
		int depth; \
		int count; \
		node_t head; \
		node_t *tail; \
		pthread_mutex_t mutex; \
		pthread_cond_t not_empty; \
		pthread_cond_t not_full; \
	}

#define QUEUE_LINK	__next

/* all macros work on pointers */

#define QUEUE_INIT(queue)	\
	do {			\
		memset(queue, 0, sizeof *queue); \
		(queue)->tail = &(queue)->head; \
		pthread_mutex_init(&(queue)->mutex, NULL); \
	} while (0)

#define QUEUE_INIT_DEPTH(queue,d)	\
	do {			\
		QUEUE_INIT(queue);	\
		(queue)->depth = d;	\
		pthread_cond_init(&(queue)->not_empty, NULL); \
		pthread_cond_init(&(queue)->not_full, NULL); \
	} while (0)

#define queue_destroy(queue)	\
	do {			\
		while ((queue)->head.__next != NULL) {	\
			(queue)->tail = (queue)->head.__next; \
			(queue)->head.__next = (queue)->tail->__next; \
			free((queue)->tail); \
		} \
		pthread_mutex_destroy(&(queue)->mutex); \
	} while (0)

#define queue_enqueue(queue,node) \
	do {			\
		(node)->__next = NULL; \
		(queue)->tail->__next = (node); \
		(queue)->tail = (node); \
		(queue)->count++; \
	} while (0)

#define queue_push(queue,node) \
	do {			\
		(node)->__next = (queue)->__next; \
		if ((queue)->tail == &(queue)->head) (queue)->tail = (node); \
		(queue)->head.__next = (node); \
		(queue)->count++; \
	} while (0)

#define queue_dequeue(queue, node) \
	do {			\
		if ((queue)->count <= 0) { *(node) = NULL; break; } \
		*(node) = (queue)->head.__next; \
		(queue)->head.__next = (*(node))->__next; \
		(queue)->count--;	\
		if ((queue)->count == 0) (queue)->tail = &(queue)->head; \
	} while(0)
	

#define queue_peek(queue)	((queue)->head.__next)

#define queue_lock(queue)	pthread_mutex_lock(&(queue)->mutex)

#define queue_unlock(queue)	pthread_mutex_unlock(&(queue)->mutex)

#define queue_enqueue_sync(queue,node) \
	do {	\
		pthread_mutex_lock(&(queue)->mutex); \
		queue_enqueue(queue,node);	\
		pthread_mutex_unlock(&(queue)->mutex); \
	} while(0)

#define queue_push_sync(queue,node) \
	do {	\
		pthread_mutex_lock(&(queue)->mutex); \
		queue_push(queue,node);	\
		pthread_mutex_unlock(&(queue)->mutex); \
	} while(0)

#define queue_dequeue_sync(queue, node)	\
	do {	\
		pthread_mutex_lock(&(queue)->mutex); \
		queue_dequeue(queue,node);	\
		pthread_mutex_unlock(&(queue)->mutex); \
	} while(0)

#define queue_rotate(queue, node) \
	do {	\
		if ((queue)->head.__next != NULL) { \
			queue_dequeue(queue,node);	\
			queue_enqueue(queue,*(node));	\
		} \
	} while(0)

#define queue_rotate_sync(queue, node) \
	do { \
		pthread_mutex_lock(&(queue)->mutex); \
		queue_rotate(queue,node);	\
		pthread_mutex_unlock(&(queue)->mutex); \
	} while (0)

#define queue_enqueue_wait(queue, node) \
	do {\
		pthread_mutex_lock(&(queue)->mutex); \
		while((queue)->count >= (queue)->depth) pthread_cond_wait(&(queue)->not_full, &(queue)->mutex); \
		queue_enqueue(queue,node);	\
		if ((queue)->count > 0) pthread_cond_signal(&(queue)->not_empty); \
		pthread_mutex_unlock(&(queue)->mutex); \
	} while (0)

#define queue_dequeue_wait(queue, node) \
	do {\
		pthread_mutex_lock(&(queue)->mutex); \
		while((queue)->count == 0) pthread_cond_wait(&(queue)->not_empty, &(queue)->mutex); \
		queue_dequeue(queue,node);	\
		if ((queue)->count < (queue)->depth) pthread_cond_signal(&(queue)->not_full); \
		pthread_mutex_unlock(&(queue)->mutex); \
	} while (0)

#endif


