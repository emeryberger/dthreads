#ifndef __TIME_H__
#define __TIME_H__

struct timeinfo {
	unsigned long low;
	unsigned long high;
};

void start(struct timeinfo *ti);
double stop(struct timeinfo * begin, struct timeinfo * end);
unsigned long elapse2ms(double elapsed);
double get_cpufreq(void);

#endif /* __TIME_H__ */
