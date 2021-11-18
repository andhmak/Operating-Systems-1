/* File: child.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define LINE_BUF 10
#define MAX_LINE 100

struct shared_use_st {
	char shared_text[MAX_LINE];
};

int main(int argc, char* argv[]) {
	/* Initialising arguments */
    int num_lines = atoi(argv[1]), N = atoi(argv[2]);
//    printf("This is the child: %d, %d\n", num_lines, N);

	/* Initialising shared memory */
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;
	int shmid;
	srand((unsigned int)getpid());
	shmid = shmget((key_t)1234, 100*sizeof(char), 0666 | IPC_CREAT);
	if (shmid == -1) {
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
//	printf("Shared memory segment with id %d attached at %p\n", shmid, shared_memory);
	shared_stuff = (struct shared_use_st *)shared_memory;

	/* Initialising semaphores */
    sem_t *semaphore_request = sem_open("request", O_RDWR);
	sem_t *semaphore_response = sem_open("response", O_RDWR);
	sem_t *semaphore_read_response = sem_open("read_response", O_RDWR);
    if (semaphore_request == SEM_FAILED) {
        perror("sem_open (semaphore_request) error");
        exit(EXIT_FAILURE);
    }
    if (semaphore_response == SEM_FAILED) {
        perror("sem_open (semaphore_response) error");
        exit(EXIT_FAILURE);
    }
    if (semaphore_read_response == SEM_FAILED) {
        perror("sem_open (semaphore_read_response) error");
        exit(EXIT_FAILURE);
    }

	/* Requesting for lines */
	clock_t start, end;
	double average_time = 0;
	for (int i = 0 ; i < N ; i++) {
		int line = rand() % num_lines;
        if (sem_wait(semaphore_request) < 0) {
            perror("sem_wait (semaphore_request) failed on child");
		    exit(EXIT_FAILURE);
        }
    	sprintf(shared_stuff->shared_text, "%d", line);
		printf("Process %d asked for line %d\n", getpid(), line + 1); // numbering starting from 1
		start = clock();
        if (sem_post(semaphore_response) < 0) {
            perror("sem_post (semaphore_response) error on child");
        }
        if (sem_wait(semaphore_read_response) < 0) {
            perror("sem_wait (semaphore_read_response) failed on child");
		    exit(EXIT_FAILURE);
        }
		end = clock();
		printf("Process %d got:\n%s\n", getpid(), shared_stuff->shared_text);
        if (sem_post(semaphore_request) < 0) {
            perror("sem_post (semaphore_request) error on child");
        }
		average_time += (double)(end - start) / CLOCKS_PER_SEC;
	}

	average_time /= N;
	printf("Child process %d experienced an average response time of %fs.\n", getpid(), average_time);

	/* CLosing semaphores */
    if (sem_close(semaphore_request) < 0) {
        perror("sem_close (semaphore_request) failed");
        exit(EXIT_FAILURE);
    }
    if (sem_close(semaphore_response) < 0) {
        perror("sem_close (semaphore_response) failed");
        exit(EXIT_FAILURE);
    }
    if (sem_close(semaphore_read_response) < 0) {
        perror("sem_close (semaphore_read_response) failed");
        exit(EXIT_FAILURE);
    }

	/* Detaching shared memory */
	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}

	/* Exiting successfully */
    exit(EXIT_SUCCESS);
}