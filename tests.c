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
    check("NAM_basic: user is registered",       user.registered == 1);
    check("NAM_basic: name is Bob",              strcmp(user.name, "Bob") == 0);
    Message reply;
    read_message(p[0], &reply);
    check("NAM_basic: reply code is MSG",        strcmp(reply.code, "MSG") == 0);
    check("NAM_basic: sender is #all",           strcmp(reply.fields[0], "#all") == 0);
    check("NAM_basic: recipient is Bob",         strcmp(reply.fields[1], "Bob") == 0);
    check("NAM_basic: welcome text correct",
    strcmp(reply.fields[2], "Welcome to the chat!") == 0);
    close(p[0]); close(p[1]);
}

//Input:    1|SET|17|Smiling politely|
//Expected: 1|MSG|40|#all|#all|Bob is now "Smiling politely"|
//What we are testing: setting a non-empty status broadcasts the status change to all registered users
void test_SET_broadcasts() {
    int p_bob[2]; pipe(p_bob);
    int p_alice[2]; pipe(p_alice);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    bob.fd = p_bob[1];
    bob.registered = 1;
    User alice;
    memset(&alice, 0, sizeof(alice));
    strcpy(alice.name, "Alice");
    alice.fd = p_alice[1];
    alice.registered = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &alice;
    state.count = 2;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "SET");
    strcpy(msg.body, "Smiling politely|");
    msg.fields[0] = msg.body;
    msg.field_count = 1;
    update_status(&bob, &state, &msg);
    check("SET_broadcasts: bob status updated",
          strcmp(bob.status, "Smiling politely") == 0);
    // alice should receive the broadcast
    Message alice_reply;
    read_message(p_alice[0], &alice_reply);
    check("SET_broadcasts: alice receives broadcast",
          strcmp(alice_reply.code, "MSG") == 0);
    check("SET_broadcasts: sender is #all",
          strcmp(alice_reply.fields[0], "#all") == 0);
    check("SET_broadcasts: broadcast text correct",
          strcmp(alice_reply.fields[2], "Bob is now \"Smiling politely\"") == 0);
    close(p_bob[0]); close(p_bob[1]);
    close(p_alice[0]); close(p_alice[1]);
}

c
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

/* paste structs and functions from chatd.c here */

int passed = 0;
int failed = 0;

void check(const char *test_name, int condition) {
    if (condition) {
        fprintf(stdout, "PASSED: %s\n", test_name);
        passed++;
    } else {
        fprintf(stderr, "FAILED: %s\n", test_name);
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
    check("NAM_basic: user is registered",      user.registered == 1);
    check("NAM_basic: name is Bob",             strcmp(user.name, "Bob") == 0);
    Message reply;
    read_message(p[0], &reply);
    check("NAM_basic: reply code is MSG",       strcmp(reply.code, "MSG") == 0);
    check("NAM_basic: sender is #all",          strcmp(reply.fields[0], "#all") == 0);
    check("NAM_basic: recipient is Bob",        strcmp(reply.fields[1], "Bob") == 0);
    check("NAM_basic: welcome text correct",
          strcmp(reply.fields[2], "Welcome to the chat!") == 0);
    close(p[0]); close(p[1]);
}

//Input:    1|SET|17|Smiling politely|
//Expected: 1|MSG|40|#all|#all|Bob is now "Smiling politely"|
//What we are testing: setting a non-empty status broadcasts the status change to all registered users
void test_SET_broadcasts() {
    int p_bob[2]; pipe(p_bob);
    int p_alice[2]; pipe(p_alice);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    bob.fd = p_bob[1];
    bob.registered = 1;
    User alice;
    memset(&alice, 0, sizeof(alice));
    strcpy(alice.name, "Alice");
    alice.fd = p_alice[1];
    alice.registered = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &alice;
    state.count = 2;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "SET");
    strcpy(msg.body, "Smiling politely|");
    msg.fields[0] = msg.body;
    msg.field_count = 1;
    update_status(&bob, &state, &msg);
    check("SET_broadcasts: bob status updated",
          strcmp(bob.status, "Smiling politely") == 0);
    Message alice_reply;
    read_message(p_alice[0], &alice_reply);
    check("SET_broadcasts: alice receives broadcast",
          strcmp(alice_reply.code, "MSG") == 0);
    check("SET_broadcasts: sender is #all",
          strcmp(alice_reply.fields[0], "#all") == 0);
    check("SET_broadcasts: broadcast text correct",
          strcmp(alice_reply.fields[2], "Bob is now \"Smiling politely\"") == 0);
    close(p_bob[0]); close(p_bob[1]);
    close(p_alice[0]); close(p_alice[1]);
}

