#ifndef SERVER_H
#define SERVER_H
#include "server_data_structures.h"

void join(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list);
void say(struct request *req, int s, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list);
void leave(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list);
void logout(UserList *user_list, char *ip_str, struct sockaddr_in *client, ChannelList *channel_list);



#endif