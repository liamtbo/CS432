#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Set the socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    fd_set read_fds;
    struct timeval timeout;

    while (1) {
        // Clear and set the read_fds for select
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        // Set a timeout for select to avoid blocking indefinitely
        timeout.tv_sec = 1;  // 1-second timeout
        timeout.tv_usec = 0;

        // Wait for data on any socket (here, just one UDP socket)
        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        } else if (activity > 0) {
            // Data available to read
            if (FD_ISSET(sockfd, &read_fds)) {
                // Receive message from client
                int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                              (struct sockaddr *)&client_addr, &addr_len);

                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';  // Null-terminate received data
                    printf("Received from client %s:%d: %s\n",
                           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

                    // You could also send a response to the client here using sendto()
                }
                else if (bytes_received < 0 && errno != EWOULDBLOCK) {
                    perror("recvfrom error");
                }
            }
        } else {
            // No activity; select timed out
            printf("Waiting for client data...\n");
        }
    }

    // Close the socket
    close(sockfd);
    return 0;
}