//Input:    1|MSG|28||Alice|Private message to Alice|
//Expected: 1|MSG|35|Bob|Alice|Private message to Alice| (sent only to Alice)
//What we are testing: private message is forwarded only to the target user with sender field replaced by real name
void test_MSG_private() {
    int p_bob[2]; pipe(p_bob);
    int p_alice[2]; pipe(p_alice);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    bob.fd = p_bob[1];
    bob.registered = 1;
    bob.is_connected = 1;
    User alice;
    memset(&alice, 0, sizeof(alice));
    strcpy(alice.name, "Alice");
    alice.fd = p_alice[1];
    alice.registered = 1;
    alice.is_connected = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &alice;
    state.count = 2;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "MSG");
    msg.fields[0] = "";
    msg.fields[1] = "Alice";
    msg.fields[2] = "Private message to Alice";
    msg.field_count = 3;
    message_handling(&bob, &state, &msg);
    Message alice_reply;
    read_message(p_alice[0], &alice_reply);
    check("MSG_private: alice receives message",
          strcmp(alice_reply.code, "MSG") == 0);
    check("MSG_private: sender replaced with Bob",
          strcmp(alice_reply.fields[0], "Bob") == 0);
    check("MSG_private: recipient is Alice",
          strcmp(alice_reply.fields[1], "Alice") == 0);
    check("MSG_private: message body correct",
          strcmp(alice_reply.fields[2], "Private message to Alice") == 0);
    close(p_bob[0]); close(p_bob[1]);
    close(p_alice[0]); close(p_alice[1]);
}

//Input:    1|MSG|20||#all|Hello, world!|
//Expected: 1|MSG|23|Bob|#all|Hello, world!| (sent to all users)
//What we are testing: broadcast message is forwarded to all connected users with sender field replaced by real name
void test_MSG_broadcast() {
    int p_bob[2]; pipe(p_bob);
    int p_alice[2]; pipe(p_alice);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    bob.fd = p_bob[1];
    bob.registered = 1;
    bob.is_connected = 1;
    User alice;
    memset(&alice, 0, sizeof(alice));
    strcpy(alice.name, "Alice");
    alice.fd = p_alice[1];
    alice.registered = 1;
    alice.is_connected = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &alice;
    state.count = 2;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "MSG");
    msg.fields[0] = "";
    msg.fields[1] = "#all";
    msg.fields[2] = "Hello, world!";
    msg.field_count = 3;
    message_handling(&bob, &state, &msg);
    Message bob_reply;
    read_message(p_bob[0], &bob_reply);
    check("MSG_broadcast: bob receives message",
          strcmp(bob_reply.fields[2], "Hello, world!") == 0);
    check("MSG_broadcast: sender is Bob",
          strcmp(bob_reply.fields[0], "Bob") == 0);
    Message alice_reply;
    read_message(p_alice[0], &alice_reply);
    check("MSG_broadcast: alice receives message",
          strcmp(alice_reply.fields[2], "Hello, world!") == 0);
    check("MSG_broadcast: recipient is #all",
          strcmp(alice_reply.fields[1], "#all") == 0);
    close(p_bob[0]); close(p_bob[1]);
    close(p_alice[0]); close(p_alice[1]);
}

//Input:    1|WHO|6|Alice| (Alice has status "I was here first")
//Expected: 1|MSG|33|#all|Bob|Alice: I was here first|
//What we are testing: WHO query for a specific user with a status returns "name: status" format
void test_WHO_specific_user_with_status() {
    int p_bob[2]; pipe(p_bob);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    bob.fd = p_bob[1];
    bob.registered = 1;
    User alice;
    memset(&alice, 0, sizeof(alice));
    strcpy(alice.name, "Alice");
    strcpy(alice.status, "I was here first");
    alice.registered = 1;
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
    read_message(p_bob[0], &reply);
    check("WHO_with_status: code is MSG",       strcmp(reply.code, "MSG") == 0);
    check("WHO_with_status: sender is #all",    strcmp(reply.fields[0], "#all") == 0);
    check("WHO_with_status: recipient is Bob",  strcmp(reply.fields[1], "Bob") == 0);
    check("WHO_with_status: response correct",
          strcmp(reply.fields[2], "Alice: I was here first") == 0);
    close(p_bob[0]); close(p_bob[1]);
}

//Input:    1|WHO|6|Carol| (Carol has no status)
//Expected: 1|MSG|20|#all|Bob|No status|
//What we are testing: WHO query for a user with no status returns "No status"
void test_WHO_specific_user_no_status() {
    int p_bob[2]; pipe(p_bob);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    bob.fd = p_bob[1];
    bob.registered = 1;
    User carol;
    memset(&carol, 0, sizeof(carol));
    strcpy(carol.name, "Carol");
    carol.registered = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.users[1] = &carol;
    state.count = 2;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "WHO");
    msg.fields[0] = "Carol";
    msg.field_count = 1;
    handling_WHO(&bob, &state, &msg);
    Message reply;
    read_message(p_bob[0], &reply);
    check("WHO_no_status: response is No status",
          strcmp(reply.fields[2], "No status") == 0);
    close(p_bob[0]); close(p_bob[1]);
}

