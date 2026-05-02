#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#define BODY_MAX 100000
#define MAX_FIELDS 4
#define MAX_CONNECTION 10
#define MAX_NAME     33
#define MAX_STATUS   65
#define MAX_CLIENTS  100


typedef struct {
    int  fd;
    char name[MAX_NAME];
    char status[MAX_STATUS];
    int  registered;
    int  is_connected; //1 if connected, 0 if disconnected
} User;

typedef struct {
    User           *users[MAX_CLIENTS];
    int             count;
    pthread_mutex_t lock;
} ServerState;

//Why global_state: Need to know who is currently connected
ServerState global_state = {
    .users = {NULL},
    .count = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER
};

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

//used to send customizable error messages based on the specific scenario (especially when approving)
void send_error(int fd, int code, const char *explanation){
    char code_str[4];
    snprintf(code_str, sizeof(code_str), "%d", code);
    sent_message(fd, "ERR", code_str, explanation, NULL);
}

//checks name not empty, every character legal, does exceed 32 chars
int is_valid_name(const char *s){
    int len = 0;
    if(s == NULL || s[0] == '\0'){
        return 0;
    }
    for(int i = 0; s[i] != '\0'; i++){
        char c = s[i];
        //isalpha checks if alphabet or not
        if(!isalpha((unsigned char)c) && !isdigit((unsigned char)c) && 
        c != '-' && c != '_') {
            return 0;
        }
        len++;
        if(len > 32){
            return 0;
        }
    }
    return 1;
}

void approving_username(User *user, ServerState *state, Message *msg){
    //Must have 1 field only
    if(msg->field_count != 1){
        send_error(user->fd, 0, "Unreadable");
        return;
    }
    char *requested_name = msg->fields[0];
    //check if too long
    if(strlen(requested_name) > 32){
        send_error(user->fd, 4, "Username too long");
        return;
    }
    //illegal or empty characters use is_valid_name
    if(!is_valid_name(requested_name)){
        send_error(user->fd, 3, "Illegal or Empty Character");
        return;
    }
    //check if name is in use
    //accesses address of lock to see whether has already been taken
    pthread_mutex_lock(&state->lock);
    for(int i = 0; i< state->count; i++){
        if(state->users[i]->registered && strcmp(state->users[i]->name, requested_name) == 0){
            //"unlocks" means username is taken
            pthread_mutex_unlock(&state->lock);
            send_error(user->fd, 1, "Name in use");
            return;
        }
    }
    //Register/Approve the username
    strncpy(user->name, requested_name, MAX_NAME - 1);
    user->name[MAX_NAME - 1] = '\0';
    user->registered = 1;
    //reserve the username
    pthread_mutex_unlock(&state->lock);
    sent_message(user->fd, "MSG", "#all", user->name, "Welcome to the chat!");
}

//status much be between 0 - 64 characters
int is_valid_status(const char *s){
    if(s == NULL){
        return 0;
    }
    int len = 0;
    for(int i = 0; s[i] != '\0'; i++){
        unsigned char c = (unsigned char)s[i];
        if(c < 32 || c > 126){
            return 0;
        }
        len++;
        if(len >64){
            return 0;
        }
    }
    return 1;
}

void update_status(User *user, ServerState *state, Message *msg){
    //Must have 1 field only
    if(msg->field_count != 1){
        send_error(user->fd, 0, "Unreadable");
        return;
    }
    char *requested_status = msg->fields[0];
        //check if too long
    if(strlen(requested_status) > 64){
        send_error(user->fd, 4, "Status too long");
        return;
    }
    //illegal or empty characters use is_valid_name
    if(!is_valid_status(requested_status)){
        send_error(user->fd, 3, "Illegal or Empty Character");
        return;
    }
    strncpy(user->status, requested_status, MAX_STATUS - 1);
    user->status[MAX_STATUS - 1] = '\0';
    //non-empty, then broadcast 
    char broadcast[BODY_MAX];
    if(user->status[0] != '\0'){
        //broadcast string logic
        snprintf(broadcast, sizeof(broadcast), "%s is now \"%s\"", user->name, user->status);
        pthread_mutex_lock(&state->lock);
        for(int i = 0; i < state->count; i++){
            if(state->users[i]->registered){
                sent_message(state->users[i]->fd, "MSG", "#all", "#all", broadcast);
            }
        }
    pthread_mutex_unlock(&state->lock);
    }
}

