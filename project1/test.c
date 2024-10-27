#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> // For in_port_t
#include <arpa/inet.h>  // For INET_ADDRSTRLEN

#define USERNAME_LEN 50

// Struct for User information
typedef struct User {
    char ip[INET_ADDRSTRLEN];
    in_port_t port;
    char username[USERNAME_LEN];
    struct User *next;  // Pointer for linked list
} User;

// Linked list of Users for easy lookup by username or IP/port
typedef struct UserList {
    User *head;
} UserList;

// Struct for Channel, which contains a name and a list of users
typedef struct Channel {
    char name[USERNAME_LEN];
    UserList users;  // Linked list of users in this channel
    struct Channel *next;  // Pointer for linked list of channels
} Channel;

// Linked list of Channels
typedef struct ChannelList {
    Channel *head;
} ChannelList;

// Function Prototypes
User *create_user(const char *ip, in_port_t port, const char *username);
void add_user(UserList *user_list, const char *ip, in_port_t port, const char *username);
void remove_user(UserList *user_list, const char *username);
User *find_user_by_username(UserList *user_list, const char *username);
User *find_user_by_ip_port(UserList *user_list, const char *ip, in_port_t port);

Channel *create_channel(const char *name);
void add_channel(ChannelList *channel_list, const char *name);
void remove_channel(ChannelList *channel_list, const char *name);
Channel *find_channel(ChannelList *channel_list, const char *name);
void add_user_to_channel(Channel *channel, const char *ip, in_port_t port, const char *username);
void remove_user_from_channel(Channel *channel, const char *username);

// Implementation

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
    remove_user(&channel->users, username);
}

// Example usage
int main() {
    ChannelList channel_list = {NULL};

    add_channel(&channel_list, "general");
    Channel *general_channel = find_channel(&channel_list, "general");
    if (general_channel) {
        add_user_to_channel(general_channel, "127.0.0.1", 5000, "user1");
        add_user_to_channel(general_channel, "127.0.0.2", 5001, "user2");

        User *user = find_user_by_username(&general_channel->users, "user1");
        if (user) {
            printf("Found user: %s, IP: %s, Port: %d\n", user->username, user->ip, user->port);
        }
        
        remove_user_from_channel(general_channel, "user1");
    }
    
    remove_channel(&channel_list, "general");

    return 0;
}
