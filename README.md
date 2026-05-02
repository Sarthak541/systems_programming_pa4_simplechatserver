# PA4 Systems Programming Simple Chat Server Project
Names: Yaswanth Gosukonda, Sarthak Talukdar
NetIds: yg547, st1332


## Introduction
In this project we created a simple chat server in the C programming language.  The goal of this project was to understand the complexities of Networking and handling multiple requests.  In order to succeed in this project, my group focused on thoroughly understanding the concepts of networking and thorough testing of all cases within the code.


## Testing Methodology
Our testing can be broken up into two categories: (1)compilation testing,(2)component testing and(3)holistic testing. (1) Compilation testing is as it sounds: testing to see whether the code compiles either through gcc or make. (2) Component testing meant that we would test each function(or component) individually to see if that particular part was working correctly. By doing so, we could isolate and diagnose where problems were occurring. (3)Holistic testing means testing to see if the entirety of the code is working correctly. In other words, if the parts are working correctly, do they build on top of each other and together produce the results we want? 


We would also want to first test all the cases given in the write-up and see if the same output is given. If not, then we would know for sure that they work.


## Testing Suite
# Test Cases Given in the Write-Up
>> 1|NAM|4|Bob|
<< 1|MSG|30|#all|Bob|Welcome to the chat!|
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
