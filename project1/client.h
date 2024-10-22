#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h> // in_port_t and in_addr


void prompt_user(char *user_input);
int string_parser(char **parsed_s, char *raw_s);
void setup_server_addr(struct sockaddr_in *server_addr, char *host_name, char *port);
void login(struct sockaddr_in *server_addr, char *username, int client_socket);

#endif