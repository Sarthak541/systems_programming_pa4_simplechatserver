# PA4 Systems Programming Simple Chat Server Project
Names: Yaswanth Gosukonda, Sarthak Talukdar
NetIds: yg547, st1332


## Introduction
In this project we created a simple chat server in the C programming language.  The goal of this project was to understand the complexities of Networking and handling multiple requests.  In order to succeed in this project, my group focused on thoroughly understanding the concepts of networking and thorough testing of all cases within the code.


## Testing Methodology
Our testing can be broken up into two categories: (1)compilation testing,(2)component testing and(3)holistic testing. (1) Compilation testing is as it sounds: testing to see whether the code compiles either through gcc or make. (2) Component testing meant that we would test each function(or component) individually to see if that particular part was working correctly. By doing so, we could isolate and diagnose where problems were occurring. (3)Holistic testing means testing to see if the entirety of the code is working correctly. In other words, if the parts are working correctly, do they build on top of each other and together produce the results we want? 

## Test File
We wrote a C test file (test.c) which tests the key logic of each function using pipes to simulate socket communication. Each test calls a function directly, passes it a crafted Message struct, and reads back the response from the pipe to verify the output. 

## Testing Suite
We are using the five scenarios:

test_NAM_basic:
Sends a NAM message with the name "Bob" to approving_username. Checks that the user is marked as registered and that the reply sent back is a MSG with the welcome text "Welcome to the chat!".

test_SET_basic:
Sets up two users, Bob and Alice, both registered. Sends a SET message with status "Busy" on behalf of Bob. Checks that Bob's status field is updated to "Busy" and that Alice receives a broadcast MSG on her socket.

test_MSG_private:
Sets up two users, Bob and Alice, both registered. Sends a MSG from Bob to Alice with body "Hello". Checks that Alice's socket receives a MSG reply with the correct code.

test_MSG_broadcast:
Sets up two users, Bob and Alice, both registered. Sends a MSG from Bob to #all with body "Hi everyone". Checks that both Bob's socket and Alice's socket each receive a MSG reply.

test_WHO_basic: Sets up two users, Bob and Alice. Alice has status "Online". Sends a WHO message querying "Alice" on behalf of Bob. Checks that Bob's socket receives a MSG reply containing Alice's name and status.

All tests were done by via the provided project client.  All tests passed.
