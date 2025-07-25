// receiver.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9090

int main() {
    int sockfd;
    char buffer[1024];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    printf("Receiver ready on port %d...\n", PORT);
    len = sizeof(cliaddr);

    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &len);
    buffer[n] = '\0';
    printf("Received: %s from %s:%d\n", buffer, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

    close(sockfd);
    return 0;
}
