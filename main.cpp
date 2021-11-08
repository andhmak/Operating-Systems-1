#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    FILE* X = fopen(argv[1], "rb");
    int K = atoi(argv[2]), N = atoi(argv[3]);
    printf("%d%d", K, N);
    return 0;
}