//max len: 80, 32 - 126 chars, at least 1 arg
int is_valid_message(const char *s){
    if(s == NULL || s[0] == '\0'){
        return 0;
    }
    int len = 0;
    for(int i = 0; s[i] != '\0'; i++){
        unsigned char c = (unsigned char)s[i];
        if(c < 32 || c > 126){
            return 0;
        }
        len++;
        if(len > 80){
            return 0;
        }
    }
    return 1;
}

void * handle_process(void* return_socketfd) {

    printf("TCP/IP Connection established!\n");
    
    close(*(int*)return_socketfd);
    free(return_socketfd);

    return NULL;

    //Find the users connected to socket
    /*
    User *user = NULL;
    for(int i = 0; i < global_state.count; i++){
        if(global_state.users[i]->fd == return_socketfd){
            user = global_state.users[i];
            break;
        }
    }
    //if no user found
    if(user == NULL){
        close(return_socketfd);
        return;
    }
    Message message;
    int r = read_message(return_socketfd, &message);
    //client disconnected
    if(r == -1){
        user->is_connected = 0;
        return;
    }
    //I/0 Error
    if(r == -2){
        send_error(return_socketfd, 0, "Unreadable");
        user->is_connected = 0;
        return;
    }
    //unknown code
    if(r == -2){
        send_error(return_socketfd, 0, "Unreadable");
        user->is_connected = 0;
        return;
    }
    if(!user->registered && strcmp(message.code, "NAM") != 0){
        send_error(return_socketfd, 0, "Unreadable");
        user->is_connected = 0;
        return;
    }
    if(strcmp(message.code, "NAM") == 0){
        approving_username(user, &global_state, &message);
    }
    else if(strcmp(message.code, "SET") == 0){
        update_status(user, &global_state, &message);
    }
    //MSG
    //WHO
    */
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Must have 1 arugment\n");
        exit(EXIT_FAILURE);
    }
    //parsing host:port from argv[1]
    char buffer[256];
    strncpy(buffer, argv[1], sizeof(buffer));
    buffer[255] = '\0';
    //since colon separates host from port, figure out where the colon occurs
    char *colon = strchr(buffer, ':');
    if(colon == NULL){
        fprintf(stderr, "Invalid format. Expected host:port\n");
        exit(EXIT_FAILURE);
    }
    *colon = '\0';
    char *host = buffer;
    char *port = colon + 1;
    if(*port == '\0'){
        fprintf(stderr, "Need port\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* list;
    int r = getaddrinfo(host, port, &hints, &list);

    if (r!=0) {
        //handles error specifically for getaddrinfo
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
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
    if (info == NULL) {
        fprintf(stderr, "Unable to bind\n");
        exit(EXIT_FAILURE);
    }

    

    //use sockaddr_storage because remote address can be ipv4 OR ipv6
    while (1) {
        struct sockaddr_storage remote_address;
        socklen_t remote_address_len = sizeof(remote_address);
        int return_socket = accept(my_socket, (struct sockaddr *)&remote_address, &remote_address_len);
        if (return_socket < 0) {
            fprintf(stderr, "Error accepting socket\n");
            exit(EXIT_FAILURE);
        }

        pthread_t thread_id;
        int *p_client_soc = (int*)malloc(sizeof(int));
        *p_client_soc = return_socket;
        //must close the return_socket inside the thread function
        pthread_create(&thread_id, NULL, handle_process, p_client_soc);
        pthread_detach(thread_id);
        
    }
    close(my_socket);
}