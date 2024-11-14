CC=gcc

CFLAGS=-Wall -W -g



all: client server

client: client.c raw.c
	$(CC) client.c raw.c $(CFLAGS) -o client

server: server.c raw.c server_data_structures.c
	$(CC) server.c raw.c server_data_structures.c $(CFLAGS) -o server

clean:
	rm -f client server *.o