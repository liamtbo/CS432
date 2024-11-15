#ifndef SERVER_H
#define SERVER_H
#include "server_data_structures.h"
#include <netinet/in.h> // in_port_t and in_addr


typedef struct ServerAddr {
    // int port; // 16 bit unsigned int
    // char ip[INET_ADDRSTRLEN]; // 16 bytes
    struct sockaddr_in server_address;    
    struct ServerAddr *next;
} ServerAddr;

typedef struct ServerAddrList {
    ServerAddr *head;
    int count;
} ServerAddrList;


void create_adjacent_servers(ServerAddrList *server_addr_list, char **argv, int argc);
// void free_adjacent_servers(ServerAddrList *server_addr_list);
void setup_server_addr(struct sockaddr_in *server_addr, char *host_name, char *port);

void process_requests(struct sockaddr_in *packet_src, UserList *user_list, ChannelList *channel_list, int s, char *buffer, ServerAddrList *server_addr_list);
void join(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list);
void say(struct request *req, int s, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list);
void leave(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list);
void logout(UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list);



#endif