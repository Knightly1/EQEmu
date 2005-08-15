#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <iostream>

int main(int argc, char* argv) {
    using std::cout;
    using std::endl;
    
    key_t shm_key;
    shmid_ds* memuser_ds = new shmid_ds;
    
    bool print = false;
    if (argc >= 2) {
	print = true;
    }
    
    for ( int i = 0; i < 8; i++) {
	memuser_ds->shm_nattch = 0;
	switch (i) {
	    case 0: shm_key = ftok(".", 'I'); break;
	    case 1: shm_key = ftok(".", 'N'); break;
	    case 2: shm_key = ftok(".", 'D'); break;
	    case 3: shm_key = ftok(".", 'S'); break;
	    case 4: shm_key = ftok(".", 'F'); break;
	    case 5: shm_key = ftok(".", 'L'); break;
	    case 6: shm_key = ftok(".", 'M'); break;
	    case 7: shm_key = ftok(".", 'O'); break;
	    default: break;
	}
	int semid = semget(shm_key, 1, 0);
	if(print){
	cout<<"ID="<<semid;
	cout<<" EQEmuShare#:"<<i;
	cout<<" useing_sem=";
	}
	cout<< semctl(semget(shm_key, 1, 0), i, GETVAL, 0);
	if(print){
	cout<<" useing_shm=";
	}
	int shmid = shmget(shm_key, 0, 0); 
	shmctl(shmid, IPC_STAT, memuser_ds);
	cout<<memuser_ds->shm_nattch;
	if (memuser_ds->shm_nattch == 0) {
	    if(print){
	    cout<<" Deleteing stale shares"<<endl;
	    }
	    shmctl(shmid, IPC_RMID, 0);
	    semctl(semget(shm_key, 1, 0), IPC_RMID, 0);
	} else {
	    if(print){
	    cout<<endl;
	    }
	}
	
	
    }
    return 0;
}
