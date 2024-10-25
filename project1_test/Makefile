CC=gcc

CFLAGS=-Wall -W -g



all: client server

run_server: server
	./server 172.31.59.247 8080

run_client: client
	./client 172.31.59.247 8080 mclovin

client: client.c raw.c
	$(CC) client.c raw.c $(CFLAGS) -o client

server: server.c raw.c
	$(CC) server.c raw.c $(CFLAGS) -o server

clean:
	rm -f client server *.o