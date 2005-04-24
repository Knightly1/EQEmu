#include "rdtsc.h"
#include <stdio.h>

#ifdef WIN32
	#include <winsock2.h>
	#include <windows.h>
	#include <sys/timeb.h>
	#include "../common/timer.h"
#else
	#include <unistd.h>
	#include <sys/time.h>
#endif

#ifdef i386
#define USE_RDTSC
#else
	#ifndef WIN32
		#warning RDTSC_Timer cannot use rdtsc on a non-intel platform, using gettimeofday
	#endif
#endif

bool RDTSC_Timer::_inited = false;
unsigned long long RDTSC_Timer::_ticsperms = 0;

RDTSC_Timer::RDTSC_Timer() {
	if(!_inited) {
		//find our clock rate
		RDTSC_Timer::init();
	}
	_start = 0;
	_end = 0;
}

RDTSC_Timer::RDTSC_Timer(bool start_it) {
	if(!_inited) {
		//find our clock rate
		RDTSC_Timer::init();
	}
	if(start_it)
		start();
	else {
		_start = 0;
		_end = 0;
	}
}

unsigned long long RDTSC_Timer::rdtsc() {
	unsigned long long res;
#ifdef USE_RDTSC

#ifdef WIN32
	//untested!
	unsigned long high, low;
	__asm {
		rdtsc
		mov high, edx
		mov low, eax
	}
	res = ((unsigned long long)high)<<32 | low;
#else
	//gnu version
	__asm__ __volatile__ ("rdtsc" : "=A" (res));
#endif
#else
	//fall back to get time of day
	timeval t;
	gettimeofday(&t, NULL);
	res = ((unsigned long long)t.tv_sec) * 1000 + t.tv_usec;
#endif
	return(res);
}

void RDTSC_Timer::init() {
#ifdef USE_RDTSC
	unsigned long long before, after, sum;
	
	int r;
	sum = 0;
	// run an average to increase accuracy of clock rate
	for(r = 0; r < CALIBRATE_LOOPS; r++) {
		before = rdtsc();
		
		//sleep a know duration to figure out clock rate
		usleep(SLEEP_TIME * 1000);	//ms * 1000
		
		after = rdtsc();
		
		sum += after - before;
	}
	
	//ticks per sleep / ms per sleep
	_ticsperms = (sum / CALIBRATE_LOOPS) / SLEEP_TIME;
	
#else
	//if using gettimeofday, this is fixed at 1000
	_ticsperms = 1000;
#endif
//	printf("Tics per milisecond: %llu \n", _ticsperms);
	
	_inited = true;	//only want to do this once	
}

//start the timer
void RDTSC_Timer::start() {
	_start = rdtsc();
	_end = 0;
}

//stop the timer
void RDTSC_Timer::stop() {
	_end = rdtsc();
}

//calculate the elapsed duration
double RDTSC_Timer::getDuration() {
	return(((double)(getTicks())) / double(_ticsperms));
}

RDTSC_Collector::RDTSC_Collector() : RDTSC_Timer() {
	reset();
}

RDTSC_Collector::RDTSC_Collector(bool start_it) : RDTSC_Timer(start_it) {
	reset();
}
	
void RDTSC_Collector::stop() {
	RDTSC_Timer::stop();
	_sum += RDTSC_Timer::getTicks();
	_count++;
}

//calculate the elapsed duration
double RDTSC_Collector::getTotalDuration() {
	return(((double)(getTotalTicks())) / double(_ticsperms));
}

double RDTSC_Collector::getAverage() {
	return(((double)(getTotalTicks())) / double(_ticsperms * _count));
}
	
void RDTSC_Collector::reset() {
	_sum = 0;
	_count = 0;
}





