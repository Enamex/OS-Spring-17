#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
// #include <math.h>
// #include <memory.h>
// #include "a3/a3.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define NUM_PHILS (5)
#define MAX_WAIT_US ((int)(3e6))
#define MIN_WAIT_US ((int)(1e6))
#define LIFETIME ((int)(50 * 1e5))
#define cast(t, e) ((t)(e))
#define leftOf(id) ((id + (NUM_PHILS - 1)) % NUM_PHILS)
#define rightOf(id) (id) // !FIXME
//((id + 1) % NUM_PHILS)
#define CONCAT_(x, y) x y
#define CONCAT(x, y) CONCAT_(x, y)
#define VA_ARGS(...) , ##__VA_ARGS__
#define println(fmt, ...) \
    do { \
        printf(CONCAT(fmt, "\n") VA_ARGS(__VA_ARGS__)); \
    } while(0)

        // fflush(stdout);
#define _right(i) (forks[i])
#define _left(i) (forks[leftOf(i)])
#define _at(i) (forks[i])

#define _cond_wait_on(x) do { pthread_cond_wait(&(x).cv, &(x).mx); } while(0)
#define lock(x) do{ pthread_mutex_lock(&(x).mx); } while(0)
#define unlock(x) do{ pthread_mutex_unlock(&(x).mx); } while(0)
#define trylock(x) pthread_mutex_trylock(&(x).mx)
#define signal(x) do{ pthread_cond_signal(&(x).cv); } while(0)

#define _INV
#define _IN
#define _OUT
// typedef _Bool bool;

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

typedef enum {
    psThinking = 0,
    psEating,
    psHungry,
} PhilState;

typedef struct {
    // pthread_mutex_t     mx;
    PhilState           state;
} Philosopher;

typedef struct {
    pthread_mutex_t mx;
    pthread_cond_t  cv;
    bool            in_use;
} Fork;

// pthread_cond_t  beacon;
// pthread_mutex_t glock;
PhilState       phil[NUM_PHILS];
Fork            forks[NUM_PHILS];
pthread_t       threads[NUM_PHILS];
bool            running[NUM_PHILS];
int             idx[NUM_PHILS];
time_t start;

time_t randTime() {
    return (MIN_WAIT_US + (rand() % MAX_WAIT_US)) / 100;
}

int time_seconds() {
    time_t now = time(0);
    time_t elapsed = now - start;
    return elapsed;
}

void* life(void* arg) {
    int lifetime = LIFETIME + randTime()*500;
    int id = *(int*)(arg);

    running[id] = true;

    bool hungry_printed = false;

    println("[" KCYN"#%d"KNRM " spawned]\n", id);

    PhilState *self = &phil[id];
    
    *self = psThinking;
    
    int eat_time = 5e5, think_time = 5e5, meals = 0;

    //
    while(lifetime > 0
            || *self == psEating || *self == psThinking
            ) {

        //~ loop INVARIANT { no resource is shared }
        _INV {
            for(int i = 0; i < NUM_PHILS; ++i) {
                if(forks[i].in_use)
                    assert(!((phil[i] == psEating) && (phil[leftOf(i)] == psEating)));
            }
        }

        switch(*self) {
            // finished thinking
            case psThinking: {
                think_time = randTime() * 50;
                println("[%4d] " KCYN"#%d"KNRM " has returned to " KGRN "thinking" KNRM "... (%.2fh)", time_seconds(), id, think_time / (1e6));

                usleep(think_time);
                lifetime -= think_time;

                //! FORK WANT PICKUP
                *self = psHungry;
                hungry_printed = false;
                //! END FORK WANT PICKUP
            } break;
            
            // still hungry
            // starts eating
            case psHungry: {
                if(!hungry_printed) {
                    println("[%4d] " KCYN"#%d"KNRM " " KYEL "wishes" KNRM " to eat.", time_seconds(), id);
                    hungry_printed = true;
                }

                //! FORK PICKUP
                lock(_right(id));
                // _right(id).in_use = true;
                while(trylock(_left(id)) != 0) {
                    // _right(id).in_use = false;
                    _cond_wait_on(_right(id));
                }
                //! have both forks
                _right(id).in_use = true;
                _left(id).in_use = true;
                *self = psEating;
                //! END FORK PICKUP
            } break;

            // finishing eating
            case psEating: {
                eat_time = randTime();
                println("[%4d] " KCYN"#%d"KNRM " has started " KRED "eating" KNRM ". (%.2fh)", time_seconds(), id, eat_time / (1e6));

                usleep(eat_time);
                lifetime -= eat_time;
                ++meals;

                //! FORK PUTDOWN
                //! release both forks
                _left(id).in_use    = false;
                _right(id).in_use   = false;
                *self               = psThinking;
                
                signal(_left(id));
                signal(_right(id));

                unlock(_right(id));
                unlock(_left(id));
                
                //! END FORK PUTDOWN
            } break;
        }
    }

    running[id] = false;

    assert(*self == psHungry);
    println("[%4d] " KCYN"#%d"KNRM " has retired to the Retiring Room, "
            "having eaten " KYEL"%d"KNRM " times.", time_seconds(), id, meals);

    return NULL;
}

void p2_1_loop() {
    // INIT
    for(int i = 0; i < NUM_PHILS; ++i) {
        pthread_mutex_init(&_at(i).mx, NULL);
        pthread_cond_init(&_at(i).cv, NULL);
    }
    // END INIT

    // Philosophers live
    start = time(0);
    
    for(int i = 0; i < NUM_PHILS; ++i) {
        idx[i] = i;
        println("" KCYN"#%d"KNRM " enters dining room...", i);
        pthread_create(&threads[i], NULL, &life, (void*)(&idx[i]));
    }

    // usleep(LIFETIME + 1000);

    // Philosophers die :'(
    for(int i = 0; i < NUM_PHILS; ++i) {
        pthread_join(threads[i], NULL);
    }

    println("Experiments over. Philosophers _dead_ in the Retiring Room.");
}