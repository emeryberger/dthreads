#ifndef _TIMER_H_
#define _TIMER_H_

#include <sys/time.h> 

typedef struct {
	struct	timeval	start, end; 
	float	diff; 
} stimer_t; 

void stimer_tick(stimer_t *timer); 

float stimer_tuck(stimer_t *timer, const char *msg); 

#endif /* _TIMER_H_ */
