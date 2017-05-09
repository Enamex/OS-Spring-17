#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
// #include <memory.h>

#define NUM_PHILS (5)
#define MAX_WAIT_US ((int)(3e6))
#define MIN_WAIT_US ((int)(1e6))
#define LIFETIME ((int)(1000 * 1e6))
#define cast(t, e) ((t)(e))
#define leftOf(id) ((id + NUM_PHILS - 1) % NUM_PHILS)
#define rightOf(id) ((id + 1) % NUM_PHILS)
#define CONCAT_(x, y) x y
#define CONCAT(x, y) CONCAT_(x, y)
#define println(fmt, ...) \
    do { \
        printf(CONCAT(fmt, "\n"), __VA_ARGS__); \
    } while(0)

        // fflush(stdout); \

typedef enum {
    psThinking = 0,
    psEating,
    psHungry,
} PhilState;

typedef struct {
    pthread_mutex_t     mutex;
    pthread_cond_t      cond;
    PhilState           state;
} Philosopher;


Philosopher phil[NUM_PHILS];
pthread_t threads[NUM_PHILS];
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

    free((int*)arg);

    println("#%d started.", id);

    Philosopher *self = &phil[id];
    Philosopher *left;
    Philosopher *right;
    left = &(phil[leftOf(id)]);
    right = &(phil[rightOf(id)]);
    
    self->state = psThinking;
    
    int eat_time = 0, think_time = 0;

    //
    while(lifetime > 0) {
        switch(self->state) {
            // finished thinking
            case psThinking: {
                println("[%4d] #%d has started thinking... (%.2fs)", time_seconds(), id, think_time / (1e6));

                think_time = randTime();
                lifetime -= think_time;

                usleep(think_time);

                // pthread_mutex_lock(&self->mutex);
                self->state = psHungry;
                // pthread_mutex_unlock(&self->mutex);
            } break;
            
            // still hungry
            // starts eating
            case psHungry: {
                println("[%4d] #%d wishes to eat.", time_seconds(), id);

                // Something very wrong here...
                // take right chopstick
                pthread_mutex_lock(&(right->mutex));
                while(right->state == psEating)
                    pthread_cond_wait(&right->cond, &(right->mutex));
                
                // take left chopstick
                pthread_mutex_lock(&(left->mutex));
                while(left->state == psEating)
                    pthread_cond_wait(&left->cond, &(left->mutex));
                
                // started eating
                self->state = psEating;

                pthread_mutex_unlock(&left->mutex);
                pthread_mutex_unlock(&right->mutex);
            } break;

            // finishing eating
            case psEating: {
                println("[%4d] #%d has started eating. (%.2fs)", time_seconds(), id, eat_time / (1e6));

                eat_time = randTime();
                lifetime -= eat_time;

                usleep(eat_time);

                // pthread_mutex_lock(&right->mutex);
                pthread_mutex_lock(&left->mutex);
                pthread_mutex_lock(&self->mutex);

                // done eating
                self->state = psThinking;
                
                // Finished eating
                pthread_cond_signal(&left->cond);
                pthread_cond_signal(&self->cond);

                pthread_mutex_unlock(&self->mutex);
                pthread_mutex_unlock(&left->mutex);
                // pthread_mutex_unlock(&right->mutex);
            } break;
        }
    }

    println("[%4d] #%d has retired to the conversation room.", time_seconds(), id);

    return NULL;
}

void p2_loop() {
    // INIT
    for(int i = 0; i < NUM_PHILS; ++i) {
        pthread_mutex_init(&phil[i].mutex, NULL);
        pthread_cond_init(&phil[i].cond, NULL);
    }
    // END INIT

    // Philosophers live
    start = time(0);
    for(int i = 0; i < NUM_PHILS; ++i) {
        println("#%d taking seat...", i);
        int* id = malloc(sizeof (int));
        *id = i;
        pthread_create(&threads[i], NULL, &life, (void*)id);
    }

    // Philosophers die :'(
    for(int i = 0; i < NUM_PHILS; ++i) {
        pthread_join(threads[i], NULL);
    }
}