#ifndef __CONDITION_H
#define __CONDITION_H

#ifndef WIN32
#include <pthread.h>
#endif

//Sombody, someday needs to figure out how to implement a condition
//system on windows...


class Condition {
	private:
#ifndef WIN32
		pthread_cond_t cond;
		pthread_mutex_t mutex;
#endif
	public:
		Condition();
		void Signal();
		void SignalAll();
		void Wait();
//		bool TimedWait(unsigned long usec);
		~Condition();
};

#endif

