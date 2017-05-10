// #include <stdio.h>
// #include <pthread.h>
// #include <time.h>
// #include <stdlib.h>
// #include <unistd.h>
// // #include <stdbool.h>

// #define NUM_PHILOSOPHERS 5
// #define MAX_WAIT ((int)(3.0 * 1000000))
// #define MIN_WAIT ((int)(0.5 * 1000000))

// typedef _Bool bool;

// typedef enum PhilosopherState {
//     psThinking,
//     psEating,
//     psHungry,
// } PS;

// int rand_wait() {
//     return MIN_WAIT + (rand() % MAX_WAIT);
// }

// pthread_mutex_t stick_mutex[NUM_PHILOSOPHERS];
// pthread_cond_t  stick_beacon[NUM_PHILOSOPHERS];

// PS phils[NUM_PHILOSOPHERS];
// pthread_t phil_threads[NUM_PHILOSOPHERS];

// void make_philosopher(void* phil_id, void*init_wait) {
//     int id = *(int*)(phil_id);
//     int wait_time = *(int*)(init_wait);
//     usleep(wait_time);
//     for(;;) {
//         // OnStart { psThinking }
//         if(phils[id] == psThinking) {
//             int left = id;
//             int right = (id + 1) % NUM_PHILOSOPHERS;
//             phils[id] = psHungry;
            
//             // left stick
//             pthread_mutex_lock(&stick_mutex[left]);
//             while(phils[left] != psThinking)
//                 pthread_cond_wait(&stick_beacon[left], &stick_mutex[left]);

//             // right stick
//             pthread_mutex_lock(&stick_mutex[right]);
//             while(phils[right] != psThinking)
//                 pthread_cond_wait(&stick_beacon[right], &stick_mutex[right]);

//             // mark engaged in eating
//             phils[id] = psEating;
            
//             // done marking
//             pthread_mutex_unlock(&stick_mutex[right]);
//             pthread_mutex_unlock(&stick_mutex[left]);
            
//             // work start

//             // work end

//             phils[id] = psThinking;

//             // done
//         }
//     }
// }

// bool is(int pn, PS state) {
//     return phils[pn] == state;
// }


// int main() {
//     srand(time(NULL));

//     return 0;
// }
