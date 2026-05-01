#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Must have 1 argument\n");
        exit(EXIT_FAILURE);
    }
}