#ifndef CONFIG_H
#define CONFIG_H

//Define to 1 to enable parallelization with pthreads
//#define PARALLEL 1

//Define to desired number of threads per queues
//The total number of queues between two pipeline stages will be
//greater or equal to #threads/MAX_THREADS_PER_QUEUE
#define MAX_THREADS_PER_QUEUE 4

#endif//CONFIG_H
