#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

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
    printf("%s\n", shared_stuff->shared_text);

	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
    exit(EXIT_SUCCESS);
}