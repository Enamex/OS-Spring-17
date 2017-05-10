#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include <sys/types.h>

#include "nofal.h"

// macros

#define NUM_THREADS 5

#define right(philo_num) ((philo_num-1+NUM_THREADS)%NUM_THREADS)

#define left(philo_num) ((philo_num+1)%NUM_THREADS)


#define cond_wait_on(philo_num)\
        do{pthread_cond_wait(&cond_var[philo_num], &mutex[philo_num]);\
            }while(0)

#define forks_init(forks)\
        do{for (int i =0; i < NUM_THREADS; i++)\
            forks[i] = 0;}while(0)

#define mutex_lock(philo_num)\
        do{\
        pthread_mutex_lock(&mutex[philo_num]);\
            }while(0)

//pthread_mutex_lock(&mutex[philo_num+1%NUM_THREADS]);\

#define mutex_unlock(philo_num)\
        do{\
        pthread_mutex_unlock(&mutex[philo_num]);\
            }while(0)

#define cond_signal(philo_num)\
        do{\
        pthread_cond_signal(&cond_var[philo_num]);\
            }while(0)

// Shared Resources
pthread_t philosophers[NUM_THREADS]; /* the thread identifier */
pthread_attr_t philosophers_attr[NUM_THREADS]; /* set of thread attributes */
pthread_mutex_t mutex[NUM_THREADS];
pthread_cond_t cond_var[NUM_THREADS];
pthread_mutex_t printing_mutex;
int idx[NUM_THREADS] = {0,1,2,3,4};
// forks to be taken and released
int forks[NUM_THREADS];
/*random commenting trying to understand the problem
 * Now we have the */

// Function Prototypes

void* runner(void *param); /* threads call this function*/
void pickup_forks(int philo_num);
void return_forks(int philo_num);



int nofal()
{
    srand(time(NULL));
	// struct arguments args;
	/* get the default attributes */
	/* Just a brain storm*/
	/* what I need to do
		I need to initialize 5 threads one for each philo_num
	what I do not know how are threads simulating this behavior 
	of philosophers when they do not know the resources
	allocated for each other thread*/
    forks_init(forks);
    pthread_mutex_init(&printing_mutex,NULL);
	for (int i =0; i < NUM_THREADS; i++)
		pthread_attr_init(&philosophers_attr[i]);
	/* create the thread */
    for (int i =0; i < NUM_THREADS; i++)
        pthread_mutex_init(&mutex[i], NULL);
    for (int i =0; i < NUM_THREADS; i++)
        pthread_cond_init(&cond_var[i], NULL);
    //for (int i =0; i <NUM_THREADS; i++)
    int i = 0;

//    while( i < NUM_THREADS-1)
    for(int i =0; i < NUM_THREADS; i++)
    {
        printf("creating thread for phil_num %d \n\n", i);
        pthread_create(&philosophers[i],&philosophers_attr[i], &runner, &idx[i]);
//        if (i < NUM_THREADS-1)
//            i++;
    }

//    /* wait for the thread to exit */
	for (int i =0; i < NUM_THREADS; i++)
		pthread_join(philosophers[i],NULL);

    return 0;

}
/* The thread will begin control in this function */
void* runner(void *param)
{
    int philo_num = *((int*) param);
    //1printf("philo num input is : %d\n\n\n\n", philo_num);
    while(1)
    {
        int secs_to_slp = (rand()+1)%3;

        // The philosopher will start by trying to eat, all of them will
        // which means it will try to lock the corresponding mutex and the mutex of the next philosopher
        // if any of them fails it will wait on that mutex
        // Waiting should be executed using the cond_var
        // it will wait on both mutexes if both are not available
        //printf("inside runner with philosopher :%d \n", *((int*) param));
        //sprintf("mutex[philo_num].__data.__lock %d", mutex[*((int*) param)].__data.__lock );
        pickup_forks(philo_num);
        
        pthread_mutex_lock(&printing_mutex);
        printf("philo_num %d eating.....    \n", philo_num);
        pthread_mutex_unlock(&printing_mutex);
        
        sleep(secs_to_slp);
        
        return_forks(philo_num);
        
        pthread_mutex_lock(&printing_mutex);
        printf("philo_num %d thinking.....    \n", philo_num);
        pthread_mutex_unlock(&printing_mutex);
        
        sleep(secs_to_slp);

    }

}
// Requesting resources
void pickup_forks(int philo_num)
{
    printf("philo_num :%d is thinking about picking up fork %d\n\n",philo_num,right(philo_num));
    mutex_lock(right(philo_num));
    forks[right(philo_num)] = 1;
    printf("philo_num :%d picked up mutex %d\n\n",philo_num,right(philo_num));
    printf("philo_num :%d is thinking about picking up fork %d\n\n",philo_num,left(philo_num));
    //forks[left(philo_num)] != 0
    while(pthread_mutex_trylock(&mutex[left(philo_num)]) !=0)
    {
        forks[right(philo_num)] = 0;
        printf("philo_num :%d put down mutex %d\n\n",philo_num,right(philo_num));
        cond_wait_on(right(philo_num));
    }
    //mutex_lock(right(philo_num));
    printf("philo_num :%d picked up fork %d\n\n",philo_num,left(philo_num));
    printf("philo_num :%d picked up fork %d\n\n",philo_num,right(philo_num));
    //mutex_lock(left(philo_num));
    forks[right(philo_num)] = 1;
    forks[left(philo_num)] = 1;
}
// Returning resources
void return_forks(int philo_num)
{
    forks[left(philo_num)] = 0;
    printf("philo_num :%d put down fork %d\n\n",philo_num,left(philo_num));
    cond_signal(left(philo_num));
    mutex_unlock(left(philo_num));
    forks[right(philo_num)] = 0;
    printf("philo_num :%d put down fork %d\n\n",philo_num,right(philo_num));
    cond_signal(right(philo_num));
    mutex_unlock(right(philo_num));
}
