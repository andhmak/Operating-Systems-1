/* File: parent.c */

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
#define MAX_LINE 100
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

struct shared_use_st {
	char shared_text[MAX_LINE];
};

int main(int argc, char* argv[]) {
	/* Initialising arguments */
    if (argc != 4) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    char* X = argv[1];
    FILE* text_file = fopen(X,"rb");
    if (text_file == NULL) {
        fprintf(stderr, "fopen failed\n");
        exit(EXIT_FAILURE);
    }
    int K = atoi(argv[2]), N = atoi(argv[3]);
    if (K < 0 || N < 0) {
        fprintf(stderr, "No negative arguments allowed\n");
        exit(EXIT_FAILURE);
    }

    /* Counting lines */
    char ch;
    int num_lines = 1;
    while ((ch = fgetc(text_file)) != EOF) {
        if(ch=='\n')
            num_lines++;
    }
    rewind(text_file);

    /* Initialising shared memory */
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
        // Trying to deallocate resources before exiting with failure
        shmctl(shmid, IPC_RMID, 0);
		exit(EXIT_FAILURE);
	}
//	printf("Shared memory segment with id %d attached at %p\n", shmid, shared_memory);
    shared_stuff = (struct shared_use_st *)shared_memory;

    /* Initialising semaphores */
    sem_t *semaphore_request = sem_open("request", O_CREAT | O_EXCL, SEM_PERMS, 1);
    sem_t *semaphore_response = sem_open("response", O_CREAT | O_EXCL, SEM_PERMS, 0);
    sem_t *semaphore_read_response = sem_open("read_response", O_CREAT | O_EXCL, SEM_PERMS, 0);

    if (semaphore_request == SEM_FAILED) {
        perror("sem_open (semaphore_request) error");
        // Trying to deallocate resources before exiting with failure
        shmctl(shmid, IPC_RMID, 0);
        exit(EXIT_FAILURE);
    }
    if (semaphore_response == SEM_FAILED) {
        perror("sem_open (semaphore_response) error");
        // Trying to deallocate resources before exiting with failure
        sem_unlink("request");
        shmctl(shmid, IPC_RMID, 0);
        exit(EXIT_FAILURE);
    }
    if (semaphore_read_response == SEM_FAILED) {
        perror("sem_open (semaphore_read_response) error");
        // Trying to deallocate resources before exiting with failure
        sem_unlink("request");
        sem_unlink("response");
        shmctl(shmid, IPC_RMID, 0);
        exit(EXIT_FAILURE);
    }

    /* Creating children */
    char num_lines_str[ARG_BUFF], N_str[ARG_BUFF];
    sprintf(num_lines_str, "%d", num_lines);
    sprintf(N_str, "%d", N);
    char *newargv[] = {"./child", num_lines_str, N_str, NULL};
    int pid;
    for (int i = 0 ; i < K ; i++) {
        if ((pid = fork()) == 0) {
            execv("./child", newargv);
            perror("exec failed");
            // Communicate the error to the parent
            // Wait for any other transaction then start a request
            if (sem_wait(semaphore_request) < 0);
            // Get error code into the shared memory
            sprintf(shared_stuff->shared_text, "-1");
            // Allow parent to respond
            if (sem_post(semaphore_response) < 0);
            // Wait for response
            if (sem_wait(semaphore_read_response) < 0);
            // Allow another child to start a transaction
            if (sem_post(semaphore_request) < 0);
            // Exit with failure
            exit(EXIT_FAILURE);
        }
        else if (pid < 0) {
            perror("fork failed");
            // Trying to deallocate resources before exiting with failure
            sem_unlink("request");
            sem_unlink("response");
            sem_unlink("read_response");
            shmctl(shmid, IPC_RMID, 0);
            exit(EXIT_FAILURE);
        }
    }

    /* Giving requested lines */
    for (int i = 0 ; i < K*N ; i++) {
        // Wait for response to be requested
        if (sem_wait(semaphore_response) < 0) {
            perror("sem_wait (semaphore_response) failed on parent");
            // Trying to deallocate resources before exiting with failure
            sem_unlink("request");
            sem_unlink("response");
            sem_unlink("read_response");
            shmctl(shmid, IPC_RMID, 0);
		    exit(EXIT_FAILURE);
        }
        // Read the line requested from the shared memory
        num_lines = atoi(shared_stuff->shared_text);
        // If requesting line
        if (num_lines >= 0) {
            // Get that line into the shared memory
            for (int j = 0 ; j <= num_lines ; j++) {
                fgets(shared_stuff->shared_text, MAX_LINE, text_file);
            }
        }
        // If error signal
        else {
            // Stop waiting for the rest of the child requests
            i += N;
            // Respond that the error message was seen
            sprintf(shared_stuff->shared_text, "OK");
        }
        // Allow reading of response
        if (sem_post(semaphore_read_response) < 0) {
            perror("sem_post (semaphore_read_response) error on child");
            // Trying to deallocate resources before exiting with failure
            sem_unlink("request");
            sem_unlink("response");
            sem_unlink("read_response");
            shmctl(shmid, IPC_RMID, 0);
		    exit(EXIT_FAILURE);
        }
        // Return to the start of the file
        rewind(text_file);
    }

    // Closing file
    if (fclose(text_file) == EOF) {
        fprintf(stderr, "fclose failed\n");
        // Trying to deallocate resources before exiting with failure
        sem_unlink("request");
        sem_unlink("response");
        sem_unlink("read_response");
        shmctl(shmid, IPC_RMID, 0);
        exit(EXIT_FAILURE);
    }

    /* Closing semaphores */
    if (sem_close(semaphore_request) < 0) {
        perror("sem_close (semaphore_request) failed");
        // Trying to deallocate resources before exiting with failure
        sem_unlink("request");
        sem_unlink("response");
        sem_unlink("read_response");
        shmctl(shmid, IPC_RMID, 0);
        exit(EXIT_FAILURE);
    }
    if (sem_close(semaphore_response) < 0) {
        perror("sem_close (semaphore_response) failed");
        // Trying to deallocate resources before exiting with failure
        sem_unlink("request");
        sem_unlink("response");
        sem_unlink("read_response");
        shmctl(shmid, IPC_RMID, 0);
        exit(EXIT_FAILURE);
    }
    if (sem_close(semaphore_read_response) < 0) {
        perror("sem_close (semaphore_read_response) failed");
        // Trying to deallocate resources before exiting with failure
        sem_unlink("request");
        sem_unlink("response");
        sem_unlink("read_response");
        shmctl(shmid, IPC_RMID, 0);
        exit(EXIT_FAILURE);
    }

    /* Detaching shared memory */
	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
        // Trying to deallocate resources before exiting with failure
        sem_unlink("request");
        sem_unlink("response");
        sem_unlink("read_response");
        shmctl(shmid, IPC_RMID, 0);
		exit(EXIT_FAILURE);
	}

    /* Waiting for children and collecting their return codes */
    int status, child_id;
    for (int i = 0 ; i < K ; i++) {
        child_id = wait(&status);
        printf("Process %d returning with code %d.\n", child_id, status);
    }

    /* Unlinking semaphores */
    if (sem_unlink("request") < 0) {
        perror("sem_unlink (semaphore_request) failed");
    }
    if (sem_unlink("response") < 0) {
        perror("sem_unlink (semaphore_response) failed");
    }
    if (sem_unlink("read_response") < 0) {
        perror("sem_unlink (semaphore_read_response) failed");
    }

    /* "Deleting" shared memory */
	if (shmctl(shmid, IPC_RMID, 0) == -1) {
		fprintf(stderr, "shmctl(IPC_RMID) failed\n");
		exit(EXIT_FAILURE);
	}

    /* Exiting successfully */
    exit(EXIT_SUCCESS);
}