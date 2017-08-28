#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "RWLock.h"
#include "script.h"

const int READ_LOCK = 1;
const int TRY_READ_LOCK = 2;
const int READ_UNLOCK = 3;
const int WRITE_LOCK = 4;
const int TRY_WRITE_LOCK = 5;
const int WRITE_UNLOCK = 6;
const int WRITE_TO_READ = 7;

struct action {
    int  who;
    int  what;
};

// Test script one, testing basic read/write functionalities    
struct action sone[6] = {
    {1, READ_LOCK},     // T1 calls read_lock, should succeed.
    {2, READ_LOCK},     // T2 calls read_lock, should succeed.
    {3, WRITE_LOCK},    // T3 calls write_lock, should be blocked.
    {1, READ_UNLOCK},   // T1 calls read_unlock.
    {2, READ_UNLOCK},   // T2 calls read_unlock, should enable T2 to get write lock.
    {3, WRITE_UNLOCK},  // T3 calls write_unlock.
};

// Test script two, testing FCFS, a blocked writer should block later readers
struct action stwo[10] = {
    {1, READ_LOCK},
    {2, READ_LOCK},
    {3, WRITE_LOCK},    // T3 should be blocked
    {4, READ_LOCK},     // T4 should be blocked too
    {1, READ_UNLOCK},
    {1, READ_LOCK},     // T1 should be blocked as well
    {2, READ_UNLOCK},   // T3 should obtain write lock
    {3, WRITE_UNLOCK},  // T4 and T1 should both get read lock here.
    {4, READ_UNLOCK},
    {1, READ_UNLOCK}
};

// Test script three, testing FCFS
struct action sthree[10] = {
    {1, WRITE_LOCK},
    {2, READ_LOCK}, //T2 blocked
    {3, READ_LOCK}, //T3 blocked
    {4, WRITE_LOCK},//T4 blocked
    {5, READ_LOCK}, //T5 blocked
    {1, WRITE_UNLOCK},//T2, T3 get locked
    {2, READ_UNLOCK}, //T3 still there
    {3, READ_UNLOCK}, //after this T4 gets locked
    {4, WRITE_UNLOCK},//T5 get locked
    {5, READ_UNLOCK}
};

// Test script four, testing FCFS
struct action sfour[10] = {
    {1, WRITE_LOCK},
    {2, READ_LOCK},
    {3, WRITE_LOCK},
    {1, WRITE_UNLOCK},
    {1, READ_LOCK},
    {2, READ_UNLOCK},
    {2, WRITE_LOCK},
    {3, WRITE_UNLOCK},
    {1, READ_UNLOCK},
    {2, WRITE_UNLOCK}
};

// Test script five, testing error correction
struct action sfive[12] = {
    {1, READ_LOCK},
    {2, READ_LOCK},
    {3, READ_LOCK},
    {4, WRITE_LOCK},
    {1, READ_LOCK},    // Calling read_lock while holding read_lock	
    {5, READ_UNLOCK},  // Calling read_unlock while holding no lock
    {3, WRITE_LOCK},   // Calling write_lock while holding read lock
    {3, WRITE_UNLOCK}, // Calling write_unlock while holding read lock
    {3, READ_UNLOCK},
    {2, READ_UNLOCK},
    {1, READ_UNLOCK},
    {4, WRITE_UNLOCK}
};

// Test script six, testing error correction
struct action ssix[14] = {
    {1, WRITE_LOCK},
    {2, READ_LOCK},
	{3, READ_LOCK},
    {4, WRITE_LOCK}, 
    {5, WRITE_UNLOCK}, // Calling write_unlock while holding no lock
    {1, READ_LOCK},    // Calling read_lock while holding write lock
    {1, READ_UNLOCK},  // Calling read_unlock while holding write lock
    {1, WRITE_LOCK},   // Calling write_lock while holding write lock
    {1, WRITE_UNLOCK}, // Allows 2,3 to hold READ_LOCK
    {2, WRITE_UNLOCK}, // Calling write_unlock while holding read lock
    {2, READ_LOCK},    // Calling read_lock while holding read lock
    {2, READ_UNLOCK},
    {3, READ_UNLOCK},
    {4, WRITE_UNLOCK}
};

// Test script seven, testing try_read_lock and try_write_lock
struct action sseven[12] = {
    {1, READ_LOCK},
    {2, TRY_READ_LOCK},
    {3, TRY_WRITE_LOCK},    // return without holding lock
    {4, TRY_READ_LOCK},     // succeeds in getting read lock
    {3, WRITE_LOCK},        // blocked 
    {5, TRY_READ_LOCK},     // Unable to obtain read lock
    {4, READ_UNLOCK},   
    {1, READ_UNLOCK},   
    {2, READ_UNLOCK},
    {3, WRITE_UNLOCK},
    {5, TRY_WRITE_LOCK},
    {5, WRITE_UNLOCK}
};

