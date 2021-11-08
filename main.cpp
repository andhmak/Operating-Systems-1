#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    string X = argv[1];
    fstream text_file;
    text_file.open(X, fstream::in);
    int K = atoi(argv[2]), N = atoi(argv[3]);
    string line;
    printf("%d%d", K, N);
    if (text_file.is_open()) {
        getline(text_file, line);
        cout << line;
    }
    return 0;
}