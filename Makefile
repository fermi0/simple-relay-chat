CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
TARGETS = server client

all: $(TARGETS)

server: server.c common.h
	$(CC) $(CFLAGS) -o server server.c

client: client.c common.h
	$(CC) $(CFLAGS) -o client client.c

run-server: server
	./server

run-client: client
	./client

clean:
	rm -f $(TARGETS) chat.log

.PHONY: all run-server run-client clean
