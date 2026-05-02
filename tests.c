#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#define BODY_MAX 100000
#define MAX_FIELDS 4
#define MAX_NAME 33
#define MAX_STATUS 65
#define MAX_CLIENTS 100

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

typedef struct {
    int   version;              // always 1 for this protocol 
    char  code[4];              // One of "NAM", "SET", "MSG", "WHO", "ERR" + '\0' 
    int   body_len;             // the number declared in the length field 
    char  body[BODY_MAX + 1];   // body buffer; +1 for the '\0' 
    char *fields[MAX_FIELDS];   // pointers INTO body[], not separate strings 
    int   field_count;          // how many fields were found 
} Message;

int passed = 0;
int failed = 0;
//checks track of the number of test cases passed
void check(const char *test_name, int condition) {
    if (condition) {
        fprintf(stdout, "PASSED: %s\n", test_name);
        passed++;
    } else {
        fprintf(stderr, "FAILED: %s\n", test_name);
        failed++;
    }
}

//What it tests: Does a message written by sent_message come out of read_message correctly
void test_round_trip() {
    int p[2]; pipe(p);
    sent_message(p[1], "MSG", "#all", "Bob", "Hello, world!");
    Message msg;
    int r = read_message(p[0], &msg);
    check("round_trip: returns 0",              r == 0);
    check("round_trip: code is MSG",            strcmp(msg.code, "MSG") == 0);
    check("round_trip: fields[0] is #all",      strcmp(msg.fields[0], "#all") == 0);
    check("round_trip: fields[1] is Bob",       strcmp(msg.fields[1], "Bob") == 0);
    check("round_trip: fields[2] is message",   strcmp(msg.fields[2], "Hello, world!") == 0);
    close(p[0]); close(p[1]);
}
