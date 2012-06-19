/* 
 * pages.cs.wisc.edu/~remzi/Classes/537/Fall2005/Lectures/lecture8.pdf
 * Simple example as deadlock.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> /* for usleep */

int A = 0;
int B = 0;
pthread_mutex_t lock_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_B = PTHREAD_MUTEX_INITIALIZER;

void * thread_a(void * para)
{
	int i = 0;
	srandom(time(0L));

	while(i < 100)
	{
		i++;
		usleep(random()%100);
		pthread_mutex_lock(&lock_A);
		A += 10;

		pthread_mutex_lock(&lock_B);
		B += 20;
		A += B;

		pthread_mutex_unlock(&lock_B);
		A += 30;

		pthread_mutex_unlock(&lock_A);
		usleep(random()%100);
	}
	printf("THREADA A:%d, B:%d\n", A, B);
	return 0L;
}



void * thread_b(void * para)
{
	int i = 0;
	srandom(time(0L));
	while(i < 100)
	{
		i++;
		usleep(random()%100);
		pthread_mutex_lock(&lock_B);
		B += 10;

		pthread_mutex_lock(&lock_A);
		A += 20;
		A += B;

		pthread_mutex_unlock(&lock_A);
		B += 30;

		pthread_mutex_unlock(&lock_B);
		usleep(random()%100);
	}
	printf("THREADB A:%d, B:%d\n", A, B);
	return 0L;
}

int main (int argc, char * argv[])
{
  pthread_t waiters[2];
  int i;

  /* Create specified philosopher's thread. */
  pthread_create (&waiters[0], 0, thread_a, NULL);
  pthread_create (&waiters[1], 0, thread_b, NULL);

  for (i = 0; i < 2; i++) {
    pthread_join (waiters[i], NULL);
  }

  return 0;
}




