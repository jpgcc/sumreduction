#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#define ARRAY_SIZE 1000000000

typedef struct {
	int nextid;
	pthread_mutex_t nextid_mutex;
	int* array;
	int* sum;
	int num_threads;
	int nums_per_thread;

	int counter;
	pthread_mutex_t counter_mutex;
	sem_t* barrier_sem;
} Shared;

int sem_post_n(sem_t *sem, int n) {
	int i, ret;
	for (i = 0; i<n; i++) {
		ret = sem_post(sem);
		if (ret) exit(-1);
	}
	return ret;
}

void synch(Shared *state, int num_threads) {
	pthread_mutex_lock(&(state->counter_mutex));
	
	if(state->counter == (num_threads)-1) {
		// last thread
		state->counter = 0;
		int i;
		for (i = 0; i < (num_threads - 1); i++) {
			sem_post(state->barrier_sem);
		}
		pthread_mutex_unlock(&(state->counter_mutex));
	} else {
		// not last thread
		++(state->counter);
		pthread_mutex_unlock(&(state->counter_mutex));
		sem_wait(state->barrier_sem);
	}
}

void * kernel( void* state_param ) {

	Shared *state = (Shared*) state_param;

	pthread_mutex_lock(&(state->nextid_mutex));
	int pn = state->nextid++;
	pthread_mutex_unlock(&(state->nextid_mutex));

	struct timeval  tv1, tv2;
	if (pn == 0) {
		//start measuring time
		gettimeofday(&tv1, NULL);
	}

	state->sum[pn] = 0;

	int i=0;
	for (i = state->nums_per_thread * pn; i < state->nums_per_thread * (pn+1); i++) {
		state->sum[pn] += state->array[i];
	}

	//reduction
	int half = state->num_threads;
	int synch_i = 1;
	
	while (half > 1) {
		synch(state, half);
		if (half%2 != 0 && pn == 0) {
			state->sum[0] = state->sum[0] + state->sum[half-1];
		}
		half = half/2;
		if (pn < half) {
			state->sum[pn] = state->sum[pn] + state->sum[pn+half];
		} else {
			break;
		}
	}

	if (pn == 0) {

	//stop measuring time
		gettimeofday(&tv2, NULL);

		printf (";%f",
			(double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
			(double) (tv2.tv_sec - tv1.tv_sec));

		fprintf(stderr,"Sum = %d\n", state->sum[0]);
	}

	return 0;
}

int genval(int seed) {
	switch(seed % 3) {
		case 0: return (seed%2 ? seed%5 : 5-(seed%5))+seed/100;
		case 1: return (seed%2 ? 7-(seed%7) : seed%7)+seed/1000;
		case 2: return (seed%2 ? seed%11 : -(11-(seed%11)))+seed/10000;
	}
	return 0; //nowarn
}

int is_base_two(int num) {
	int cmp = 1;
	while (cmp <= num) {
		if (cmp == num) return 1;
		cmp <<= 1;
	}
	return 0;
}

int main(int argc, char** argv) {
	/*if (argc != 2) {
		fprintf(stderr, "Please specify number of threads\n");
		return -1;
	}
	int num_threads = atoi(argv[1]);
	if (ARRAY_SIZE % num_threads) {
		fprintf(stderr, "Please specify number of threads submultiple of %d\n", ARRAY_SIZE);
		return -2;
	}*/
	int* array = malloc(ARRAY_SIZE * sizeof(int));
	int i;
	for (i=0; i < ARRAY_SIZE; i++) {
		array[i] = genval(i);
	}

	int num_threads;
	for (num_threads = 1; num_threads<=128; num_threads++) {
		if ((num_threads%10) != 0 && !is_base_two(num_threads))  {
			continue;
		}
		fprintf(stderr, "%d threads\n", num_threads);
		printf("%d;", num_threads);
		int run;
		for (run=0; run < 10; run++) {
			fprintf(stderr, "run %d\n", run);
			Shared shared_state;
			shared_state.array = array;
			shared_state.nextid = 0;
			pthread_mutex_init(&(shared_state.nextid_mutex), NULL);
			shared_state.nums_per_thread = (ARRAY_SIZE + (num_threads-1))/num_threads;
			shared_state.num_threads = num_threads;

			shared_state.counter = 0;
			pthread_mutex_init(&(shared_state.counter_mutex), NULL);
			shared_state.barrier_sem = sem_open("/barrier_sem", O_CREAT|O_EXCL, S_IRWXU , 0);

			shared_state.sum = malloc(num_threads * sizeof(int));

			pthread_t* children = malloc(num_threads * sizeof(pthread_t));
			for (i=0; i < num_threads; i++) {
				pthread_create (&children[i], NULL, kernel, (void *) &shared_state) ;
			}

			for (i=num_threads-1; i >= 0; i--) {
				pthread_join(children[i], NULL)  ;
			}

			free(shared_state.sum);

			sem_close(shared_state.barrier_sem);
			sem_unlink("/barrier_sem");
		}
		printf("\n");
	}

	free(array);
	return 0;

}