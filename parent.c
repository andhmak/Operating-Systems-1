#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define ARG_BUFF 12
#define MAX_LINE 1000
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

struct shared_use_st {
	char shared_text[MAX_LINE];
};

int main(int argc, char* argv[]) {
    char* X = argv[1];
    FILE* text_file = fopen(X,"rb");
    if(text_file == NULL) {
        printf("fopen failed");
        exit(EXIT_FAILURE);
    }
    int K = atoi(argv[2]), N = atoi(argv[3]);
    char ch;
    int num_lines = 1;
    while((ch = fgetc(text_file)) != EOF) {
        if(ch=='\n')
            num_lines++;
    }
    void rewind(FILE *f);   
    rewind(text_file);
    printf("This is the parent: %d, %d, %d\n", K, N, num_lines);

    struct shared_use_st *shared_stuff;
	void *shared_memory = (void *)0;
	int shmid;
	shmid = shmget((key_t)1234, MAX_LINE*sizeof(char), 0666 | IPC_CREAT);
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

    sem_t *semaphore_request = sem_open("request", O_CREAT, SEM_PERMS, 1);
    sem_t *semaphore_response = sem_open("response", O_CREAT, SEM_PERMS, 0);
    sem_t *semaphore_read_response = sem_open("read_response", O_CREAT, SEM_PERMS, 0);

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

    char num_lines_str[ARG_BUFF], N_str[ARG_BUFF];
    sprintf(num_lines_str, "%d", num_lines);
    sprintf(N_str, "%d", N);
    char *newargv[] = {"./child", num_lines_str, N_str, NULL};
    int pid;
    for (int i = 0 ; i < K ; i++) {
        if ((pid = fork()) == 0) {
            execv("./child", newargv);
            perror("exec failed");
            exit(EXIT_FAILURE);
        }
        else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        else {
            printf("Fork %d returned %d\n", i, pid);
        }
    }

    for (int i = 0 ; i < K*N ; i++) {
        if (sem_wait(semaphore_response) < 0) {
            perror("sem_wait (semaphore_response) failed on parent");
		    exit(EXIT_FAILURE);
        }
        num_lines = atoi(shared_stuff->shared_text);
        for (int j = 0 ; j <= num_lines ; j++) {
            fgets(shared_stuff->shared_text, MAX_LINE, text_file);
        }
        void rewind(FILE *f);   
        rewind(text_file);
        if (sem_post(semaphore_read_response) < 0) {
            perror("sem_post (semaphore_read_response) error on child");
        }
    }

	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
    int status;
    for (int i = 0 ; i < K ; i++) {
        printf("Process %d returning.\n", wait(&status));
    }
	if (shmctl(shmid, IPC_RMID, 0) == -1) {
		fprintf(stderr, "shmctl(IPC_RMID) failed\n");
		exit(EXIT_FAILURE);
	}
    if (sem_close(semaphore_request) < 0) {
        perror("sem_close (semaphore_request) failed");
        sem_unlink("request");
        exit(EXIT_FAILURE);
    }
    if (sem_close(semaphore_response) < 0) {
        perror("sem_close (semaphore_response) failed");
        sem_unlink("response");
        exit(EXIT_FAILURE);
    }
    if (sem_close(semaphore_read_response) < 0) {
        perror("sem_close (semaphore_read_response) failed");
        sem_unlink("read_response");
        exit(EXIT_FAILURE);
    }
    if (sem_unlink("request") < 0) {
        perror("sem_unlink (semaphore_request) failed");
    }
    if (sem_unlink("response") < 0) {
        perror("sem_unlink (semaphore_response) failed");
    }
    if (sem_unlink("read_response") < 0) {
        perror("sem_unlink (semaphore_read_response) failed");
    }

    exit(EXIT_SUCCESS);
}