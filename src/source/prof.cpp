#ifdef ENABLE_PROFILING

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   Prof.cpp
 * @brief  Used for profiling.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie
 */


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

#include "prof.h"

#define MAX_BUFFER  32768
double cpu_freq = 23.2750708008;

extern int allocTimes;
extern int cleanupSize;
extern unsigned long *serial_time;

static void get_time(struct timeinfo * ti) {
  unsigned int tlow, thigh;
  
  asm volatile ("rdtsc"
		: "=a"(tlow),
		  "=d"(thigh));
  
  ti->low = tlow;
  ti->high = thigh;
}

static double get_elapsed (struct timeinfo * start, struct timeinfo * stop) {
  double elapsed = 0.0;
  
  elapsed = (double) (stop->low - start->low) + (double) (UINT_MAX)
    * (double) (stop->high - start->high);
  if (stop->low < start->low)
    elapsed -= (double) UINT_MAX;
  
  return elapsed;
}


void start (struct timeinfo *ti) {
  /* Clear the start_ti and stop_ti */
  get_time(ti);
  return;
}

/*
 * Stop timing.
 */
double stop(struct timeinfo * begin, struct timeinfo * end) {
  double elapsed = 0.0;
  struct timeinfo stop_ti;
  
  if (end == NULL) {
    get_time(&stop_ti);
    elapsed = get_elapsed (begin, &stop_ti);
  } else {
    get_time(end);
    elapsed = get_elapsed (begin, end);
  }

  return elapsed;
}

/* Provide a function to turn the elapsed time to microseconds. */
unsigned long elapsed2us (double elapsed) {
  unsigned long us;
  us = (unsigned long) (elapsed / cpu_freq);
  return us;
}

#endif
