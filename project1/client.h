#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h> // in_port_t and in_addr


void prompt_user(char *user_input, int client_socket, struct sockaddr_in *server_addr);

void exit_program(struct sockaddr_in *server_addr, int client_socket);
void join_channel(struct sockaddr_in *server_addr, int client_socket, char *channel, int *channel_count, char *active_channel);
void logout(struct sockaddr_in *server_addr, int client_socket);
void login(struct sockaddr_in *server_addr, char *username, int client_socket);
void leave_channel(struct sockaddr_in *server_addr, int client_socket, char *channel);
void say_command(struct sockaddr_in *server_addr, int client_socket,  
                  char *active_channel, char *message);

int string_parser(char **parsed_s, char *raw_s);
void setup_server_addr(struct sockaddr_in *server_addr, char *host_name, char *port);

#endif