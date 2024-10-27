#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> // socket
#include <netinet/in.h> // in_port_t and in_addr
#include <string.h>
#include <arpa/inet.h> // inet_pton
#include "duckchat.h"
#include "server.h"




int main(int argc, char *argv[]) {
    char *host_name = argv[1];
    char *port = argv[2];

    struct sockaddr_in server_addr;
    // after declaring, server mem loc might have garbage values
    // this might mess up padding
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET; // IPv4 internet protocol
    server_addr.sin_port = htons(atoi(port)); // local port 5000
    if (strcmp(host_name, "localhost") == 0) {
        strcpy(host_name, "127.0.0.1");
    }
    // converts IP address (host_name) from textual representation into
    // a binary format and store in server_addr.sin_addr
    if (inet_pton(AF_INET, host_name, &server_addr.sin_addr) < 1) {
        perror("Error: problem converting str to net byte order");
        exit(EXIT_FAILURE);
    }

    // allocates a socket and returns a descriptor
    // create IPv4, UDP socket, IP for protocols
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { perror("Error calling socket()"); exit(EXIT_FAILURE);}

    // bind socket to specific IP addr and port so it can listen for client connections
    int b = bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (b < 0) { perror("Error calling bind()"); exit(EXIT_FAILURE);}

    // TODOL need to change to just req, find req type, convert pointer to that type
    char buffer[sizeof(struct request_say)];  // Use the size of the largest struct

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    ChannelList channel_list = {NULL};
    UserList user_list = {NULL};
    add_channel(&channel_list, "Common");

    while (1) {
        int r = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&client, &client_len);
        // struct sockaddr_in *client_sender = (struct sockaddr_in *)&client;

        // Print the IP address
        char ip_str[INET_ADDRSTRLEN]; // Buffer for the IP string
        // converts IP from binary form (net byte order) to readable string
        inet_ntop(AF_INET, &client.sin_addr, ip_str, sizeof(ip_str));
        // printf("Client IP: %s\n", ip_str);
        // printf("port: %d\n", ntohs(client.sin_port));


        /*where im at todo: when 2 clients are connected, and one leaves we seg*/


        struct request *req = (struct request *)buffer;  // Initial cast to check req_type
        printf("req: %d\n", req->req_type);
        if (req->req_type == REQ_LOGIN) { // TODO , make sure username isn't in use
            printf("login\n");
            struct request_login *req_login = (struct request_login *)req; 

            add_user(&user_list, ip_str, ntohs(client.sin_port), req_login->req_username);
        } 
        else if (req->req_type == REQ_LOGOUT) {
            printf("logout\n");
            struct request_logout *req_logout = (struct request_logout *)req;
            User *user = find_user_by_ip_port(&user_list, ip_str, ntohs(client.sin_port));
            Channel *current = channel_list.head;
            printf("seg?\n");
            while (current) {
                remove_user_from_channel(current, user->username);
                current = current->next;
            }
            remove_user(&user_list, user->username);
        }
        else if (req->req_type == REQ_JOIN) {
            struct request_join *req_join = (struct request_join *)req;
            User *user = find_user_by_ip_port(&user_list, ip_str, ntohs(client.sin_port));
            // Channel *head_channel = channel_list.head;
            Channel *specified_channel = find_channel(&channel_list, req_join->req_channel);
            // if channel found
            if (specified_channel) {
                add_user_to_channel(specified_channel, ip_str, ntohs(client.sin_port), user->username);
            }
            // if channel not found
            else {
                add_channel(&channel_list, req_join->req_channel);
                specified_channel = find_channel(&channel_list, req_join->req_channel);
                // printf("succecfulyl added channel: %s\n", channel_list.head->name);
                // printf("channel %s list head: %s\n", specified_channel->name, specified_channel->users.head);
                add_user_to_channel(specified_channel, ip_str, ntohs(client.sin_port), user->username);
            }
        }
        else if (req->req_type == REQ_LEAVE) {
            struct request_leave *req_leave = (struct request_leave *)req;
            User *user = find_user_by_ip_port(&user_list, ip_str, ntohs(client.sin_port));
            Channel *specified_channel = find_channel(&channel_list, req_leave->req_channel);
            remove_user_from_channel(specified_channel, user->username);
        }

        print_channels(&channel_list);
        printf("-------------------------");
    }
}

// Create a new user
User *create_user(const char *ip, in_port_t port, const char *username) {
    User *new_user = (User *)malloc(sizeof(User));
    if (new_user) {
        strncpy(new_user->ip, ip, INET_ADDRSTRLEN);
        new_user->port = port;
        strncpy(new_user->username, username, USERNAME_LEN);
        new_user->next = NULL;
    }
    return new_user;
}

// Add a user to a UserList
void add_user(UserList *user_list, const char *ip, in_port_t port, const char *username) {
    User *new_user = create_user(ip, port, username);
    new_user->next = user_list->head; // seg faulting here
    user_list->head = new_user;
}

// Remove a user from UserList by username
void remove_user(UserList *user_list, const char *username) {
    User *current = user_list->head, *previous = NULL;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            if (previous) {
                previous->next = current->next;
            } else {
                user_list->head = current->next;   
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

// Find user by username
User *find_user_by_username(UserList *user_list, const char *username) {
    User *current = user_list->head;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Find user by IP and port
User *find_user_by_ip_port(UserList *user_list, const char *ip, in_port_t port) {
    User *current = user_list->head;
    while (current) {
        if (strcmp(current->ip, ip) == 0 && current->port == port) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Create a new channel
Channel *create_channel(const char *name) {
    Channel *new_channel = (Channel *)malloc(sizeof(Channel));
    if (new_channel) {
        strncpy(new_channel->name, name, USERNAME_LEN);
        new_channel->users.head = NULL;
        new_channel->next = NULL;
    }
    return new_channel;
}

// Add a channel to a ChannelList
void add_channel(ChannelList *channel_list, const char *name) {
    Channel *new_channel = create_channel(name);
    new_channel->next = channel_list->head;
    channel_list->head = new_channel;
}

// Remove a channel from ChannelList by name
void remove_channel(ChannelList *channel_list, const char *name) {
    Channel *current = channel_list->head, *previous = NULL;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (previous) {
                previous->next = current->next;
            } else {
                channel_list->head = current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

// Find a channel by name
Channel *find_channel(ChannelList *channel_list, const char *name) {
    Channel *current = channel_list->head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Add a user to a channel
void add_user_to_channel(Channel *channel, const char *ip, in_port_t port, const char *username) {
    add_user(&channel->users, ip, port, username);
}

// Remove a user from a channel
void remove_user_from_channel(Channel *channel, const char *username) {
    remove_user(&(channel->users), username);
}

void print_channels(const ChannelList *channel_list) {
    Channel *current_channel = channel_list->head;
    
    // Traverse through each channel
    while (current_channel) {
        printf("Channel: %s\n", current_channel->name);

        // Traverse through each user in the current channel
        User *current_user = current_channel->users.head;
        while (current_user) {
            printf("  User: %s, IP: %s, Port: %d\n", 
                   current_user->username, 
                   current_user->ip, 
                   current_user->port);
            current_user = current_user->next;
        }
        
        current_channel = current_channel->next;
    }
}