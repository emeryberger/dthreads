/* 
 * http://www.isi.edu/~faber/cs402/notes/lecture8.pdf
 * Dining-philoshopers problem. Known as deadlock.
 */
#include <pthread.h>
#include <stdio.h>

#define NUM_OF_PHIL 5 /* Number of philosphers */
#define N        NUM_OF_PHIL
#define LEFT(i)  (i)
#define RIGHT(i) (((i)+1) %N)

typedef enum { BUSY, FREE } fork_state;

fork_state state[N];
int count = 0;

void set_busy(int item)
{
	/* If it is not busy, mark it as busy. */
    while(state[item] == FREE) {
	state[item] = BUSY;
        break;
    }
}

void get_left_fork(int mine)
{
    int item = LEFT(mine);
    set_busy(item);	
}

void get_right_fork(int mine)
{
    int item = RIGHT(mine);
	
    set_busy(item);
}

void set_free(int item)
{
    /* If it is not busy, mark it as busy. */
    while(state[item] == BUSY) {
	state[item] = FREE;
        break;
    }
}

void put_left_fork(int mine)
{
    int item = LEFT(mine);
    set_free(item);	
}

void put_right_fork(int mine)
{
    int item = RIGHT(mine);
    set_free(item);
}

//FIXME: try to make work more confortable
void do_some_work()  
{
    int i;
    for(i = 0; i < 0x2000000; i++)
	i = i * 10.0 /5 / 2;
}

void eat(void)
{
    do_some_work(); 
}

void * philosopher(void * para) {
    int mine = (int)para;
    int i = 0;

    while(i < 2) {
	i++;
  //      usleep(mine);
        get_left_fork(mine);
        get_right_fork(mine);

        eat();

        put_left_fork(mine);
        put_right_fork(mine);
    }
    printf("%d is finished\n", mine+1);
    return 0L;
}

int main (int argc, char * argv[])
{
  pthread_t waiters[NUM_OF_PHIL];
  int i;

  /* Create specified philosopher's thread. */
  for(i = 0; i < NUM_OF_PHIL; i++)
	  pthread_create (&waiters[i], 0, philosopher, (void *)i);

  for (i = 0; i < NUM_OF_PHIL; i++) {
    pthread_join (waiters[i], NULL);
  }

  return 0;
}




