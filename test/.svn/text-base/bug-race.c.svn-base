#include <time.h>
#include <pthread.h>
#include <stdio.h>

/* Assume total is 4, then thread 1 executes total++ and thread 2 executes
total++ concurrently. After the execution, the value of total may be either
5 or 6. If total is 5, then we lose one increment.
 */
#define N 100
int total = 0;
int set[N+1];

int is_even(int num)
{
	return (!(num%2));
}

struct pair_compare {
	int lower;
	int upper;
};
struct pair_compare pair[2]; 

void delay (void) {
  volatile double d = 1.0;
  int i;
  for (i = 0; i < 10000; i++) {
    d = d * d + d / d;
  }
}

void * pick_even(void * para)
{
  int i;
  struct pair_compare * cur = (struct pair_compare *)para;
  for (i=cur->lower; i<= cur->upper; i++) {
    if (is_even(i)) {
      delay();
      set[total] = i + total; 
      delay();
      total = total + 1; 
      //	  printf("set[%d] = %d, addr %x\n", total, i, &set[total]);  
    }
  }
  return(NULL);
}

int main (int argc, char * argv[])
{
  pthread_t waiters[2];
  int i;

  /* Initialize pairs to pickup. */
  pair[0].lower = 1; 
  pair[0].upper = N/2; 
  pair[1].lower = N/2+1; 
  pair[1].upper = N;

  /* Create the different threads. */
#if 1
  for (i = 0; i < 2; i++) 
  	pthread_create (&waiters[i], 0, pick_even, (void *)&pair[i]);

  for (i = 0; i < 2; i++) 
    pthread_join(waiters[i], NULL);


#else

  pick_even ((void *) &pair[0]);
  pick_even ((void *) &pair[1]);

#endif

  for(i = 0; i < total; i++)
	printf("%d ", set[i]);
  printf("\n");
  return 0;
}

