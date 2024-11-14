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

    // char **addresses = (char **)malloc(sizeof(char *) * )
    ServerAddrList server_addr_list;
    memset(&server_addr_list, 0, sizeof(ServerAddrList));
    populate_adjacent_servers(&server_addr_list, argv, argc);

    // ServerAddr *current = server_addr_list.head;
    // while (current) {
    //     printf("%s:%d\n", current->ip, current->port);
    //     current = current->next;
    // }

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

    // bind socket to specific IP addr and port so it can listen for client connections
    int b = bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (b < 0) { perror("Error calling bind()"); exit(EXIT_FAILURE);}

    char buffer[sizeof(struct request_say)];  // Use the size of the largest struct

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

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
                int bytes_returned = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&client, &client_len);
                if (bytes_returned > 0) {
                    buffer[bytes_returned] = '\0';
                    // Print the IP address
                    char ip_str[INET_ADDRSTRLEN]; // Buffer for the IP string
                    // converts IP from binary form (net byte order) to readable string
                    inet_ntop(AF_INET, &client.sin_addr, ip_str, sizeof(ip_str));

                    struct request *req = (struct request *)buffer;  // Initial cast to check req_type
                    // printf("req: %d\n", req->req_type);

                    if (req->req_type == REQ_LOGIN) { // TODO , make sure username isn't in use
                        struct request_login *req_login = (struct request_login *)req; 
                        printf("server: %s login in\n", req_login->req_username);
                        add_user(&user_list, ip_str, ntohs(client.sin_port), req_login->req_username);
                    }
                    else if (req->req_type == REQ_LOGOUT) {
                        logout(&user_list, ip_str, &client, &channel_list);

                    } else if (req->req_type == REQ_JOIN) {
                        join(req, &user_list, ip_str, &client, &channel_list);

                    } else if (req->req_type == REQ_LEAVE) {
                        leave(req, &user_list, ip_str, &client, &channel_list);
                        
                    } else if (req->req_type == REQ_SAY) {
                        say(req, s, &user_list, ip_str, &client, &channel_list);

                    } else if (req->req_type == REQ_LIST) {
                        printf("server: listing channels is not supported\n");

                    }
                    else if (req->req_type == REQ_WHO) { 
                        printf("server: who is in channel is not supported\n");
                    }
                    print_channels(&channel_list);
                    User *curr = user_list.head;
                    while (curr) {
                        printf("users in user list: %s\n", curr->username);
                        curr = curr->next;
                    }
                    printf("-------------------------");
                }
                if (bytes_returned < 0) {
                    perror("recvfrom failed");
                    continue;
                }
            } else {
                printf("Waiting for client data...\n");
            }
        }        
    }
    // free_adjacent_servers(&server_addr_list);
}

// holds all adjacent servers
void populate_adjacent_servers(ServerAddrList *server_addr_list, char **argv, int argc) {
    // ServerAddr new_server_addr;
    for (int i = 3; i < argc; i++) {
        ServerAddr *new_server = (ServerAddr *)malloc(sizeof(ServerAddr));
        strncpy(new_server->ip, argv[i], INET_ADDRSTRLEN);
        new_server->port = atoi(argv[++i]);
        new_server->next = server_addr_list->head;
        server_addr_list->head = new_server;
        server_addr_list->count++;
    }
}

void free_adjacent_servers(ServerAddrList *server_addr_list) {
    ServerAddr *current = server_addr_list->head;
    ServerAddr *temp;
    while (current) {
        temp = current->next; 
        free(current);
        current = temp;
    }
}

void say(struct request *req, int s, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list) {
    struct request_say *req_say = (struct request_say *)req;
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(client->sin_port));
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
        client->sin_port = htons(current_user->port);
        // converts IPV4 from text to binary form
        if (inet_pton(AF_INET, current_user->ip, &(client->sin_addr)) < 1) {
            printf("Error: problem converting str to net byte order"), fflush(stdout);
            exit(EXIT_FAILURE);
        }
        if (sendto(s, &txt_say, sizeof(txt_say), 0, (struct sockaddr *)client, sizeof(*client)) < 0) {
            perror("sendto error");
            exit(EXIT_FAILURE);
        }
        current_user = current_user->next;
    }
}

// TODO channel list isn't being upated - /leave Common, /exit removes Common
void leave(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list) {
    struct request_leave *req_leave = (struct request_leave *)req;
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(client->sin_port));
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

void join(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list) {
    struct request_join *req_join = (struct request_join *)req;
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(client->sin_port));
    printf("server: %s join channel %s\n", user->username, req_join->req_channel);
    // Channel *head_channel = channel_list.head;
    Channel *specified_channel = find_channel(channel_list, req_join->req_channel);
    // if channel found
    if (specified_channel) {
        add_user_to_channel(specified_channel, ip_str, ntohs(client->sin_port), user->username);
        specified_channel->count += 1;
    }
    // if channel not found
    else {
        add_channel(channel_list, req_join->req_channel);
        specified_channel = find_channel(channel_list, req_join->req_channel);
        add_user_to_channel(specified_channel, ip_str, ntohs(client->sin_port), user->username);
        channel_list->count += 1;
        specified_channel->count += 1;
    }
}

void logout(UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list) {
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(client->sin_port));
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

