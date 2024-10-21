#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> // socket
#include <netinet/in.h> // in_port_t and in_addr
#include <string.h>
#include <arpa/inet.h> // inet_pton


int main(int argc, char *argv[]) {
    char *host_addr = argv[1];
    char *port = argv[2];

    struct sockaddr_in server_addr;
    // after declaring, server mem loc might have garbage values
    // this might mess up padding
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET; // IPv4 internet protocol
    server_addr.sin_port = htons(atoi(port)); // local port 8080
    // tells os to accept incoming connectons on any net interface
    // any IP address assigned to the machine
    if (inet_pton(AF_INET, host_addr, &server_addr.sin_addr) < 1) {
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

    char buf[100]; int buf_len = sizeof(buf);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    while (1) {
        int r = recvfrom(s, buf, buf_len, 0, (struct sockaddr *)&client, &client_len);
        break;
    }

}