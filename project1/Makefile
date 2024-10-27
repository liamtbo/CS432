CC=gcc

CFLAGS=-Wall -W -g



all: client server

run_server: clean server
	./server localhost 5000

run_client: client
	./client localhost 5000 mclovin

client: client.c raw.c
	$(CC) client.c raw.c $(CFLAGS) -o client

server: server.c raw.c
	$(CC) server.c raw.c $(CFLAGS) -o server

clean:
	rm -f client server *.o