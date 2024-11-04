#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for stdin_fileno
#include <string.h>
#include "raw.h" // for raw and cooked mode
#include "duckchat.h"
#include "client.h"
#include <sys/socket.h>
#include <netinet/in.h> // in_port_t and in_addr
#include <arpa/inet.h> // inet_pton
#include <sys/select.h> // select()

#define USER_INPUT_MAX 256

int main(int argc, char *argv[]) {
    if (argc != 4) {
        perror("Error: Invalid input");
        exit(EXIT_FAILURE);
    }
    char *host_name = argv[1];
    char *port = argv[2];
    char *username = argv[3];
    if (strlen(username) >= USERNAME_MAX) {
        perror("Error: Username needs to be sub 32 chars");
        exit(EXIT_FAILURE);
    }

    raw_mode();

    // user needs to join commons
    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error: problem creating socket");
        exit(EXIT_FAILURE);
    }

    // setting up server address
    struct sockaddr_in server_addr;
    setup_server_addr(&server_addr, host_name, port);

    // user channels 
    // TODO: this is wrong, this should all be stored in server
    int channel_count = 0;
    char active_channel[128];

    login(&server_addr, username, client_socket);
    join_channel(&server_addr, client_socket, "Common", &channel_count, active_channel);

    // largest user input is for sending message - 64 Bytes
    char *user_input = (char *)malloc(sizeof(char) * (USER_INPUT_MAX)); // TODO: needs to be bigger or guardian code
    if (user_input == NULL) {printf("mem alloc failed\n"); exit(EXIT_FAILURE);}
    
    // parsing
    char **parsed_s = (char **)malloc(sizeof(char *) * 2); // 1: command, 2: arg
    if (parsed_s == NULL) {printf("mem alloc failed\n"); exit(EXIT_FAILURE);}
    for (int i=0; i<2; i++) {
        parsed_s[i] = (char *)malloc(sizeof(char) * CHANNEL_MAX);
        if (parsed_s[i] == NULL) {printf("mem alloc failed\n"); exit(EXIT_FAILURE);}        
    }

    while(1) {
        prompt_user(user_input, client_socket, &server_addr);

        // if there was a command given, command_exists > 0, else 0
        int argument = string_parser(parsed_s, user_input);
        if (argument > 0) {
            char *command = parsed_s[0];
            if (strcmp(command, "/exit") == 0) {
                printf("exiting...\n");
                // logout(&server_addr, client_socket);
                cooked_mode();
                exit_program(&server_addr, client_socket);
            } else if (strcmp(command, "/join") == 0) {
                printf("joining %s...\n", parsed_s[1]);
                // TODO: realloc if channel is full
                // save channel to users channels
                join_channel(&server_addr, client_socket, parsed_s[1], &channel_count, active_channel);                
                printf("active channel: %s\n", active_channel);  
            } else if (strcmp(command, "/leave") == 0) {
                // printf("leaving %s...\n", parsed_s[1]);
                leave_channel(&server_addr, client_socket, parsed_s[1]);
                // is user left active channel
                if (strcmp(parsed_s[1], active_channel) == 0) {
                    strcpy(active_channel, "None");
                }
            }else if (strcmp(command, "/list") == 0) {
                struct request_list req_list;
                req_list.req_type = REQ_LIST;
                if (sendto(client_socket, &req_list, sizeof(req_list), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    printf("/list failed to send\n");
                    cooked_mode();
                    exit(EXIT_FAILURE);
                }
            } else if (strcmp(command, "/who") == 0) { // TODO, needs info from server
                struct request_who req_who;
                req_who.req_type = REQ_WHO;
                strcpy(req_who.req_channel, parsed_s[1]);
                if (sendto(client_socket, &req_who, sizeof(req_who), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    printf("/who failed to send\n");
                    cooked_mode();
                    exit(EXIT_FAILURE);
                }
            } else if (strcmp(command, "/switch") == 0) {
                strcpy(active_channel, parsed_s[1]);
                printf("active channel: %s\n", active_channel);  
            } 
            else {
                printf("%s is an invalid command. Please enter one of the following: \
                        \n\t/exit\n\t/join\n\t/leave\n\t/list\n\t/who\n\t/switch\n", parsed_s[1]);
            }
        } else {
            // printf("say command\n");
            if (strcmp(active_channel, "None") == 0) {
                printf("No active channel: Please join or switch to a channel\n");
            } else {
                // TODO: add say command structure
                say_command(&server_addr, client_socket, active_channel, user_input);
            }
        }
    }

    cooked_mode();
    
    close(client_socket);
    // freeing
    free(user_input);
    for (int i=0; i<2; i++) {free(parsed_s[i]);}
    free(parsed_s);
}

void say_command(struct sockaddr_in *server_addr, int client_socket,  
                  char *active_channel, char *message) {
    struct request_say req_say;
    req_say.req_type = REQ_SAY;
    strcpy(req_say.req_channel, active_channel);
    // printf("say_command():message: %s\n", message);
    strcpy(req_say.req_text, message);

    ssize_t send_message = sendto(client_socket, &req_say, sizeof(req_say), 0,
             (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (send_message < 0) { perror("Error: problem sending say"); exit(EXIT_FAILURE);}
}

// logout user and exit the client software
void exit_program(struct sockaddr_in *server_addr, int client_socket) {
    logout(server_addr, client_socket);
    printf("exited application\n");
    exit(EXIT_SUCCESS);
}

// joins named channel, creating the channel if it does not exist
void join_channel(struct sockaddr_in *server_addr, int client_socket, 
                  char *channel, int *channel_count, 
                  char *active_channel) {
    if (strlen(channel) > 127) { // 127 bc of null character
        printf("Failed to add: Please keep channel name under 127 chars\n"); fflush(stdout);
        return;
    }

    struct request_join req_join;
    req_join.req_type = REQ_JOIN;
    strcpy(req_join.req_channel, channel);

    // add in new channel to users list of channels
    strcpy(active_channel, channel);
    (*channel_count)++;

    ssize_t send_message = sendto(client_socket, &req_join, sizeof(req_join), 0,
             (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (send_message < 0) { perror("Error: problem sending join"); exit(EXIT_FAILURE);}
}

// leave the names channel
void leave_channel(struct sockaddr_in *server_addr, int client_socket, char *channel) {
    struct request_leave req_leave;
    req_leave.req_type = REQ_LEAVE;
    strcpy(req_leave.req_channel, channel);

    ssize_t send_message = sendto(client_socket, &req_leave, sizeof(req_leave), 0,
            (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (send_message < 0) { perror("Error: problem sending join"); exit(EXIT_FAILURE);}
}

// login to channel
void login(struct sockaddr_in *server_addr, char *username, int client_socket) {
    struct request_login req_login;
    req_login.req_type = REQ_LOGIN;
    strcpy(req_login.req_username, username);

    ssize_t send_message = sendto(client_socket, &req_login, sizeof(req_login), 0,
             (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (send_message < 0) { perror("Error: problem sending login"); exit(EXIT_FAILURE);}
}

// logout
void logout(struct sockaddr_in *server_addr, int client_socket) {
    struct request_logout req_logout;
    req_logout.req_type = REQ_LOGOUT;

    if (sendto(client_socket, &req_logout, sizeof(req_logout), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        printf("logout failed to send\n");
        cooked_mode();
        exit(EXIT_FAILURE);
    }

}

void prompt_user(char *user_input, int client_socket, struct sockaddr_in *server_addr) {
    char c;
    int count = 0;
    user_input[0] = '\0';
    fd_set rfds;
    // int retval;
    printf("> "); fflush(stdout);
    char buffer[sizeof(struct text_say)]; 
    socklen_t buf_size = sizeof(buffer);

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(client_socket, &rfds);
        if (select(client_socket + 1, &rfds, NULL, NULL, NULL) < 0) {
            perror("select() failed\n");
            cooked_mode();
            exit_program(server_addr, client_socket);
        }
        // if theres an input from the user into stdin
        if (FD_ISSET(client_socket, &rfds)) {
            if (strlen(user_input) > 0) {
                for (int i = 0; i < count + 2; i++) { printf("\b"); } // +2 for >
            } else {
                for (int i = 0; i < 2; i++) { printf("\b"); }
            }
            int bytes_received = recvfrom(client_socket, buffer, sizeof(buffer), 0, 
                                    (struct sockaddr *)server_addr, &buf_size);
            buffer[bytes_received] = '\0';
            struct text *txt = (struct text *)buffer;
            // printf("message received: %d\n", txt->txt_type); fflush(stdout);
            if (txt->txt_type == TXT_SAY) {
                struct text_say *txt_say = (struct text_say *)txt;
                printf("[%s][%s]: %s\n", txt_say->txt_channel, txt_say->txt_username, txt_say->txt_text);
                fflush(stdout);
            }
            else if (txt->txt_type == TXT_LIST) {
                struct text_list *txt_list = (struct text_list *)txt;
                // char list_buf[32];
                char list_buf[sizeof(struct text_list)];
                printf("Channels List:\n"); fflush(stdout);
                printf("\t%s\n", txt_list->txt_channels);
                for (int i = 0; i < txt_list->txt_nchannels-1; i++) {
                    bytes_received = recvfrom(client_socket, list_buf, sizeof(list_buf), 0, 
                        (struct sockaddr *)server_addr, &buf_size);
                    txt_list = (struct text_list *)list_buf;
                    printf("\t%s\n", txt_list->txt_channels); fflush(stdout);
                }
            }
            else if (txt->txt_type == TXT_WHO) {
                struct text_who *txt_who = (struct text_who *)txt;
                char who_buf[sizeof(*txt_who)];
                printf("Who List:\n"); fflush(stdout);
                printf("\t%s\n", txt_who->us_username); // todo how is this getting info in txt_list too
                for (int i = 0; i < txt_who->txt_nusernames-1; i++) {
                    bytes_received = recvfrom(client_socket, who_buf, sizeof(who_buf), 0, 
                        (struct sockaddr *)server_addr, &buf_size);
                    txt_who = (struct text_who *)who_buf;
                    printf("\t%s\n", txt_who->us_username); fflush(stdout);
                }
            }
            printf("> %s", user_input); fflush(stdout);
        }
        if (FD_ISSET(0, &rfds)) {
            read(STDIN_FILENO, &c, 1);
            if (c == '\b' || c == 0x7F) { // backspace key generate diff controlchars depending on terminal
                if (count != 0) {
                    count--;
                    user_input[count] = '\0'; // delete prev char
                    char *backspace = "\b \b"; 
                    printf("%s", backspace); 
                    fflush(stdout);
                }
                continue;
            } else {
                printf("%c", c); fflush(stdout);
            }
            if (c == '\n') {user_input[count] = '\0'; break;}
            user_input[count] = c;
            count++;
            if (count == (USER_INPUT_MAX - 1)) {
                user_input[count] = '\0'; 
                printf("\nmax user input reached\n"); fflush(stdout);
                break;
            }

            // reinit stdin file descriptor
            // TODO reset socket as well later
            FD_ZERO(&rfds);
            FD_SET(0, &rfds);
        }
    }
}

void setup_server_addr(struct sockaddr_in *server_addr, char *host_name, char *port) {
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(atoi(port));
    if (strcmp(host_name, "localhost") == 0) {
        strcpy(host_name, "127.0.0.1");
    }
    // converts string server ip to AF address family
    if (inet_pton(AF_INET, host_name, &server_addr->sin_addr) < 1) {
        printf("Error: problem converting str to net byte order"), fflush(stdout);
        cooked_mode();
        exit(EXIT_FAILURE);
    }
}

// rets number of args and command
// no "/" = 0, "/exit" = 1, "/leave channel" = 2 
int string_parser(char **parsed_s, char*raw_s) {
    if (raw_s[0] != '/') {return 0;} 
    int count = 1;
    char *token;
    token = strtok(raw_s, " "); // get command
    strcpy(parsed_s[0], token);
    // printf("token: %s\n", token); fflush(stdout);
    token = strtok(NULL, " "); // get command args if they exist
    // printf("token: %s\n", token); fflush(stdout);
    if (token != NULL) {
        // count ++;
        strcpy(parsed_s[1], token); // here
    }

    return count;
}