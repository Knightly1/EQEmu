#ifndef RDTSC_H
#define RDTSC_H


#define CALIBRATE_LOOPS 3
#define SLEEP_TIME 10	//in ms

/*

	This class implementes the highest possibly prescision timer
	which is avaliable on the current archetecture.
	
	On intel, this uses the rdtsc instruction to get the actual 
	clock cycle count, and elsewhere it falls back to gettimeofday

	All calculations are carried out in 64 bit integers.
*/

class RDTSC_Timer {
public:
	RDTSC_Timer();
	RDTSC_Timer(bool start_it);
	
	void start();	//start the timer
	virtual void stop();	//stop the timer
	double getDuration();	//returns the number of miliseconds elapsed
	
	//access functions
	unsigned long long getTicks() { return(_end - _start); }
	static unsigned long long ticksPerMS() { return(_ticsperms); }
	
protected:
	static unsigned long long rdtsc();
	
	unsigned long long _start;
	unsigned long long _end;

protected:
	static void init();
	static bool _inited;
	static unsigned long long _ticsperms;
};

//this is a timer which can be started and stoped many times.
//each time it contributes its counter to a sum, whic is used
//to find net duration.
class RDTSC_Collector : public RDTSC_Timer {
public:
	RDTSC_Collector();
	RDTSC_Collector(bool start_it);
	
	void reset();
	
	void stop();	//stop the timer
	
	double getTotalDuration();	//returns the number of miliseconds elapsed
	double getAverage();
	
	unsigned long long getTotalTicks() { return(_sum); }
	unsigned long long getCount() { return(_count); }
	
protected:
	unsigned long long _sum;
	unsigned long long _count;
};


#endif
