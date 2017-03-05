#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <math.h>

#include "timer.h"

double time_now()
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);

	return t.tv_sec + (1e-9 * t.tv_nsec);
}

void sleep_for(const double duration)
{
	struct timespec req;
	struct timespec rem;

	double full_secs;
	req.tv_nsec = (long)(modf(duration, &full_secs) * 1e9);
	req.tv_sec = (time_t)full_secs;

	while(nanosleep(&req, &rem) != 0) {
		req = rem;
	}
}
