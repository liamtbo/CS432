#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server_data_structures.h"


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
    new_user->next = user_list->head;
    user_list->head = new_user;
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
        // printf("does user exist in user list: %s\n", current->username);
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
    // printf("here: %s\n", channel->users.head->username);
    if (remove_user(&(channel->users), username)) {
        channel->count -= 1;
    }

}

// Remove a user from UserList by username
int remove_user(UserList *user_list, const char *username) {
    User *current = user_list->head, *previous = NULL;
    // printf("current->username: %s\n", current->username);
    // printf("username1: %s\n", username);
    while (current) {
        if (strcmp(current->username, username) == 0) {
            // printf("seg?\n");
            if (previous) {
                previous->next = current->next;
            } else {
                user_list->head = current->next;   
            }
            free(current);
            return 1;
        }
        // printf("seg?\n");
        previous = current;
        current = current->next;
    }
    return 0;
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