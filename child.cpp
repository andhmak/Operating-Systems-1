#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main(int argc, char* argv[]) {
    int X = atoi(argv[1]), N = atoi(argv[2]);
    cout << "This is the child: " << X << ", " << N << endl;
    return 0;
}