
#include "debug.h"
#include "Condition.h"

#ifdef WIN32
#else
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#endif

using namespace std;

#ifdef WIN32
/*

	Windows does not support condition variables by default.
	So we use a simple hack of sleeping in wait() and doing
	nothing anywhere else. 
	
	some possible places to look for ways to do this:
	http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
	
	http://sources.redhat.com/pthreads-win32/
	http://sources.redhat.com/cgi-bin/cvsweb.cgi/pthreads/pthread_cond_signal.c?rev=1.7&content-type=text/x-cvsweb-markup&cvsroot=pthreads-win32
	
*/

#define CONDITION_HACK_GRANULARITY 4


Condition::Condition() 
{
}

void Condition::Signal()
{
}

void Condition::SignalAll()
{
}

void Condition::Wait()
{
	Sleep(CONDITION_HACK_GRANULARITY);
}

Condition::~Condition()
{
}


#else	//!WIN32

Condition::Condition() 
{
	pthread_cond_init(&cond,NULL);
	pthread_mutex_init(&mutex,NULL);
}

void Condition::Signal()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

void Condition::SignalAll()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
}

void Condition::Wait()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond,&mutex);
	pthread_mutex_unlock(&mutex);
}

/*
I commented this specifically because I think it might be very
difficult to write a windows counterpart to it, so I would like
to discourage its use until we can confirm that it can be reasonably
implemented on windows.

bool Condition::TimedWait(unsigned long usec)
{
struct timeval now;
struct timespec timeout;
int retcode=0;
	pthread_mutex_lock(&mutex);
	gettimeofday(&now,NULL);
	now.tv_usec+=usec;
	timeout.tv_sec = now.tv_sec + (now.tv_usec/1000000);
	timeout.tv_nsec = (now.tv_usec%1000000) *1000;
	//cout << "now=" << now.tv_sec << "."<<now.tv_usec << endl;
	//cout << "timeout=" << timeout.tv_sec << "."<<timeout.tv_nsec << endl;
	retcode=pthread_cond_timedwait(&cond,&mutex,&timeout);
	pthread_mutex_unlock(&mutex);

	return retcode!=ETIMEDOUT;
}
*/

Condition::~Condition()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_destroy(&cond);
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
}

#endif
