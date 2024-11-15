#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> // socket
#include <netinet/in.h> // in_port_t and in_addr
#include <string.h>
#include <arpa/inet.h> // inet_pton
#include "duckchat.h"
#include "server.h"
#include <netdb.h> // gethostbyname
#include <sys/select.h> // select(), fd_set
#include <errno.h> // errno and EINTR


int main(int argc, char *argv[]) {

    if (argc % 2 == 0) {
        printf("argc: %d\n", argc);
        printf("error: wrong number of inputs\n");
        exit(EXIT_FAILURE);
    }
    char *host_name = argv[1];
    char *port = argv[2];

    // this hold the sockadd_in for every adjacent server
    // PE!!! this was never tested
    ServerAddrList server_addr_list;
    memset(&server_addr_list, 0, sizeof(ServerAddrList));
    create_adjacent_servers(&server_addr_list, argv, argc);

    struct sockaddr_in server_addr;
    // after declaring, server mem loc might have garbage values
    // this might mess up padding
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET; // IPv4 internet protocol
    server_addr.sin_port = htons(atoi(port)); // local port 5000
    // Use "127.0.0.1" if host_name is "localhost"
    // Convert hostname to IP address
    if (inet_pton(AF_INET, host_name, &server_addr.sin_addr) <= 0) {
        // If the hostname is not an IP address, try to resolve it
        struct hostent *host = gethostbyname(host_name);
        if (host == NULL) {
            perror("gethostbyname");
            exit(EXIT_FAILURE);
        }
        memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);
    }

    // allocates a socket and returns a descriptor
    // create IPv4, UDP socket, IP for protocols
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { perror("Error calling socket()"); exit(EXIT_FAILURE);}

    // bind socket to specific IP addr and port so it can listen for packet_src connections
    int b = bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (b < 0) { perror("Error calling bind()"); exit(EXIT_FAILURE);}

    char buffer[sizeof(struct request_say)];  // Use the size of the largest struct

    struct sockaddr_in packet_src;
    socklen_t client_len = sizeof(packet_src);

    ChannelList channel_list = {NULL};
    UserList user_list = {NULL};
    // add_channel(&channel_list, "Common");
    // channel_list.count += 1;

    fd_set read_fds;
    struct timeval timeout;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(s, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int activity = select(s + 1, &read_fds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        } else if (activity > 0) {
            if (FD_ISSET(s, &read_fds)) {
                int bytes_returned = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&packet_src, &client_len);
                if (bytes_returned > 0) {
                    buffer[bytes_returned] = '\0';
                    process_requests(&packet_src, &user_list, &channel_list, s, buffer, &server_addr_list);
                }
                if (bytes_returned < 0) {
                    perror("recvfrom failed");
                    continue;
                }
            } else {
                printf("Waiting for packet_src data...\n");
            }
        }
        // printf("testing blocking\n");
        // TODO: need to test how old joins are
    }
    // free_adjacent_servers(&server_addr_list);
}

void process_requests(struct sockaddr_in *packet_src, UserList *user_list, 
                        ChannelList *channel_list, int s, char *buffer, 
                        ServerAddrList *server_addr_list) {
    // Print the IP address
    char ip_str[INET_ADDRSTRLEN]; // Buffer for the IP string
    // converts IP from binary form (net byte order) to readable string
    inet_ntop(AF_INET, &packet_src->sin_addr, ip_str, sizeof(ip_str));

    struct request *req = (struct request *)buffer;  // Initial cast to check req_type
    // printf("req: %d\n", req->req_type);

    if (req->req_type == REQ_LOGIN) { // TODO , make sure username isn't in use
        struct request_login *req_login = (struct request_login *)req; 
        printf("server: %s login in\n", req_login->req_username);
        add_user(user_list, ip_str, ntohs(packet_src->sin_port), req_login->req_username);
    }
    else if (req->req_type == REQ_LOGOUT) {
        logout(user_list, ip_str, packet_src, channel_list);

    } else if (req->req_type == REQ_JOIN) {
        join(req, user_list, ip_str, packet_src, channel_list);

    } else if (req->req_type == REQ_LEAVE) {
        leave(req, user_list, ip_str, packet_src, channel_list);
        
    } else if (req->req_type == REQ_SAY) {
        say(req, s, user_list, ip_str, packet_src, channel_list);

    } else if (req->req_type == REQ_LIST) {
        printf("server: listing channels is not supported\n");

    }
    else if (req->req_type == REQ_WHO) { 
        printf("server: who is in channel is not supported\n");
    }
    print_channels(channel_list);
    User *curr = user_list->head;
    while (curr) {
        printf("users in user list: %s\n", curr->username);
        curr = curr->next;
    }
    printf("-------------------------");
}

