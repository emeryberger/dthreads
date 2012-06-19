#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include "time.h"

#define GLOBAL_PAGES (4*1024)
#define PAGE_SIZE (4*1024)

struct control_info {
	int rounds;  /* How many rounds of each unit. */
	int bWrite;  /* Whether to write the private area in order to introduce footprint on pages */
	int pages;   /* How many pages? If bWrite is 1, dirty pages. */
	char * addr; /* Start address of share area. */
    int bComputeFirst;
	int conflict;   /* Conflict rate */
};

/* Global buffer */
char g_buffer[GLOBAL_PAGES * PAGE_SIZE];

/* Global lock */
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

unsigned int overlap_count = 0;

/* 1ms work in different machine. */
void unit_work(void)
{
	int i;
	int f,f1,f2;
	struct timeinfo begin, end;

	f1 =12441331;
	f2 = 3245235;
	for (i = 47880; i > 0; i--) {
//	for (i = 13800; i > 0; i--) {
                f *= f1;        /*    x    x     1/x */
                f1 *= f2;       /*    x    1     1/x */
                f1 *= f2;       /*    x    1/x   1/x */
                f2 *= f;        /*    x    1/x   1   */
                f2 *= f;        /*    x    1/x   x   */
                f *= f1;        /*    1    1/x   x   */
                f *= f1;        /*    1/x  1/x   x   */
                f1 *= f2;       /*    1/x  1     x   */
                f1 *= f2;       /*    1/x  x     x   */
                f2 *= f;        /*    1/x  x     1   */
                f2 *= f;        /*    1/x  x     1/x */
                f *= f1;        /*    1    x     1/x */
        }
}

void dirty_pages(char * start, int num, int content)
{
	int i;

	if(start == NULL || num == 0)
		return;

	for(i = 0; i < num; i++)
		start[i*PAGE_SIZE] = content;
	return;
}

void * child_thread(void * data)
{
//    printf("CHILD: %d\n", getpid());
	struct control_info * pCntrl = (struct control_info *)data;
	int rounds;
	char * pStart = NULL;
	int pages = 0;
	int i;
	

	pthread_mutex_lock(&g_lock);
	if(pCntrl->conflict) {
		/* Write something to first page of global area in order to introduce some rollbak. */
		g_buffer[1] = i;
	}
	pthread_mutex_unlock(&g_lock);
	
	/* Check whether to write to private pages. */
	if(pCntrl->bWrite)
	{	
		pStart = pCntrl->addr;
		pages  = pCntrl->pages;
	}

	 //printf("in child thread %d\n", getpid());	
	/* Get rounds information */
	rounds = pCntrl->rounds;

    /* Do specified work. */	
	for(i = 0; i < rounds; i++)
	{
		/* Do 1ms computation work. */
		unit_work();
	}
	
	/* Write to pages. */
	dirty_pages(pStart, pages, i);
//    printf("%d is finished\n", getpid());
} 

const long int A = 48271;
const long int M = 2147483647;
static int random2(unsigned int m_seed)
{
    long tmp_seed;
    long int Q = M / A;
    long int R = M % A;
    tmp_seed = A * ( m_seed % Q ) - R * ( m_seed / Q );
    if ( tmp_seed >= 0 )
    {
        m_seed = tmp_seed;
    }
    else
    {
       m_seed = tmp_seed + M;
    }

    return (m_seed%M);
}

void print_help(void)
{
	printf("Please check the input\n");
	printf("1, overall workload with units of ms. Normally 16000.\n");
	printf("2, transaction size\n");
	printf("3, share memory pages\n");
	printf("4, conflict rate\n");
	printf("5, number of threads. Not a must, default is 8. It should be set when conflict rate is not 0\n");
}
/* Different input parameters
 */
int main(int argc,char**argv)
{
	double elapse = 0.0;
	int i, j;
	int workload;
	int tran_size; 
	int mem_pages; 
	int conflict_rate; 
	int times; /* Times to run in order to meet the overall workload. */
	char * addr = (char *)((unsigned int)g_buffer+PAGE_SIZE); 
	struct control_info *cntrl;
	int random_seed = time(NULL);

	pthread_mutex_init(&g_lock, NULL);
	
	start(NULL);

	/* Initialize thread parameters */
	if(argc < 5)  {
		print_help();
		exit(1);
	}

	workload = atoi(argv[1]);
	tran_size = atoi(argv[2]);
	mem_pages = atoi(argv[3]);
	conflict_rate = atoi(argv[4]);
	const int thr_num = (argc == 6) ? atoi(argv[5]):16; /* default value for number of threads. */
	pthread_t waiters[thr_num];
	
//	printf("conflict rate is %d!!!!\n", conflict_rate);
	cntrl = (struct control_info *)malloc(thr_num * sizeof(struct control_info));	
	if(cntrl == NULL)
	{
		printf("Can not alloc space for control structure\n");
		exit(1);
	}
	memset((void *)cntrl, 0, (thr_num * sizeof(struct control_info)));
	//printf("initialize the control informaiton\n");
	/* Initialize common control information. */
	for(i = 0; i < thr_num; i++)
	{
		struct control_info * control = (struct control_info *)((int)cntrl + i * sizeof(struct control_info));
		control->rounds = tran_size;
		control->pages  = mem_pages;
//		printf("%d is %p with pages %d\n", i, control, control->pages);	
		
		if(control->pages > 0)
			control->bWrite = 1;
		else
			control->bWrite = 0;
	
		random_seed = random2(random_seed);
		if(conflict_rate) {

			if(i == 0 || (random_seed%100 < conflict_rate)) {
//				printf("random_seed %d, conflict!!!\n", random_seed);
				overlap_count++;
				control->conflict = 1;
			}
			else {
				control->conflict = 0;
//				printf("random_seed %d, not conflict!!!\n", random_seed);
			}
		}
//		printf(" %d is pages %d\n", i, mem_pages*i);
		control->addr = (char *)((unsigned long)addr + mem_pages*PAGE_SIZE*i);
	} 

	/* Compute the size according to overall workload and tran_size. */
	/* Check whether (thr_num * tran_size) is lower than workload. */
	if((thr_num * tran_size) > workload || (workload%(thr_num * tran_size) != 0)) {
		printf("Workload should be multiple times of the product of thread number and transaction size. \n");
		print_help();
		exit(1);
	}
		
	times = workload/(thr_num * tran_size);
	
	//printf("times is %d\n", times);
	for(i = 0; i < times; i++)
	{
		for(j = 0; j < thr_num; j++) {
			pthread_create (&waiters[j], NULL, child_thread, (void *)&cntrl[j]);
            //printf("%d: %d\n", j, waiters[j]);
        }
		for(j = 0; j < thr_num; j++) {
			pthread_join (waiters[j], NULL);
		}
	}
	elapse = stop(NULL, NULL);
	
	//printf("overlap %d\n", overlap_count);
	printf("%ld\n", elapse2ms(elapse));
	
	/* Free the spaces. */
	free(cntrl);
	return 0;
}
