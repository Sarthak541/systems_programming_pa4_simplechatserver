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
    int r;
    //field 1: version
    r = read_each_field(fd, version_str, sizeof(version_str));
    if(r<0){
        return r;
    }
    if(strcmp(version_str, "1") != 0){
        return 2;
    }
    msg->version = 1;
    //field 2: code
    r = read_each_field(fd, msg->code, sizeof(msg->code));
    if(r<0){
        return r;
    }
    if(strcmp(msg->code, "NAM") != 0 &&
       strcmp(msg->code, "SET") != 0 &&
       strcmp(msg->code, "MSG") != 0 &&
       strcmp(msg->code, "WHO") != 0 && 
       strcmp(msg->code, "ERR") != 0){
        return 2;
    }
    //field 3: body length
    r = read_each_field(fd, length_str, sizeof(length_str));
        if(r<0){
        return r;
    }
    for(int i = 0; length_str[i] != '\0'; i++){
        if(!isdigit((unsigned char)length_str[i])){
            return -2;
        }
    }
    msg->body_len = atoi(length_str);
    if(msg->body_len < 0 || msg->body_len >= BODY_MAX){
        return -2;
    }
    r = read_body(fd, msg->body, msg->body_len);
    if(r < 0){
        return r;
    }
    msg->body[msg->body_len] = '\0';
    if(msg->body_len == 0 || msg->body[msg->body_len - 1] != '|'){
        return -2;
    }
    msg->field_count = 0;
    char *p = msg->body;
    char *end = msg->body + msg->body_len;
    while(p <end){
        msg->fields[msg->field_count++] = p;
        while(p < end && *p != '|'){
            p++;
        }
        *p = '\0';
        p++;
    }
    return 0;
}

int sent_message(int fd, const char *code, const char *f1, const char *f2, const char *f3){
    int body_len = 0;
    //do +1 for '\0'
    if(f1){
        body_len += strlen(f1) + 1;
    }
    if(f2){
        body_len += strlen(f2) + 1;
    }
    if(f3){
        body_len += strlen(f3) + 1;
    }
    //assemble into one buffer
    char buf[BODY_MAX + 32];
    int header_len = snprintf(buf, sizeof(buf), "1|%s|%d|", code, body_len);
    //append all field followed by '|'
    int pos = header_len;
    if(f1){
        strcpy(buf + pos, f1);
        pos += strlen(f1);
        buf[pos++] = '|';
    }
    if(f2){
        strcpy(buf + pos, f2);
        pos += strlen(f2);
        buf[pos++] = '|';    
    }
    if(f3){
        strcpy(buf + pos, f3);
        pos += strlen(f3);
        buf[pos++] = '|';
    }
    int total = 0;
    //write pos bytes to the socket
    while(total < pos){
        int sent = write(fd, buf + total, pos - total);
        if(sent < 0){
            //overflow or I/O error
            return -1;
        }
        total += sent;
    }
    return 0;
}

void print_message(Message *msg) {
    fprintf(stdout, "version:     %d\n", msg->version);
    fprintf(stdout, "code:        %s\n", msg->code);
    fprintf(stdout, "body_len:    %d\n", msg->body_len);
    fprintf(stdout, "field_count: %d\n", msg->field_count);
    for (int i = 0; i < msg->field_count; i++) {
        fprintf(stdout, "fields[%d]:   %s\n", i, msg->fields[i]);
    }
    fprintf(stdout, "\n");
}

void test_read_write() {
    Message msg;
    int r;

    // test 1: MSG with three fields
    int p1[2]; pipe(p1);
    sent_message(p1[1], "MSG", "#all", "Bob", "Hello, world!");
    r = read_message(p1[0], &msg);
    if (r != 0) { fprintf(stderr, "TEST 1 FAILED: %d\n", r); }
    else { fprintf(stdout, "TEST 1 PASSED:\n"); print_message(&msg); }
    close(p1[0]); close(p1[1]);

    // test 2: NAM with one field
    int p2[2]; pipe(p2);
    sent_message(p2[1], "NAM", "Bob", NULL, NULL);
    r = read_message(p2[0], &msg);
    if (r != 0) { fprintf(stderr, "TEST 2 FAILED: %d\n", r); }
    else { fprintf(stdout, "TEST 2 PASSED:\n"); print_message(&msg); }
    close(p2[0]); close(p2[1]);

    // test 3: ERR with two fields
    int p3[2]; pipe(p3);
    sent_message(p3[1], "ERR", "1", "Name in use", NULL);
    r = read_message(p3[0], &msg);
    if (r != 0) { fprintf(stderr, "TEST 3 FAILED: %d\n", r); }
    else { fprintf(stdout, "TEST 3 PASSED:\n"); print_message(&msg); }
    close(p3[0]); close(p3[1]);

    // test 4: MSG with empty sender
    int p4[2]; pipe(p4);
    sent_message(p4[1], "MSG", "", "Alice", "Private message");
    r = read_message(p4[0], &msg);
    if (r != 0) { fprintf(stderr, "TEST 4 FAILED: %d\n", r); }
    else { fprintf(stdout, "TEST 4 PASSED:\n"); print_message(&msg); }
    close(p4[0]); close(p4[1]);
}

int main(int argc, char* argv[]) {
    test_read_write();
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

    //use sockaddr_storage because remote address can be ipv4 OR ipv6
    struct sockaddr_storage remote_address;
    socklen_t remote_address_len = sizeof(remote_address);
    int return_socket = accept(my_socket, &remote_address, &remote_address_len);
    if (return_socket < 0) {
        fprintf(stderr, "Error accepting socket\n");
        exit(EXIT_FAILURE);
    }
    else {
        fprintf(stdout, "Working TCP/IP!\n");
    }
    close(return_socket);
    close(my_socket);
    
    if (info == NULL) {
        fprintf(stderr, "Unable to bind\n");
        exit(EXIT_FAILURE);
    }

}