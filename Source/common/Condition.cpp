#include "Condition.h"
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <iostream>

using namespace std;
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

Condition::~Condition()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_destroy(&cond);
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
}
