#ifndef __CONDITION_H

#define __CONDITION_H

#include <pthread.h>

class Condition {
	private:
		pthread_cond_t cond;
		pthread_mutex_t mutex;
	public:
		Condition();
		void Signal();
		void SignalAll();
		void Wait();
		bool TimedWait(unsigned long usec);
		~Condition();
};

#endif