// holds all adjacent servers
void create_adjacent_servers(ServerAddrList *server_addr_list, char **argv, int argc) {
    for (int i = 3; i < argc; i++) {
        ServerAddr *new_server = (ServerAddr *)malloc(sizeof(ServerAddr));
        char *ip = argv[i];
        char *port = argv[++i];
        setup_server_addr(&new_server->server_address, ip, port);
        new_server->next = server_addr_list->head;
        server_addr_list->head = new_server;
        server_addr_list->count++;
    }
}

void setup_server_addr(struct sockaddr_in *server_addr, char *host_name, char *port) {
    // Zero out the structure
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(atoi(port));

    // Use "127.0.0.1" if host_name is "localhost"
    // Convert hostname to IP address
    if (inet_pton(AF_INET, host_name, &server_addr->sin_addr) <= 0) {
        // If the hostname is not an IP address, try to resolve it
        struct hostent *host = gethostbyname(host_name);
        if (host == NULL) {
            perror("gethostbyname");
            exit(EXIT_FAILURE);
        }
        memcpy(&server_addr->sin_addr, host->h_addr_list[0], host->h_length);
    }
}

void say(struct request *req, int s, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list) {
    struct request_say *req_say = (struct request_say *)req;
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(packet_src->sin_port));
    printf("server: %s sends say message in %s\n", user->username, req_say->req_channel);
    struct text_say txt_say;
    txt_say.txt_type = 0;
    strcpy(txt_say.txt_channel, req_say->req_channel);
    strcpy(txt_say.txt_username, user->username);
    strcpy(txt_say.txt_text, req_say->req_text);

    Channel *specified_channel = find_channel(channel_list, req_say->req_channel);
    UserList channels_users = specified_channel->users;
    User *current_user = channels_users.head;
    while (current_user) {
        packet_src->sin_port = htons(current_user->port);
        // converts IPV4 from text to binary form
        if (inet_pton(AF_INET, current_user->ip, &(packet_src->sin_addr)) < 1) {
            printf("Error: problem converting str to net byte order"), fflush(stdout);
            exit(EXIT_FAILURE);
        }
        if (sendto(s, &txt_say, sizeof(txt_say), 0, (struct sockaddr *)packet_src, sizeof(*packet_src)) < 0) {
            perror("sendto error");
            exit(EXIT_FAILURE);
        }
        current_user = current_user->next;
    }
}

// TODO channel list isn't being upated - /leave Common, /exit removes Common
void leave(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list) {
    struct request_leave *req_leave = (struct request_leave *)req;
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(packet_src->sin_port));
    Channel *specified_channel = find_channel(channel_list, req_leave->req_channel);
    if (specified_channel != NULL) {
        printf("server: %s leaves channel %s\n", user->username, specified_channel->name);
        remove_user_from_channel(specified_channel, user->username);
        // specified_channel->count -= 1;
        if (specified_channel->count == 0) {
            printf("server: removing empty channel %s\n", specified_channel->name);
            remove_channel(channel_list, specified_channel->name);
        }
    } else {
        printf("server: %s trying to leave non-existent channel %s\n", user->username, req_leave->req_channel);
    }
}

// void s2sjoin() {
//     // send join to adjacent servers and add their server ID's to local channel
//     ServerAddr *current_send = server_addr_list->head;
//     while (current_send) {
//         if (sendto(s, &req_join, sizeof(req_join), 0, (struct sockaddr *)(&current_send->server_address), sizeof(current_send->server_address)) < 0) {
//             perror("sendto error");
//             exit(EXIT_FAILURE);
//         }
//         current_send = current_send->next;
//     }
// }

void join(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, 
        ChannelList *channel_list) {
    struct request_join *req_join = (struct request_join *)req;
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(packet_src->sin_port));
    printf("server: %s join channel %s\n", user->username, req_join->req_channel);
    // Channel *head_channel = channel_list.head;
    Channel *specified_channel = find_channel(channel_list, req_join->req_channel);
    // if channel found
    if (specified_channel) {
        add_user_to_channel(specified_channel, ip_str, ntohs(packet_src->sin_port), user->username);
        specified_channel->count += 1;
    }
    // if channel not found
    else {
        // adding channel
        add_channel(channel_list, req_join->req_channel);
        specified_channel = find_channel(channel_list, req_join->req_channel);
        add_user_to_channel(specified_channel, ip_str, ntohs(packet_src->sin_port), user->username);
        channel_list->count += 1;
        specified_channel->count += 1;
    }
}

void logout(UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list) {
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(packet_src->sin_port));
    printf("sever: %s logged out\n", user->username);
    if (user == NULL) {
        printf("user == null\n");
    }
    Channel *current = channel_list->head;
    while (current) {
        remove_user_from_channel(current, user->username);
        // current->count -= 1;
        if (current->count == 0) {
            remove_channel(channel_list, current->name);
        }
        current = current->next;
    }
    remove_user(user_list, user->username);
}