// Test script eight, testing try_read_lock, try_write_lock, with error checking
struct action seight[13] = {
    {1, READ_LOCK},
    {1, TRY_READ_LOCK},
    {2, TRY_WRITE_LOCK},    // return without holding lock
    {1, READ_UNLOCK},     
    {2, TRY_WRITE_LOCK},        
    {2, TRY_READ_LOCK},     // Error
    {3, READ_LOCK},   
    {2, TRY_WRITE_LOCK},    // Error
    {2, WRITE_UNLOCK},
    {3, TRY_WRITE_LOCK},    // Error
    {1, TRY_READ_LOCK},
    {1, READ_UNLOCK},
    {3, READ_UNLOCK}
};

// Test script nine, testing write_to_read
struct action snine[18] = {
    {1, READ_LOCK},
    {2, WRITE_LOCK}, //blocked    
    {3, READ_LOCK},     //blocked
    {4, READ_LOCK},        //blcoked
    {5, WRITE_LOCK},     // Error - blocked
    {1, READ_UNLOCK},   	//1, 2 get locked
    {1, WRITE_LOCK},    // Error //blocked after 2
    {2, WRITE_TO_READ}, //2 is now read
    {4, READ_UNLOCK},    // Error //should return 0
    {4, WRITE_LOCK}, //gets blocked by 2, 3
    {2, READ_UNLOCK},
    {3, READ_UNLOCK},//should wake write 5
    {3, READ_LOCK},
    {5, WRITE_TO_READ},
	{5, READ_UNLOCK},
	{1, WRITE_UNLOCK},
    {4, WRITE_UNLOCK},
    {3, READ_UNLOCK}
};

struct action sten[50] = {
    RANDOM_SCRIPT
};

struct script {
    int n;         // number of threads
    int s;         // number of steps
    struct action  *steps;
};

const int num_scripts = 10;
const int MAX_NUM_THREADS = 10;
struct script scripts[num_scripts] = {
   {3,  6, sone},
   {4, 10, stwo},
   {5, 10, sthree},
   {3, 10, sfour},
   {5, 12, sfive},
   {5, 14, ssix},
   {5, 12, sseven},
   {3, 13, seight},
   {5, 18, snine},
   {10, 50, sten}
};

struct script *code;
int ip = 0;
RWLock lock;

int counter = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* actor(void *arg)
{
    int id, res;
    
    // Obtain an id for current thread
    pthread_mutex_lock(&mutex);
    id = counter;
    counter++;
    pthread_mutex_unlock(&mutex);

    // Running through the steps in the transcript
    while (ip < code->s) {
        struct action *act = & code->steps[ip];
        if (id != act->who) { // Wait until current thread need to act
            pthread_yield();
            continue;
        }
        sleep(1);
        ip++;
        switch (act->what) {
        case READ_LOCK:
            printf("Thread %d calls read_lock. \n", id);
            res = lock.read_lock();
            printf("  Thread %d read_lock returns %d. \n", id, res);
            break;
        case TRY_READ_LOCK:
            printf("Thread %d calls try_read_lock. \n", id);
            res = lock.try_read_lock();
            printf("  Thread %d try_read_lock returns %d. \n", id, res);
            break;
        case READ_UNLOCK:
            printf("Thread %d calls read_unlock. \n", id);
            res = lock.read_unlock();
            printf("  Thread %d read_unlock returns %d. \n", id, res);
            break;
        case WRITE_LOCK:
            printf("Thread %d calls write_lock. \n", id);
            res = lock.write_lock();
            printf("  Thread %d write_lock returns %d. \n", id, res);
            break;
        case TRY_WRITE_LOCK:
            printf("Thread %d calls try_write_lock. \n", id);
            res = lock.try_write_lock();
            printf("  Thread %d try_write_lock returns %d. \n", id, res);
            break;
        case WRITE_UNLOCK:
            printf("Thread %d calls write_unlock. \n", id);
            res = lock.write_unlock();
            printf("  Thread %d write_unlock returns %d. \n", id, res);
            break;
        case WRITE_TO_READ:
            printf("Thread %d calls write_to_read. \n", id);
            res = lock.write_to_read();
            printf("  Thread %d write_to_read returns %d. \n", id, res);
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) 
{
    pthread_t threads[MAX_NUM_THREADS];
    int sid = -1;
    if (argc == 2) {
        sid = atoi(argv[1]) - 1;
    }
    if (sid < 0 || sid >= num_scripts) {
        fprintf(stderr, "Usage: %s sid\n     where sid is between 1 and %d.\n ", argv[0], num_scripts); 
        return 1;
    }
    code = &scripts[sid];
    if (code->n > MAX_NUM_THREADS) {
        fprintf(stderr, "Script has too many threads.\n");
    }
    for (int i=0; i<code->n; i++) {
        pthread_create(&threads[i], NULL, actor, NULL);
    }
    for (int i=0; i<code->n; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
  

