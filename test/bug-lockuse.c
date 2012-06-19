#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

/* Global lock */
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

void unit_work(void)
{
	int i;
	for(i = 0; i < 100; i++) ;
}
void * child_thread(void * data)
{
	pthread_mutex_lock(&g_lock);
		
	/* Do 1ms computation work. */
	unit_work();
		
	/* Page access */		
	pthread_mutex_lock(&g_lock);

	return NULL;
} 


int main(int argc,char**argv)
{
	const int nThreads = 8;
	pthread_t waiters[nThreads];
	int i;
	for(i = 0; i < nThreads; i++)
		pthread_create (&waiters[i], NULL, child_thread, NULL);

	for(i = 0; i < nThreads; i++)
		pthread_join (waiters[i], NULL);

	return 0;
}