//Input:    1|WHO|5|#all| (Alice: "I was here first", Bob: "Smiling politely", Carol: no status)
//Expected: 1|MSG|61|#all|Bob|Alice: I was here first\nBob: Smiling politely\nCarol|
//What we are testing: WHO #all returns all users with statuses, newline separated, no trailing newline
void test_WHO_all() {
    int p_bob[2]; pipe(p_bob);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    strcpy(bob.status, "Smiling politely");
    bob.fd = p_bob[1];
    bob.registered = 1;
    User alice;
    memset(&alice, 0, sizeof(alice));
    strcpy(alice.name, "Alice");
    strcpy(alice.status, "I was here first");
    alice.registered = 1;
    User carol;
    memset(&carol, 0, sizeof(carol));
    strcpy(carol.name, "Carol");
    carol.registered = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &alice;
    state.users[1] = &bob;
    state.users[2] = &carol;
    state.count = 3;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "WHO");
    msg.fields[0] = "#all";
    msg.field_count = 1;
    handling_WHO(&bob, &state, &msg);
    Message reply;
    read_message(p_bob[0], &reply);
    check("WHO_all: code is MSG",               strcmp(reply.code, "MSG") == 0);
    check("WHO_all: Alice in response",         strstr(reply.fields[2], "Alice: I was here first") != NULL);
    check("WHO_all: Bob in response",           strstr(reply.fields[2], "Bob: Smiling politely") != NULL);
    check("WHO_all: Carol in response",         strstr(reply.fields[2], "Carol") != NULL);
    int len = strlen(reply.fields[2]);
    check("WHO_all: no trailing newline",       reply.fields[2][len - 1] != '\n');
    close(p_bob[0]); close(p_bob[1]);
}

//Input:    1|NAM|4|Bob| (Bob already registered), second client sends 1|NAM|4|Bob|
//Expected: 1|ERR|13|1|Name in use|
//What we are testing: duplicate name registration is rejected with ERR 1 and second client stays unregistered
void test_NAM_duplicate() {
    int p[2]; pipe(p);
    User existing;
    memset(&existing, 0, sizeof(existing));
    strcpy(existing.name, "Bob");
    existing.registered = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &existing;
    state.count = 1;
    User new_user;
    memset(&new_user, 0, sizeof(new_user));
    new_user.fd = p[1];
    new_user.registered = 0;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "NAM");
    strcpy(msg.body, "Bob|");
    msg.fields[0] = msg.body;
    msg.field_count = 1;
    approving_username(&new_user, &state, &msg);
    check("NAM_duplicate: new user not registered",  new_user.registered == 0);
    Message reply;
    read_message(p[0], &reply);
    check("NAM_duplicate: ERR sent",                 strcmp(reply.code, "ERR") == 0);
    check("NAM_duplicate: error code is 1",          strcmp(reply.fields[0], "1") == 0);
    close(p[0]); close(p[1]);
}

//Input:    1|MSG|22||Carol|Hello Carol!| (Carol is not connected)
//Expected: 1|ERR|9|2|Unknown recipient|
//What we are testing: sending a message to a user who does not exist returns ERR 2
void test_MSG_unknown_recipient() {
    int p_bob[2]; pipe(p_bob);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    bob.fd = p_bob[1];
    bob.registered = 1;
    bob.is_connected = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.count = 1;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "MSG");
    msg.fields[0] = "";
    msg.fields[1] = "Carol";
    msg.fields[2] = "Hello Carol!";
    msg.field_count = 3;
    message_handling(&bob, &state, &msg);
    Message reply;
    read_message(p_bob[0], &reply);
    check("MSG_unknown_recipient: ERR sent",        strcmp(reply.code, "ERR") == 0);
    check("MSG_unknown_recipient: error code is 2", strcmp(reply.fields[0], "2") == 0);
    close(p_bob[0]); close(p_bob[1]);
}

//Input:    1|WHO|6|Carol| (Carol is not connected)
//Expected: 1|ERR|9|2|Unknown recipient|
//What we are testing: WHO query for a user who does not exist returns ERR 2
void test_WHO_unknown_user() {
    int p_bob[2]; pipe(p_bob);
    User bob;
    memset(&bob, 0, sizeof(bob));
    strcpy(bob.name, "Bob");
    bob.fd = p_bob[1];
    bob.registered = 1;
    ServerState state;
    memset(&state, 0, sizeof(state));
    pthread_mutex_init(&state.lock, NULL);
    state.users[0] = &bob;
    state.count = 1;
    Message msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.code, "WHO");
    msg.fields[0] = "Carol";
    msg.field_count = 1;
    handling_WHO(&bob, &state, &msg);
    Message reply;
    read_message(p_bob[0], &reply);
    check("WHO_unknown_user: ERR sent",         strcmp(reply.code, "ERR") == 0);
    check("WHO_unknown_user: error code is 2",  strcmp(reply.fields[0], "2") == 0);
    close(p_bob[0]); close(p_bob[1]);
}

int main() {
    fprintf(stdout, "=== Running tests ===\n\n");
    test_NAM_basic();
    test_SET_broadcasts();
    test_MSG_private();
    test_MSG_broadcast();
    test_WHO_specific_user_with_status();
    test_WHO_specific_user_no_status();
    test_WHO_all();
    test_NAM_duplicate();
    test_MSG_unknown_recipient();
    test_WHO_unknown_user();
    fprintf(stdout, "\n=== Results: %d passed, %d failed ===\n",
            passed, failed);
    return failed > 0 ? 1 : 0;
}

