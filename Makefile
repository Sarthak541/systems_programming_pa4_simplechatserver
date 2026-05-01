CC = gcc
CFLAGS = -std=c99 -fsanitize=address,undefined -Wall
TARGET = chatd

.PHONY = all clean

all: $(TARGET)

chatd: chatd.c
	$(CC) $(CFLAGS) chatd.c -o $(TARGET)

clean: 
	rm -rf chatd 
