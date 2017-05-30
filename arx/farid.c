#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

pthread_mutex_t mutex;
pthread_mutex_t pmutex;
pthread_cond_t cond[5];
enum {THINKING, HUNGRY, EATING} state[5];
int forks[5] = {0, 0, 0, 0, 0};

void *run(void *);
void pickup_forks(int);
void putdown_forks(int);
void test(int);

int main(int argc, char *argv[]) {
	pthread_t tid[5];
	pthread_attr_t attr[5];
	int idx[5] = {0, 1, 2, 3, 4};

	/* inititialize threads */
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&pmutex, NULL);
	for (int i = 0; i < 5; ++i) {
		pthread_attr_init(&attr[i]);
		pthread_cond_init(&cond[i], NULL);
		pthread_create(&tid[i], &attr[i], run, &idx[i]);
		state[i] = THINKING;
	}

	for (int i = 0; i < 5; ++i)
		pthread_join(tid[i], NULL);
	return 0;
}

void *run(void *p)
{
	int i = *((int*) p);
	while(1) {
		int dur = rand() % 3 + 1;

		pickup_forks(i);
		printf("Philo %d is eating\n", i);
		sleep(dur);

		putdown_forks(i);
		printf("Philo %d is thinking...\n", i);
		sleep(dur);
	}
}

void pickup_forks(int i)
{
	pthread_mutex_lock(&mutex);
	state[i] = HUNGRY;

	pthread_mutex_lock(&pmutex);
	printf("Philo %d is trying to pickup fork %d\n", i, (i + 4) % 5);
	printf("Philo %d is trying to pickup fork %d\n", i, i);
	pthread_mutex_unlock(&pmutex);

	test(i);
	while (state[i] != EATING)
		pthread_cond_wait(&cond[i], &mutex);

	pthread_mutex_lock(&pmutex);
	printf("Philo %d picked up fork %d\n", i, (i + 4) % 5);
	printf("Philo %d picked up fork %d\n", i, i);
	pthread_mutex_unlock(&pmutex);

	pthread_mutex_unlock(&mutex);
}

void putdown_forks(int i)
{
	pthread_mutex_lock(&mutex);
	state[i] = THINKING;

	pthread_mutex_lock(&pmutex);
	printf("Philo %d is putting down fork %d\n", i, (i + 4) % 5);
	printf("Philo %d is putting down fork %d\n", i, i);
	pthread_mutex_unlock(&pmutex);

	test((i + 4) % 5);
	test((i + 1) % 5);
	pthread_mutex_unlock(&mutex);
}

void test(int i)
{
	if ((state[(i + 4) % 5] != EATING) && (state[i] == HUNGRY)\
		&& (state[(i + 1) % 5] != EATING)) {
		state[i] = EATING;
		pthread_cond_signal(&cond[i]);
	}
}
