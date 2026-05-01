#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define BODY_MAX 100000
#define MAX_FIELDS 4
#define MAX_CONNECTION 10

typedef struct {
    int   version;              // always 1 for this protocol 
    char  code[4];              // One of "NAM", "SET", "MSG", "WHO", "ERR" + '\0' 
    int   body_len;             // the number declared in the length field 
    char  body[BODY_MAX + 1];   // body buffer; +1 for the '\0' 
    char *fields[MAX_FIELDS];   // pointers INTO body[], not separate strings 
    int   field_count;          // how many fields were found 
} Message;

//Reading; Three Phases: find out the number of bytes before '|', read exactly that many bytes for the body, fill that into the msg
//stop before each '|' and read each field
static int read_each_field(int fd, char *buf, int max){
    int i = 0;
    char c;
    while(1){
        int n = read(fd, &c, 1);
        //disconnect
        if(n == 0){
            return -1;
        }
        //I/0 error
        if(n < 0){
            return -2;
        }
        //found | ; replace | with \0 + record the # bytes
        if(c == '|'){
            buf[i] = '\0';
            return i;
        }
        //overflow
        if(i >= max - 1){
            return -2;
        }
        buf[i++] = c;
    }
}
//reads the number of bytes obtained from read_each_field and puts it in msg
static int read_body(int fd, char *buf, int n){
    int total = 0;
    while(total < n){
        int got = read(fd, buf + total, n - total);
        if(got == 0){
            return -1;
        }
        //error
        if(got < 0){
            return -2;
        }
        total += got;
    }
    return 0;
}

int read_message(int fd, Message *msg){
    char version_str[8];
    char length_str[8];
    //field 1: version
    int r = read_each_field(fd, version_str, sizeof(version_str));
    //field 2: code

    //
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Must have 1 argument\n");
        exit(EXIT_FAILURE);
    }
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* list;
    char* host = "localhost"; //testing purposes
    char* port = "8080";
    int r = getaddrinfo(host, port, &hints, &list);

    if (r!=0) {
        //handles error specifically for getaddrinfo
        gai_strerror(r);
        exit(EXIT_FAILURE);
    }
    
    struct addrinfo* info;
    int my_socket;

    for (info = list; info != NULL; info = info->ai_next) {
        my_socket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (my_socket < 0) {
            continue;
        }
        int bind_success = bind(my_socket, info->ai_addr, info->ai_addrlen);
        if (bind_success < 0) {
            close(my_socket);
            continue;
        }
        int listen_val = listen(my_socket, MAX_CONNECTION);

        break;
    }
    freeaddrinfo(list);
    close(my_socket);
    if (info == NULL) {
        fprintf(stderr, "Unable to bind\n");
        exit(EXIT_FAILURE);
    }

}