#ifndef SERVER_LINKED_LIST_H
#define SERVER_LINKED_LIST_H

#include <netinet/in.h> // in_port_t and in_addr
#include <time.h>

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

typedef struct ServerAndTime {
    struct ServerAddr *server;
    time_t time; // TODO update larer to real time type
    struct ServerAndTime *next;
} ServerAndTime;

typedef struct ServerAndTimeList {
    ServerAndTime *head;
} ServerAndTimeList;

// Struct for Channel, which contains a name and a list of users
typedef struct Channel {
    char name[USERNAME_LEN];
    UserList users;  // Linked list of users in this channel
    struct Channel *next;  // Pointer for linked list of channels
    int count; // count number of users in channel
    ServerAndTimeList server_time_list;
} Channel;

// Linked list of Channels
typedef struct ChannelList {
    Channel *head;
    int count;
} ChannelList;

User *create_user(const char *ip, in_port_t port, const char *username);
int remove_user(UserList *user_list, const char *username);
void add_user(UserList *user_list, const char *ip, in_port_t port, const char *username);
User *find_user_by_username(UserList *user_list, const char *username);
User *find_user_by_ip_port(UserList *user_list, const char *ip, in_port_t port);

Channel *create_channel(const char *name);
void add_channel(ChannelList *channel_list, const char *name);
void remove_channel(ChannelList *channel_list, const char *name);
Channel *find_channel(ChannelList *channel_list, const char *name);
void add_user_to_channel(Channel *channel, const char *ip, in_port_t port, const char *username);
void remove_user_from_channel(Channel *channel, const char *username);

void print_channels(const ChannelList *channel_list);
#endif