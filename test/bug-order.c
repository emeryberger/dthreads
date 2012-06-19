#include <pthread.h>
#include <stdio.h>
#include <unistd.h> /* for usleep */

/* Atomicity violation bugs copied from "MUVI" paper written by "Lu Shan". */
#define SIZE  100
#define true 0
#define false 1
#define ITERATION 10

struct JSPropertyCache {
  int table;
  int empty; /* whether the table is empty*/
};

pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_lock2 = PTHREAD_MUTEX_INITIALIZER;
struct JSPropertyCache g_cache;

void do_some_work()
{
	int i;
	for(i = 0; i < 0x2000; i++)
		i = i * 10.0 /5 / 2;
}
void * js_FlushPropertyCache(void * pointer)
{
	struct JSPropertyCache * cache = (struct JSPropertyCache *)pointer;
	int i = 0;
	int violated = 0;

	while (i++ < ITERATION)
	{
		pthread_mutex_lock(&g_lock);
		cache->table = 0;
		pthread_mutex_unlock(&g_lock);
		do_some_work();

		pthread_mutex_lock(&g_lock2);
		cache->empty = true;
		pthread_mutex_unlock(&g_lock2); 

		usleep(i%2);

		/* Do some checking */
		pthread_mutex_lock(&g_lock);
		pthread_mutex_lock(&g_lock2);
		if(cache->table != 0 && cache->empty == false) {
			violated = 1;
			pthread_mutex_unlock(&g_lock2);
			pthread_mutex_unlock(&g_lock);
			break;
		}	
		pthread_mutex_unlock(&g_lock);
		pthread_mutex_unlock(&g_lock2);
	}
	if(violated == 1)
		printf("atomicity violation in multivariable detected.\n");
	else
		printf("No violation found!\n");

        return 0L;
}

void * js_PropertyCacheFill(void * pointer)
{
	struct JSPropertyCache * cache = (struct JSPropertyCache *)pointer;
	int i = 0;

	while (i++ < ITERATION)
	{
		pthread_mutex_lock(&g_lock);
		cache->table = 1;
		pthread_mutex_unlock(&g_lock);

		do_some_work();

		pthread_mutex_lock(&g_lock2);
		cache->empty = false;
		pthread_mutex_unlock(&g_lock2); 
		usleep(i%5);
	}

	return 0L;
} 


int main (int argc, char * argv[])
{
  pthread_t waiters[2];
  int i;

  /* Create the FLUSH thread. */
  pthread_create (&waiters[0], 0, js_FlushPropertyCache, (void *)&g_cache);

  /* Create the FILL thread. */
  pthread_create (&waiters[1], 0, js_PropertyCacheFill, (void *)&g_cache);

  for (i = 0; i < 2; i++) {
    pthread_join (waiters[i], NULL);
  }

  return 0;
}
