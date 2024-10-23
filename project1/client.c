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

    login(&server_addr, username, client_socket);
    join_channel(&server_addr, client_socket, "Common");

    // largest user input is for sending message - 64 Bytes
    char *user_input = (char *)malloc(sizeof(char) * (SAY_MAX));
    if (user_input == NULL) {printf("mem alloc failed\n"); exit(EXIT_FAILURE);}
    prompt_user(user_input);
    
    // parsing
    char **parsed_s = (char **)malloc(sizeof(char *) * 2); // 1: command, 2: arg
    if (parsed_s == NULL) {printf("mem alloc failed\n"); exit(EXIT_FAILURE);}
    for (int i=0; i<2; i++) {
        parsed_s[i] = (char *)malloc(sizeof(char) * CHANNEL_MAX);
        if (parsed_s[i] == NULL) {printf("mem alloc failed\n"); exit(EXIT_FAILURE);}        
    }
    int count = string_parser(parsed_s, user_input);
    
    // creating packeged message
    // struct request req;
    // package(parsed_s, &req);

    cooked_mode();
    
    close(client_socket);
    // freeing
    free(user_input);
    for (int i=0; i<2; i++) {free(parsed_s[i]);}
    free(parsed_s);
}

void join_channel(struct sockaddr_in *server_addr, int client_socket, char *channel) {
    struct request_join req_join;
    req_join.req_type = REQ_JOIN;
    strcpy(req_join.req_channel, channel);

    ssize_t send_message = sendto(client_socket, &req_join, sizeof(req_join), 0,
             (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (send_message < 0) { perror("Error: problem sending join"); exit(EXIT_FAILURE);}

}

void login(struct sockaddr_in *server_addr, char *username, int client_socket) {
    struct request_login req_login;
    req_login.req_type = REQ_LOGIN;
    strcpy(req_login.req_username, username);

    ssize_t send_message = sendto(client_socket, &req_login, sizeof(req_login), 0,
             (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (send_message < 0) { perror("Error: problem sending login"); exit(EXIT_FAILURE);}
}

void setup_server_addr(struct sockaddr_in *server_addr, char *host_name, char *port) {
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(atoi(port));
    // converts string server ip to AF address family
    if (inet_pton(AF_INET, host_name, &server_addr->sin_addr) < 1) {
        perror("Error: problem converting str to net byte order");
        exit(EXIT_FAILURE);
    }
}

void prompt_user(char *user_input) {
    char c;
    int count = 0;
    printf("> "); fflush(stdout);
    
    while (1) {
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
        if (count == (SAY_MAX - 1)) {user_input[count] = '\0'; break;}
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
        count ++;
        strcpy(parsed_s[1], token); // here
    }
    return count;
}