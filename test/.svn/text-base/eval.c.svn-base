#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include "time.h"

#define GLOBAL_PAGES (4*1024)
#define PAGE_SIZE (4*1024)

struct control_info {
	int workload;
	int rounds;
};

/* Global buffer */
char g_buffer[GLOBAL_PAGES * PAGE_SIZE];

/* Global lock */
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

unsigned int overlap_count = 0;

/* 1s work in different machine. */
void unit_work(void)
{
	int i;
	int f,f1,f2;
	struct timeinfo begin, end;

	f1 =12441331;
	f2 = 3245235;
	for (i = 47880000; i > 0; i--) {
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

void * child_thread(void * data)
{
	struct control_info * pCntrl = (struct control_info *)data;
	char * pStart = NULL;
	int pages = 0;
	int i = 0, j = 0;
	int workload = pCntrl->workload;	
	int rounds = pCntrl->rounds;

	fprintf(stderr, "%d : total rounds %d\n", getpid(), rounds);
	while(i < rounds) {
		fprintf(stderr, "%d : now rounds %d\n", getpid(), i);
		pthread_mutex_lock(&g_lock);
		pthread_mutex_unlock(&g_lock);
    	/* Do specified work. */	
		for(j = 0; j < workload; j++)
		{
			/* Do 1ms computation work. */
			unit_work();
		}
		i++;
		continue;
	}

	fprintf(stderr, "in the end, rounds %d\n", i);	
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
	printf("2, number of threads. Not a must, default is 8. It should be set when conflict rate is not 0\n");
}
/* Different input parameters
 */
int main(int argc,char**argv)
{
	double elapse = 0.0;
	int i, j;
	int workload;
	int times; /* Times to run in order to meet the overall workload. */
	char * addr = (char *)((unsigned int)g_buffer+PAGE_SIZE); 
	struct control_info *cntrl;
	int random_seed = time(NULL);

	pthread_mutex_init(&g_lock, NULL);
	
	start(NULL);

	/* Initialize thread parameters */
	if(argc < 2)  {
		print_help();
		exit(1);
	}

	workload = atoi(argv[1]);
	const int thr_num = (argc == 3) ? atoi(argv[2]):8; /* default value for number of threads. */
	pthread_t waiters[thr_num];
	
	cntrl = (struct control_info *)malloc(thr_num * sizeof(struct control_info));	
	if(cntrl == NULL)
	{
		printf("Can not alloc space for control structure\n");
		exit(1);
	}
	memset((void *)cntrl, 0, (thr_num * sizeof(struct control_info)));
	/* Initialize common control information. */
	for(i = 0; i < thr_num; i++)
	{
		struct control_info * control = (struct control_info *)((int)cntrl + i * sizeof(struct control_info));
		control->rounds = 5;
		control->workload = workload;
//		control->workload = i*workload;
	} 

	for(j = 0; j < thr_num; j++) {
		pthread_create (&waiters[j], NULL, child_thread, (void *)&cntrl[j]);
        //printf("%d: %d\n", j, waiters[j]);
     }
	 for(j = 0; j < thr_num; j++) {
		pthread_join (waiters[j], NULL);
	}
	elapse = stop(NULL, NULL);
	
	//printf("overlap %d\n", overlap_count);
	printf("elapse %ld\n", elapse2ms(elapse));
	
	/* Free the spaces. */
	free(cntrl);
	return 0;
}
