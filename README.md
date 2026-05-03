# PA4 Systems Programming Simple Chat Server Project
Names: Yaswanth Gosukonda, Sarthak Talukdar
NetIds: yg547, st1332


## Introduction
In this project we created a simple chat server in the C programming language.  The goal of this project was to understand the complexities of Networking and handling multiple requests.  In order to succeed in this project, my group focused on thoroughly understanding the concepts of networking and thorough testing of all cases within the code.


## Testing Methodology
Our testing can be broken up into two categories: (1)compilation testing,(2)component testing and(3)holistic testing. (1) Compilation testing is as it sounds: testing to see whether the code compiles either through gcc or make. (2) Component testing meant that we would test each function(or component) individually to see if that particular part was working correctly. By doing so, we could isolate and diagnose where problems were occurring. (3)Holistic testing means testing to see if the entirety of the code is working correctly. In other words, if the parts are working correctly, do they build on top of each other and together produce the results we want? 


We would also want to first test all the cases given in the write-up and see if the same output is given. If the output matches, we know that part of the server is working correctly. There are seven of these. Then, we wrote 3 of our own test cases to cover scenarios that the original 7 did not address: duplicate name registration (ERR 1), messaging a user who is not connected (ERR 2), and querying WHO for a user who is not connected (ERR 2).

## Testing Suite

Test Cases Given in the Write-Up

Input: 1|NAM|4|Bob|
Output: 1|MSG|30|#all|Bob|Welcome to the chat!|
What it tests: successful registration — server accepts a valid name, adds user to #all, and sends welcome message

Input: 1|SET|17|Smiling politely|
Output: 1|MSG|40|#all|#all|Bob is now "Smiling politely"|
What it tests: Setting a non-empty status broadcasts the status change to all users in #all

Input: 1|MSG|28||Alice|Private message to Alice|
Output: 1|MSG|35|Bob|Alice|Private message to Alice|
What it tests: Private message forwarding — server replaces empty sender field with real username, sends only to Alice not to everyone

Input: 1|MSG|20||#all|Hello, world!|
Output: 1|MSG|23|Bob|#all|Hello, world!|
What it tests: Broadcast message — server replaces empty sender field with real username and forwards to all connected users

Input: 1|WHO|6|Alice|
Output: 1|MSG|33|#all|Bob|Alice: I was here first|
What it tests: WHO query for a specific user who has a status set — response is "name: status"

Input: 1|WHO|6|Carol|
Output: 1|MSG|20|#all|Bob|No status|
What it tests: WHO query for a specific user who has no status set — response is "No status"

Input: 1|WHO|5|#all|
Output: 1|MSG|61|#all|Bob|Alice: I was here first\nBob: Smiling politely\nCarol|
What it tests: WHO query for entire room — response lists all users with their statuses, users with no status show name only, separated by newlines

Additional Test Cases

Input: 1|NAM|4|Bob| (Bob already registered), second client sends 1|NAM|4|Bob|
Output: 1|ERR|13|1|Name in use|
What it tests: duplicate name registration — server rejects a name already in use with ERR 1 and second client stays unregistered

Input: 1|MSG|22||Carol|Hello Carol!| (Carol is not connected)
Output: 1|ERR|9|2|Unknown recipient|
What it tests: private message to nonexistent user — server returns ERR 2 when recipient is not currently connected

Input: 1|WHO|6|Carol| (Carol is not connected)
Output: 1|ERR|9|2|Unknown recipient|
What it tests: WHO query for nonexistent user — server returns ERR 2 when the queried name is not currently connected

## Test File
We wrote a C test file (test.c) which tests the key logic of each function using pipes to simulate socket communication. Each test calls a function directly, passes it a crafted Message struct, and reads back the response from the pipe to verify the output. The tests cover the following functions:

- approving_username: tests successful registration and duplicate name rejection
- update_status: tests that a non-empty status is stored and broadcast to all registered users
- message_handling: tests broadcast to #all, private message forwarding, and unknown recipient error
- handling_WHO: tests single user with status, single user with no status, unknown user error, and full room listing

To compile and run the test file:
```bash
gcc -Wall -Wextra -pthread -o test test.c
./test
```

Each test prints PASSED or FAILED along with the specific check that failed if any. The final line prints a summary of how many tests passed and how many failed.
