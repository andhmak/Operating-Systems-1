#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

#define ARG_BUFF 12

using namespace std;

int main(int argc, char* argv[]) {
    string X = argv[1];
    fstream text_file;
    text_file.open(X, fstream::in);
    int K = atoi(argv[2]), N = atoi(argv[3]);
    string line;
    int num_lines = 0;
    if (text_file.is_open()) {
        while(!text_file.eof()) {
            getline(text_file, line);
            num_lines++;
        }
    }
    text_file.clear();
    text_file.seekg(0);
    cout << "This is the parent: " << K << ", " << N << ", " << num_lines << endl;

    int status;
    char num_lines_str[100], N_str[100];
    sprintf(num_lines_str, "%d", num_lines);
    sprintf(N_str, "%d", N);
    static char *newargv[] = {"./child", num_lines_str, N_str, NULL};
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
    for (int i = 0 ; i < K ; i++) {
        cout << "Process " << wait(&status) << " returning." << endl;
    }
    return 0;
}