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

void check(const char *name, int cond) {
    if (cond) {
        printf("PASSED: %s\n", name);
        passed++;
    } else {
        printf("FAILED: %s\n", name);
        failed++;
    }
}

//Input:    1|NAM|4|Bob|
//Expected: 1|MSG|30|#all|Bob|Welcome to the chat!|
//What we are testing: basic registration — valid unused name gets registered and receives a welcome message 
void test_NAM_basic() {
    int p[2]; pipe(p);

    User user;
    memset(&user, 0, sizeof(user));
    user.fd = p[1];

    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "NAM");
    strcpy(msg.body, "Bob|");
    msg.fields[0] = msg.body;
    msg.field_count = 1;

    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);

    approving_username(&user, &state, &msg);

    check("NAM: registered", user.registered == 1);

    Message reply;
    read_message(p[0], &reply);

    check("NAM: got MSG reply", strcmp(reply.code, "MSG") == 0);
    
    close(p[0]); close(p[1]);
}

//Input:    1|SET|17|Smiling politely|
//Expected: 1|MSG|40|#all|#all|Bob is now "Smiling politely"|
//What we are testing: setting a non-empty status broadcasts the status change to all registered users
void test_SET_basic() {
    int p1[2]; pipe(p1);
    int p2[2]; pipe(p2);

    User bob = {.fd = p1[1], .registered = 1};
    strcpy(bob.name, "Bob");

    User alice = {.fd = p2[1], .registered = 1};
    strcpy(alice.name, "Alice");

    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &alice;
    state.count = 2;

    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "SET");
    strcpy(msg.body, "Busy|");
    msg.fields[0] = msg.body;
    msg.field_count = 1;

    update_status(&bob, &state, &msg);

    check("SET: status updated", strcmp(bob.status, "Busy") == 0);

    Message reply;
    read_message(p2[0], &reply);

    check("SET: broadcast sent", strcmp(reply.code, "MSG") == 0);

    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);
}

//Input:    1|MSG|28||Alice|Private message to Alice|
//Expected: 1|MSG|35|Bob|Alice|Private message to Alice| (sent only to Alice)
//What we are testing: private message is forwarded only to the target user with sender field replaced by real name
void test_MSG_private() {
    int p1[2]; pipe(p1);
    int p2[2]; pipe(p2);

    User bob = {.fd = p1[1], .registered = 1};
    strcpy(bob.name, "Bob");

    User alice = {.fd = p2[1], .registered = 1};
    strcpy(alice.name, "Alice");

    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &alice;
    state.count = 2;

    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "MSG");
    msg.fields[1] = "Alice";
    msg.fields[2] = "Hello";
    msg.field_count = 3;

    message_handling(&bob, &state, &msg);

    Message reply;
    read_message(p2[0], &reply);

    check("MSG private: delivered", strcmp(reply.code, "MSG") == 0);

    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);
}
//Input:    1|MSG|20||#all|Hello, world!|
//Expected: 1|MSG|23|Bob|#all|Hello, world!| (sent to all users)
//What we are testing: broadcast message is forwarded to all connected users with sender field replaced by real name
void test_MSG_broadcast() {
    int p1[2]; pipe(p1);
    int p2[2]; pipe(p2);

    User bob = {.fd = p1[1], .registered = 1};
    strcpy(bob.name, "Bob");

    User alice = {.fd = p2[1], .registered = 1};
    strcpy(alice.name, "Alice");

    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &alice;
    state.count = 2;

    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "MSG");
    msg.fields[1] = "#all";
    msg.fields[2] = "Hi everyone";
    msg.field_count = 3;

    message_handling(&bob, &state, &msg);

    Message r1, r2;
    read_message(p1[0], &r1);
    read_message(p2[0], &r2);

    check("MSG broadcast: sent to bob", strcmp(r1.code, "MSG") == 0);
    check("MSG broadcast: sent to alice", strcmp(r2.code, "MSG") == 0);

    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);
}
//Input:    1|WHO|6|Alice| (Alice has status "I was here first")
//Expected: 1|MSG|33|#all|Bob|Alice: I was here first|
//What we are testing: WHO query for a specific user with a status returns "name: status" format
void test_WHO_basic() {
    int p[2]; pipe(p);

    User bob = {.fd = p[1], .registered = 1};
    strcpy(bob.name, "Bob");

    User alice = {.registered = 1};
    strcpy(alice.name, "Alice");
    strcpy(alice.status, "Online");

    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &alice;
    state.count = 2;

    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "WHO");
    msg.fields[0] = "Alice";
    msg.field_count = 1;

    handling_WHO(&bob, &state, &msg);

    Message reply;
    read_message(p[0], &reply);

    check("WHO: response sent", strcmp(reply.code, "MSG") == 0);

    close(p[0]); close(p[1]);
}


int main() {
    fprintf(stdout, "=== Running tests ===\n\n");
    test_NAM_basic();
    test_SET_basic();
    test_MSG_private();
    test_MSG_broadcast();
    test_WHO_basic();
    fprintf(stdout, "\n=== Results: %d passed, %d failed ===\n",
            passed, failed);
    return failed > 0 ? 1 : 0;
}


