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

long int *ID_LIST;
int ID_COUNT = 0;
int ID_LIST_SIZE = 128;

int main(int argc, char *argv[]) {

    if (argc % 2 == 0) {
        printf("argc: %d\n", argc);
        printf("error: wrong number of inputs\n");
        exit(EXIT_FAILURE);
    }
    
    ID_LIST = (long int *)malloc(sizeof(long int) * 128);

    char *host_name = argv[1];
    char *port = argv[2];

    // this hold the sockadd_in for every adjacent server
    // PE!!! this was never tested
    ServerAddrList server_addr_list;
    memset(&server_addr_list, 0, sizeof(ServerAddrList));
    create_adjacent_servers(&server_addr_list, argv, argc);

    struct sockaddr_in local_server_addr;
    // after declaring, server mem loc might have garbage values
    // this might mess up padding
    memset(&local_server_addr, 0, sizeof(struct sockaddr_in));
    local_server_addr.sin_family = AF_INET; // IPv4 internet protocol
    local_server_addr.sin_port = htons(atoi(port)); // local port 5000
    // Use "127.0.0.1" if host_name is "localhost"
    // Convert hostname to IP address
    if (inet_pton(AF_INET, host_name, &local_server_addr.sin_addr) <= 0) {
        // If the hostname is not an IP address, try to resolve it
        struct hostent *host = gethostbyname(host_name);
        if (host == NULL) {
            perror("gethostbyname");
            exit(EXIT_FAILURE);
        }
        memcpy(&local_server_addr.sin_addr, host->h_addr_list[0], host->h_length);
    }

    // allocates a socket and returns a descriptor
    // create IPv4, UDP socket, IP for protocols
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { perror("Error calling socket()"); exit(EXIT_FAILURE);}

    // bind socket to specific IP addr and port so it can listen for packet_src connections
    int b = bind(s, (struct sockaddr *)&local_server_addr, sizeof(local_server_addr));
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
                    // buffer[bytes_returned] = '\0';
                    process_requests(&packet_src, &user_list, &channel_list, s, buffer, &server_addr_list, &local_server_addr);
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
                        ServerAddrList *server_addr_list, struct sockaddr_in *local_server_addr) {
    // Print the IP address
    char ip_str[INET_ADDRSTRLEN]; // Buffer for the IP string
    // converts IP from binary form (net byte order) to readable string
    inet_ntop(AF_INET, &packet_src->sin_addr, ip_str, sizeof(ip_str));

    struct request *req = (struct request *)buffer;  // Initial cast to check req_type
    // printf("req: %d\n", req->req_type);

    if (req->req_type == REQ_LOGIN) {
        struct request_login *req_login = (struct request_login *)req; 
        printf("server: %s login in\n", req_login->req_username);
        add_user(user_list, ip_str, ntohs(packet_src->sin_port), req_login->req_username);
    }
    else if (req->req_type == REQ_LOGOUT) {
        logout(user_list, ip_str, packet_src, channel_list);

    } else if (req->req_type == REQ_JOIN) {
        join(req, user_list, ip_str, packet_src, channel_list, server_addr_list, s, local_server_addr);
    } else if (req->req_type == REQ_LEAVE) {
        leave(req, user_list, ip_str, packet_src, channel_list);
        
    } else if (req->req_type == REQ_SAY || req->req_type == S2S_SAY) {
        say(req, s, user_list, ip_str, packet_src, channel_list, local_server_addr);

    } else if (req->req_type == REQ_LIST) {
        printf("server: listing channels is not supported\n");
    } else if (req->req_type == REQ_WHO) { 
        printf("server: who is in channel is not supported\n");
    }
    // print_channels(channel_list);
    // User *curr = user_list->head;
    // while (curr) {
    //     printf("users in user list: %s\n", curr->username);
    //     curr = curr->next;
    // }
    // printf("-------------------------\n");
}

