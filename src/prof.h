#ifndef __PROF_H__
#define __PROF_H__

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
 * @brief  Used for profiling purpose.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef ENABLE_PROFILING

typedef struct timeinfo {
	unsigned long low;
	unsigned long high;
} timeinfo_t;

void start(struct timeinfo *ti);
double stop(struct timeinfo * begin, struct timeinfo * end);
unsigned long elapse2ms(double elapsed);

#define TIMER(x) size_t x##_total; timeinfo_t x##_start
#define COUNTER(x) volatile size_t x##_count;

#define START_TIMER(x) start(&global_data->stats.x##_start)
#define STOP_TIMER(x) global_data->stats.x##_total += elapse2ms(stop(&global_data->stats.x##_start, NULL))
#define PRINT_TIMER(x) fprintf(stderr, " " #x " time: %.4fms\n", (double)global_data->stats.x##_total / 100000.0)

#define INC_COUNTER(x) global_data->stats.x##_count++
#define DEC_COUNTER(x) global_data->stats.x##_count--
#define PRINT_COUNTER(x) fprintf(stderr, " " #x " count: %lu\n", global_data->stats.x##_count);

struct runtime_stats {
	//size_t alloc_count;
	//size_t cleanup_size;
	TIMER(serial);
	COUNTER(commit);
	COUNTER(twinpage);
	COUNTER(suspectpage);
	COUNTER(slowpage);
	COUNTER(dirtypage);
	COUNTER(lazypage);
	COUNTER(shorttrans);
};

#else

struct runtime_stats {};

#define START_TIMER(x)
#define STOP_TIMER(x)
#define PRINT_TIMER(x)

#define INC_COUNTER(x)
#define DEC_COUNTER(x)
#define PRINT_COUNTER(x)

#endif
#endif
