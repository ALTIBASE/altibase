#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "acpConfig.h"
#include "acpTypes.h"
#include "acpMemBarrier.h"
#include "acpAtomic-ARM64_LINUX.h"

#define MAX_THREAD 1000

pthread_t threads[MAX_THREAD];
unsigned int shared_counter;
int n_loop;

void *thread_main(void *arg) {
	int i; 

	for (i=0; i<n_loop; i++) {
		acpAtomicAdd32EXPORT(&shared_counter, 1);
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	int n_thread;
	long status;
	long i;
	int rc;

	if (argc != 3) {
		fprintf(stderr, "Usage: ./test_driver <n_thraed> <n_loop>\n");
		exit(1);
	}

	n_thread = atoi(argv[1]);
	if (n_thread >= MAX_THREAD) {
		fprintf(stderr, "Too many threads: %d\n", n_thread);
		exit(1);
	}

	n_loop = atoi(argv[2]);
	if (n_loop < 0) {
		fprintf(stderr, "Loop must be positive: %d\n", n_loop);
		exit(1);
	}

	fprintf(stderr, "n_loop: %d\n", n_loop);
	shared_counter = 0;
	for (i=0; i<n_thread; i++) {
 		pthread_create(&threads[i], NULL, &thread_main, (void *)i);
		fprintf(stderr, "%ld-th thread created.\n", i);
	}
	fprintf(stderr, "%d threads created.\n", n_thread);

	for (i=0; i<n_thread; i++) {
		rc = pthread_join(threads[i], (void **) &status);
		if (rc == 0) {
			fprintf(stderr, "Completed join with thread %ld status = %ld\n", i, status);
		} else  {
			fprintf(stderr, "ERROR; return code from pthread_join() is %d, thread %ld\n", rc, i);
		}
	}
	fprintf(stdout, "#threads: %d n_loop: %d shared counters: %d expected: %d\n", n_thread, n_loop, shared_counter,
									n_thread * n_loop);

	return 0;
}