// holds all adjacent servers
void create_adjacent_servers(ServerAddrList *server_addr_list, char **argv, int argc) {
    for (int i = 3; i < argc; i++) {
        ServerAddr *new_server = (ServerAddr *)malloc(sizeof(ServerAddr));
        if (new_server == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
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

void say(struct request *req, int s, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, 
        ChannelList *channel_list, struct sockaddr_in *local_server_addr) {

    struct text_say txt_say;
    struct s2s_say *s2sSay = (struct s2s_say *)malloc(sizeof(struct s2s_say));
    if (s2sSay == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    memset(s2sSay, 0, sizeof(*s2sSay));

    // if packet_src is from user
    if (req->req_type == REQ_SAY) {
        struct request_say *req_say = (struct request_say *)req;
        User *user = find_user_by_ip_port(user_list, ip_str, ntohs(packet_src->sin_port));
        printf("server: %s sends say message in %s\n", user->username, req_say->req_channel);
        txt_say.txt_type = 0;
        strcpy(txt_say.txt_channel, req_say->req_channel);
        strcpy(txt_say.txt_username, user->username);
        strcpy(txt_say.txt_text, req_say->req_text);

        s2sSay->req_type = S2S_SAY;
        s2sSay->id = get_urandom();
        strncpy(s2sSay->username, user->username, USERNAME_MAX);
        strncpy(s2sSay->channel, req_say->req_channel, CHANNEL_MAX);
        strncpy(s2sSay->text, req_say->req_text, SAY_MAX);
    }
    // if packet_src is from other server
    else if (req->req_type == S2S_SAY) {
        // todo: check packet against recent IDs
        memcpy(s2sSay, (struct s2s_say *)req, sizeof(struct s2s_say));

        if (check_id(s2sSay->id)) {
            return;
        }
        if (ID_COUNT == ID_LIST_SIZE) {
            long int *temp = realloc(ID_LIST, ID_COUNT * 2 * sizeof(long int));
            if (temp == NULL) {
                perror("realloc failed");
                free(ID_LIST); // Free the old list to prevent memory leak
                exit(EXIT_FAILURE);
            }
            ID_LIST = temp;
            ID_LIST_SIZE = ID_COUNT * 2;
        }
        ID_LIST[ID_COUNT] = s2sSay->id;
        ID_COUNT++;

        txt_say.txt_type = 0;
        strncpy(txt_say.txt_channel, s2sSay->channel, CHANNEL_MAX);
        strncpy(txt_say.txt_username, s2sSay->username, USERNAME_MAX);
        strncpy(txt_say.txt_text, s2sSay->text, SAY_MAX);

        printf("%d %d recv S2S say %s %s %s\n", ntohs(local_server_addr->sin_port), ntohs(packet_src->sin_port),
        s2sSay->username, s2sSay->channel, s2sSay->text);
    } 

    // send message to all user on local server
    Channel *specified_channel = find_channel(channel_list, txt_say.txt_channel);
    UserList channels_users = specified_channel->users;
    User *current_user = channels_users.head;
    int original_packet_src = packet_src->sin_port;
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
    packet_src->sin_port = original_packet_src;
    // printf("calling say:\n");
    // print_server_ports(channel_list, local_server_addr);
    // printf("---------------------\n");

    // send s2s say message
    ServerAndTime *dst_server = specified_channel->server_time_list.head;
    while (dst_server) {
        if (ntohs(dst_server->server->server_address.sin_port) != ntohs(packet_src->sin_port)) {
            // todo put IP into log
            printf("%d %d send S2S say %s %s %s\n", ntohs(local_server_addr->sin_port), ntohs(dst_server->server->server_address.sin_port),
                    s2sSay->username, s2sSay->channel, s2sSay->text);
            if (sendto(s, s2sSay, sizeof(*s2sSay), 0, (struct sockaddr *)&dst_server->server->server_address, sizeof(dst_server->server->server_address)) < 0) {
                perror("sendto error");
            }
        } 
        dst_server = dst_server->next;
    }
    free(s2sSay);
}

// check if this packet was received recently
int check_id(long int id) {
    for (int i=0; i<ID_COUNT; i++) {
        if (id == ID_LIST[i]) {
            return 1; // this is a recent say packet
        }
    }
    return 0;
}

unsigned int get_urandom() {
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (urandom == NULL) {
        perror("Failed to open /dev/urandom");
        return EXIT_FAILURE;
    }
    unsigned int random_value;
    if (fread(&random_value, sizeof(random_value), 1, urandom) != 1) {
        perror("Failed to read from /dev/urandom");
        fclose(urandom);
        return EXIT_FAILURE;
    }
    fclose(urandom);
    return random_value;
}

void leave(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, ChannelList *channel_list) {
    struct request_leave *req_leave = (struct request_leave *)req;
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(packet_src->sin_port));
    Channel *specified_channel = find_channel(channel_list, req_leave->req_channel);
    if (specified_channel != NULL) {
        printf("server: %s leaves channel %s\n", user->username, specified_channel->name);
        remove_user_from_channel(specified_channel, user->username);
        // specified_channel->count -= 1;
        if (specified_channel->user_count == 0) {
            printf("server: removing empty channel %s\n", specified_channel->name);
            remove_channel(channel_list, specified_channel->name);
        }
    } else {
        printf("server: %s trying to leave non-existent channel %s\n", user->username, req_leave->req_channel);
    }
}

/*handles both client packets and server2server packets
Tells the difference by checking if user exists in database or not*/
void join(struct request *req, UserList *user_list, char *ip_str, struct sockaddr_in *packet_src, 
        ChannelList *channel_list, ServerAddrList *server_addr_list, int s, struct sockaddr_in *local_server_addr) {
    struct request_join *req_join = (struct request_join *)req;
    User *user = find_user_by_ip_port(user_list, ip_str, ntohs(packet_src->sin_port));
    
    // conversion for printing
    char local_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local_server_addr->sin_addr, local_ip_str, INET_ADDRSTRLEN);
    char src_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &packet_src->sin_addr, src_ip_str, INET_ADDRSTRLEN);

    // Channel *head_channel = channel_list.head;
    Channel *specified_channel = find_channel(channel_list, req_join->req_channel);

    // if packet src is sent from a user
    if (user) {
        printf("%s:%d %s:%d recv Request join %s %s\n", local_ip_str, ntohs(local_server_addr->sin_port),
                src_ip_str, ntohs(packet_src->sin_port), user->username, req_join->req_channel);
    }
    // if packet src is a another server and the channel doesn't exist locally
    if (!user && !specified_channel) {
        printf("%s:%d %s:%d recv S2S Join %s\n", local_ip_str, ntohs(local_server_addr->sin_port),
                src_ip_str, ntohs(packet_src->sin_port), req_join->req_channel);
    }

    // PE !! placement might not get all the join, we'll see
    // if packet src is a server and channel exists
    if (!user && specified_channel) {
        // Channel *channel = find_channel(channel_list, req_join->req_channel);
        ServerAndTime *join_src_server = find_server(&specified_channel->server_time_list, packet_src);
        // if server is already subbed to channel, update its time
        if (join_src_server) {
            join_src_server->time = time(NULL);
        }
    }

    // if channel found
    if (specified_channel) {
        // if user doesn't exist, packet_src is another server
        if (user) {
            add_user_to_channel(specified_channel, ip_str, ntohs(packet_src->sin_port), user->username);
            specified_channel->user_count += 1;
        }
    }
    // if channel not found
    else {
        // adding channel
        add_channel(channel_list, req_join->req_channel);
        channel_list->count += 1;
        specified_channel = find_channel(channel_list, req_join->req_channel);

        // add src_packet server to channels subbed servers
        ServerAddr *packet_src_server = (ServerAddr *)malloc(sizeof(ServerAddr));
        memcpy(&packet_src_server->server_address, packet_src, sizeof(struct sockaddr_in));
        packet_src_server->next = NULL;
        
        // if server sent this s2sSay message sub serve to channel
        if (!user) {
            // if server isn't already subbed to channel
            if (!find_server(&specified_channel->server_time_list, &packet_src_server->server_address)) {
                sub_server_to_channel(packet_src_server, specified_channel);
                specified_channel->server_count += 1;
            }
        }
        

        // if user does exist, add user to channel
        if (user) {
            add_user_to_channel(specified_channel, ip_str, ntohs(packet_src->sin_port), user->username);
            specified_channel->user_count += 1;
        }

        // send join to adjacent servers and add their server ID's to local channel
        // for every adjacent server
        ServerAddr *dst_server = server_addr_list->head;
        while (dst_server) {
            // printf("dst_server: %d\n", ntohs(dst_server->server_address.sin_port));
            // for every adjacent server within specified channel

            // check if dst server is in chanenls list of subbed servers
            ServerAndTime *server_subbed_to_channel = specified_channel->server_time_list.head;
            int dst_server_in_channels_servers_flag = 0;
            while (server_subbed_to_channel) {
                // printf("local %d checking if dst_server %d is subbed to channel\n", ntohs(local_server_addr->sin_port), ntohs(dst_server->server_address.sin_port));
                if (ntohs(server_subbed_to_channel->server->server_address.sin_port) == ntohs(dst_server->server_address.sin_port)) {
                    // printf("\tlocal server %d has server %d subbed to channel\n", ntohs(local_server_addr->sin_port), ntohs(dst_server->server_address.sin_port));
                    dst_server_in_channels_servers_flag = 1;
                    break;
                }
                server_subbed_to_channel = server_subbed_to_channel->next;
            }
            // if dst server is not in channels list of subbed servers, sub dst server and send join
            // printf("dst_server_in_channels_servers_flag: %d\n", dst_server_in_channels_servers_flag);
            if (!dst_server_in_channels_servers_flag) {
                // printf("dst server %d is not in channels servers\n", ntohs(dst_server->server_address.sin_port));
                // if server is not already subbed to specified channel
                char curr_send_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &dst_server->server_address.sin_addr, curr_send_ip, INET_ADDRSTRLEN);
                printf("%s:%d %s:%d send S2S Join %s\n", local_ip_str, ntohs(local_server_addr->sin_port),
                    curr_send_ip, ntohs(dst_server->server_address.sin_port), req_join->req_channel);
                if (sendto(s, req_join, sizeof(*req_join), 0, (struct sockaddr *)(&dst_server->server_address), sizeof(dst_server->server_address)) < 0) {
                    perror("sendto error");
                    exit(EXIT_FAILURE);
                }
                sub_server_to_channel(dst_server, specified_channel);
                specified_channel->server_count += 1;
            }
            dst_server = dst_server->next;
        }
    }

    // print_server_ports(channel_list, local_server_addr);
    // printf("---------------------\n");
}

