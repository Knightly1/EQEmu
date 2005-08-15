#ifndef MMFMUTEX_H
#define MMFMUTEX_H
#include <sys/types.h> // moved before sys/shm.h for freeBSD
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "../common/types.h"

#include "MMF.h"

// the manuals say you have to define this struct your self. 
#ifndef FREEBSD  // for freeBSD
union semun
{
    int val;
    struct semid_ds* buf;
    unsigned short int *array;
    struct seminfo *__buf;
};
#endif  // for freeBSD

class MMFMutex
 {
    public:
        MMFMutex(int iIndex);
        virtual ~MMFMutex();
        bool Lock( uint32 dwTimeout = 0 );
        void Release(const MMF*);
    
    protected:
        int m_id; 
        key_t m_key; 
        pthread_t m_owner;
        sem_t m_semaphore;
        int m_recursive_count;
};
#endif
