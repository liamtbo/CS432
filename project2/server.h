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

void send_join_adjacent_servers(ServerAddrList *server_addr_list, Channel *specified_channel, 
    char *local_ip_str, struct sockaddr_in *local_server_addr, int s, struct request_join *req_join, User *user);
void unsub_server(Channel *specified_channel, struct sockaddr_in *server_removing, struct sockaddr_in *local_server_addr);
int check_id(long int id);
void print_server_ports(ChannelList *channel_list, struct sockaddr_in *local_server_addr);
unsigned int get_urandom();
ServerAndTime *find_server(ServerAndTimeList *server_time_list, struct sockaddr_in *target_server);
void sub_server_to_channel(ServerAddr *server, Channel *specified_channel);
void create_adjacent_servers(ServerAddrList *server_addr_list, char **argv, int argc);
void setup_server_addr(struct sockaddr_in *server_addr, char *host_name, char *port);

void process_requests(struct sockaddr_in *packet_src, UserList *user_list, 
                    ChannelList *channel_list, int s, char *buffer, ServerAddrList *server_addr_list,
                    struct sockaddr_in *local_server_addr);
void join(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, 
        ChannelList *channel_list, ServerAddrList *server_addr_list, int s, struct sockaddr_in *local_server_addr);
void say(struct request *req, int s, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list, struct sockaddr_in *local_server_addr);
void leave(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list, struct sockaddr_in *local_server_addr, int s);
void logout(UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list);



#endif