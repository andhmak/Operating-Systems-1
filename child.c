#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define LINE_BUF 10

struct shared_use_st {
	char shared_text[100];
};

int main(int argc, char* argv[]) {
    int X = atoi(argv[1]), N = atoi(argv[2]);
    printf("This is the child: %d, %d\n", X, N);
    sleep(1);
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
	printf("Shared memory segment with id %d attached at %p\n", shmid, shared_memory);
	shared_stuff = (struct shared_use_st *)shared_memory;


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

	for (int i = 0 ; i < N ; i++) {
		int line = rand() % X;
        if (sem_wait(semaphore_request) < 0) {
            perror("sem_wait (semaphore_request) failed on child");
		    exit(EXIT_FAILURE);
        }
    	sprintf(shared_stuff->shared_text, "%d", line);
		printf("Asked line %d\n", line);
        if (sem_post(semaphore_response) < 0) {
            perror("sem_post (semaphore_response) error on child");
        }
        if (sem_wait(semaphore_read_response) < 0) {
            perror("sem_wait (semaphore_read_response) failed on child");
		    exit(EXIT_FAILURE);
        }
		printf("%s", shared_stuff->shared_text);
        if (sem_post(semaphore_request) < 0) {
            perror("sem_post (semaphore_request) error on child");
        }
	}

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

	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
    exit(EXIT_SUCCESS);
}