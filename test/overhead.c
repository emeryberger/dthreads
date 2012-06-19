#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include "time.h"

#define PAGE_SIZE (4096)
#define PAGE_NUM  (100*1024)

void * child_thread(void * data)
{
    char * start = (char *)data;
	int i;

	for(i = 0; i < PAGE_NUM; i++) 
		start[i*PAGE_SIZE] = i;
	return NULL;
}

/* Different input parameters
 */
int main(int argc,char**argv)
{
	double elapse = 0.0;
	char * ptr = NULL;
	unsigned long time;
	pthread_t thread;
		
	// Malloc a huge block of memory. 
	ptr = malloc(PAGE_NUM * PAGE_SIZE);
	if(ptr == NULL) {
		fprintf(stderr, "Cannot allocate memory for test, exit\n");
		exit(-1);
	}

	memset(ptr, 0, PAGE_NUM * PAGE_SIZE);

	// Start a new thread and start to count the time.
	start(NULL);
	pthread_create (&thread, NULL, child_thread, (void *)ptr);
	pthread_join (thread, NULL);
	elapse = stop(NULL, NULL);
	
	time = elapse2ms(elapse);

	//printf("overlap %d\n", overlap_count);
	fprintf(stderr, "Total time %f seconds, per page overhead %d us\n", (double)time/1000000.00, time/PAGE_NUM);
	
	/* Free the spaces. */
	return 0;
}
