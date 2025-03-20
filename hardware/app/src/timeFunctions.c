// File to include time ultility functions

#define _POSIX_C_SOURCE 199309L // For clock time
#include <time.h>

long long get_time_in_ms(void) {
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	long long seconds = spec.tv_sec;
	long long nanoSeconds = spec.tv_nsec;
	long long milliSeconds = seconds * 1000
	+ nanoSeconds / 1000000;
	return milliSeconds;
}

void sleep_for_ms(long long delayInMs) {
	const long long NS_PER_MS = 1000 * 1000;
	const long long NS_PER_SECOND = 1000000000;
	long long delayNs = delayInMs * NS_PER_MS;
	int seconds = delayNs / NS_PER_SECOND;
	int nanoseconds = delayNs % NS_PER_SECOND;
	struct timespec reqDelay = {seconds, nanoseconds};
	nanosleep(&reqDelay, (struct timespec *) NULL);
}