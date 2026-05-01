#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#define PORT 8080 //for testing purposes use localhost
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Must have 1 argument\n");
        exit(EXIT_FAILURE);
    }
    int socket_fd;   
}