#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
// #include <memory.h>

#define NUM_PHILS (5)
#define MAX_WAIT_US ((int)(3e6))
#define MIN_WAIT_US ((int)(1e6))
#define LIFETIME ((int)(50 * 1e6))
#define cast(t, e) ((t)(e))
#define leftOf(id) ((id + (NUM_PHILS - 1)) % NUM_PHILS)
#define rightOf(id) ((id + 1) % NUM_PHILS)
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
#define _my(i) (forks[i])

// typedef _Bool bool;

typedef enum {
    psThinking = 0,
    psEating,
    psHungry,
} PhilState;

typedef struct {
    // mutex of right fork to the philosopher
    // this's just to simplify the code; every philosopher _struct_
    // is assigned a fork and has to use one other fork besides theirs to be able to eat
    // pthread_mutex_t     mx;
    PhilState           state;
} Philosopher;

typedef struct {
    // pthread_mutex_t mx;
    // pthread_cond_t  cv;
    bool            in_use;
} Fork;

pthread_cond_t  beacon;
pthread_mutex_t glock;
PhilState       phil[NUM_PHILS];
Fork            forks[NUM_PHILS];
pthread_t       threads[NUM_PHILS];
time_t start;

time_t randTime() {
    return MIN_WAIT_US + (rand() % MAX_WAIT_US);
}

int time_seconds() {
    time_t now = time(0);
    time_t elapsed = now - start;
    return elapsed;
}

void* life(void* arg) {
    int lifetime = LIFETIME;
    int id = *(int*)(arg);

    bool hungry_printed = false;

    free((int*)arg);

    println("[Thread #%d spawned]", id);

    PhilState *self = &phil[id];
    PhilState *left, *right;
    left = &(phil[leftOf(id)]);
    right = &(phil[id]);
    
    *self = psThinking;
    
    int eat_time = 5e5, think_time = 5e5;

    //
    while(lifetime > 0) {
        switch(*self) {
            // finished thinking
            case psThinking: {
                think_time = randTime();
                lifetime -= think_time;
                println("[%4d] #%d has returned to thinking... (%.2fs)", time_seconds(), id, think_time / (1e6));

                usleep(think_time);

                //! FORK WANT PICKUP
                pthread_mutex_lock(&glock);
                *self = psHungry;
                hungry_printed = false;
                pthread_mutex_unlock(&glock);
                // pthread_mutex_lock(&_left(id).mx);
                // pthread_mutex_lock(&_right(id).mx);
                
                // _left(id).in_use    = false;
                // _right(id).in_use   = false;

                // pthread_mutex_unlock(&_right(id).mx);
                // pthread_mutex_unlock(&_left(id).mx);
                //! END FORK WANT PICKUP
            } break;
            
            // still hungry
            // starts eating
            case psHungry: {
                if(!hungry_printed)
                    println("[%4d] #%d wishes to eat.", time_seconds(), id);
                hungry_printed = true;

                //! FORK PICKUP
                // Something very wrong here...
                // take right chopstick
                
                // pthread_mutex_lock(&_left(id).mx);
                // pthread_mutex_lock(&_right(id).mx);
                pthread_mutex_lock(&glock);
                    bool *liu = &_left(id).in_use, *riu = &_right(id).in_use;

                    // while(*liu ^ *riu) {
                    //     if(*liu) {
                    //         // _right(id).in_use = true;
                    //         while(_left(id).in_use)
                    //             pthread_cond_wait(&beacon, &glock);
                    //     }
                    //     else {
                    //         // _left(id).in_use = true;
                    //         while(_right(id).in_use)
                    //             pthread_cond_wait(&beacon, &glock);
                    //     }
                    // }
                    
                    if(*liu || *riu) {
                        pthread_cond_wait(&beacon, &glock);
                    }
                    else
                    if(!(*liu || *riu)) {
                        _left(id).in_use    = true;
                        _right(id).in_use   = true;
                        *self = psEating;
                    }
                pthread_mutex_unlock(&glock);
                // pthread_mutex_unlock(&_right(id).mx);
                // pthread_mutex_unlock(&_left(id).mx);

                //! END FORK PICKUP
            } break;

            // finishing eating
            case psEating: {
                eat_time = randTime();
                lifetime -= eat_time;
                println("[%4d] #%d has started EATING. (%.2fs)", time_seconds(), id, eat_time / (1e6));

                usleep(eat_time);

                //! FORK PUTDOWN
                // pthread_mutex_lock(&_left(id).mx);
                // pthread_mutex_lock(&_right(id).mx);
                pthread_mutex_lock(&glock);
                
                *self               = psThinking;
                _left(id).in_use    = false;
                _right(id).in_use   = false;

                pthread_mutex_unlock(&glock);
                // pthread_mutex_unlock(&_right(id).mx);
                // pthread_mutex_unlock(&_left(id).mx);
                
                //! END FORK PUTDOWN
            } break;
        }
    }

    println("[%4d] #%d has retired to the Retiring Room.", time_seconds(), id);

    return NULL;
}

void p2_loop() {
    // INIT
    // for(int i = 0; i < NUM_PHILS; ++i) {
    //     pthread_mutex_init(&_my(i).mx, NULL);
    //     pthread_cond_init(&_my(i).cv, NULL);
    // }
    pthread_mutex_init(&glock, NULL);
    pthread_cond_init(&beacon, NULL);
    // END INIT

    // Philosophers live
    start = time(0);
    for(int i = 0; i < NUM_PHILS; ++i) {
        println("#%d taking seat...", i);
        int* id = malloc(sizeof (int));
        *id = i;
        pthread_create(&threads[i], NULL, &life, (void*)id);
    }

    usleep(LIFETIME);

    println("Experiments over. Philosophers _dead_ in the Retiring Room.");

    // Philosophers die :'(
    // for(int i = 0; i < NUM_PHILS; ++i) {
    //     pthread_join(threads[i], NULL);
    // }
}