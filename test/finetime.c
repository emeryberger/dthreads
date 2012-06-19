/* Exacttime.c written by Tongping Liu. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "time.h"

static struct timeinfo start_ti, stop_ti;
#define MAX_BUFFER  32768
double cpu_freq = 1.0;

/* Get cpu frequency. */
double get_cpufreq(void)
{
	double freq = 0.0;
	const char searchStr[] = "cpu MHz\t\t: ";
	char line[MAX_BUFFER];
	int fd = open("/proc/cpuinfo", O_RDONLY);
	read (fd, line, MAX_BUFFER);
	char *pos = strstr(line, searchStr);
	if (pos) {
		float f;
		pos += strlen(searchStr);
		sscanf(pos, "%f", &f);
		freq = (double) f * 1000000.0;
	}
	return freq;
}

static void __get_time(struct timeinfo * ti)
{
	unsigned int tlow, thigh;

	asm volatile ("rdtsc"
		  : "=a"(tlow),
		    "=d"(thigh));

	ti->low  = tlow;
	ti->high = thigh;
}

double __count_elapse(struct timeinfo * start, struct timeinfo * stop)
{
	double elapsed = 0.0;

	elapsed = (double)(stop->low - start->low) + (double)(UINT_MAX)*(double)(stop->high - start->high);
	if (stop->low < start->low)
		elapsed -= (double)UINT_MAX;

	return elapsed;
}

double get_elapse(struct timeinfo * start, struct timeinfo * stop)
{
	double elapse = 0.0;
    	elapse = __count_elapse(start, stop);

	return elapse;
}

/* The following functions are exported to user application. Normally, a special case about using this file is like this.
       	start();
       	*****
 	elapse = stop();
		
	time = elapse2us();
 */

void start(struct timeinfo *ti)
{
	/* Clear the start_ti and stop_ti */
	if(cpu_freq == 1.0)
		cpu_freq = get_cpufreq();

	/* Get start time informaiton and store to global start_ti variable. */
	if (ti == NULL) {
		ti = &start_ti;
	}

	__get_time(ti);
	return;
}

/*
 * Stop timing.
 */
double stop(struct timeinfo * begin, struct timeinfo * end)
{
	double elapse = 0.0;

	if (end == NULL) {
		end = &stop_ti;
	}
	__get_time(end);

	if (begin == NULL) {
		begin = &start_ti;
	}

	return get_elapse(begin, end);
}

/* Provide a function to turn the elapse time to microseconds. */
unsigned long elapse2ms(double elapsed)
{
	unsigned long ms;

//	printf("cpu_freq is %f\n", cpu_freq);
//	ms =(unsigned long)(elapsed*1000000.0 /cpu_freq);
//	ms =(unsigned long)(elapsed*1000.0 /cpu_freq);
	ms =(unsigned long)(elapsed*1000000.0 /cpu_freq);
	return(ms);
}

