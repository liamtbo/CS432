#ifndef SERVER_H
#define SERVER_H
#include "server_data_structures.h"
#include <netinet/in.h> // in_port_t and in_addr


typedef struct ServerAddr {
    int port; // 16 bit unsigned int
    char ip[INET_ADDRSTRLEN]; // 16 bytes
    struct ServerAddr *next;
} ServerAddr;

typedef struct ServerAddrList {
    ServerAddr *head;
    int count;
} ServerAddrList;

void populate_adjacent_servers(ServerAddrList *server_addr_list, char **argv, int argc);
void free_adjacent_servers(ServerAddrList *server_addr_list);

void join(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list);
void say(struct request *req, int s, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list);
void leave(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list);
void logout(UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list);



#endif