void print_server_ports(ChannelList *channel_list, struct sockaddr_in *local_server_addr) {
    // Check if the channel list is valid
    if (channel_list == NULL || channel_list->head == NULL) {
        printf("Channel list is empty.\n");
        return;
    }

    Channel *current_channel = channel_list->head;
    printf("local server: %d\n", ntohs(local_server_addr->sin_port));
    // Loop through each channel in the list
    while (current_channel != NULL) {
        printf("\tChannel: %s\n", current_channel->name);

        // Get the server time list from the current channel
        ServerAndTime *current_server_time = current_channel->server_time_list.head;

        // Loop through each ServerAndTime entry
        while (current_server_time != NULL) {
            // Extract the ServerAddr structure
            ServerAddr *current_server = current_server_time->server;

            // Loop through all ServerAddr nodes
            while (current_server != NULL) {
                // Print out the port number (using ntohs to handle network byte order)
                int port = ntohs(current_server->server_address.sin_port);
                printf("\t\tServer Port: %d\n", port);

                // Move to the next ServerAddr in the linked list
                current_server = current_server->next;
            }

            // Move to the next ServerAndTime entry
            current_server_time = current_server_time->next;
        }

        // Move to the next channel in the list
        current_channel = current_channel->next;
    }
}


ServerAndTime *find_server(ServerAndTimeList *server_time_list, struct sockaddr_in *target_server) {
    ServerAndTime *curr_server = server_time_list->head;

    while (curr_server) {
        // Compare IP address and port
        if (curr_server->server->server_address.sin_addr.s_addr == target_server->sin_addr.s_addr &&
            curr_server->server->server_address.sin_port == target_server->sin_port) {
            return curr_server; // Found the server
        }
        curr_server = curr_server->next;
    }
    return NULL; // Server not found
}


void sub_server_to_channel(ServerAddr *server, Channel *specified_channel) {
    ServerAndTime *new_server_and_time = (ServerAndTime *)malloc(sizeof(ServerAndTime));
    if (new_server_and_time == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    new_server_and_time->server = server;
    new_server_and_time->time = time(NULL);
    new_server_and_time->next = specified_channel->server_time_list.head;
    specified_channel->server_time_list.head = new_server_and_time;
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
        if (current->user_count == 0) {
            remove_channel(channel_list, current->name);
        }
        current = current->next;
    }
    remove_user(user_list, user->username);
}

