// receiver.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdbool.h>
#include "receiver.h"

int udp_listen(int PORT, bool (*on_receive)(const char *payload, int len)) {
    int sockfd;
    char buffer[1024];
    struct sockaddr_in cliaddr;
    socklen_t len;

    // Use RAW socket to match sender
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket creation failed - need sudo");
        exit(1);
    }
    // Giới hạn buffer nhận để dễ bị mất gói
    int recv_buf_size = 1024; // 1KB
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(recv_buf_size)) < 0) {
        perror("setsockopt SO_RCVBUF failed");
    }

    // Đặt timeout cho recvfrom (3 giây)
    struct timeval timeout;
    timeout.tv_sec = 3;      // 3 giây timeout
    timeout.tv_usec = 0;     // 0 micro giây
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt SO_RCVTIMEO failed");
    }
    printf("Raw UDP receiver ready on port %d...\n", PORT);
    len = sizeof(cliaddr);

    while (1) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &len);
        
        if (n < 0) {
            perror("recvfrom timeout or error");
            break;
        }

        // Parse IP header
        struct iphdr *ip_header = (struct iphdr *)buffer;
        struct udphdr *udp_header = (struct udphdr *)(buffer + (ip_header->ihl * 4));

        if (ntohs(udp_header->dest) == PORT) {
            char *payload = buffer + (ip_header->ihl * 4) + sizeof(struct udphdr);
            int payload_len = ntohs(udp_header->len) - sizeof(struct udphdr);

            if (payload_len > 0 && payload_len < 1024) {
                payload[payload_len] = '\0';
                if (on_receive) {
                    if (on_receive(payload, payload_len))
                        break;
                } else {
                    printf("Received UDP packet:\n");
                    printf("  From: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(udp_header->source));
                    printf("  Payload: %s\n", payload);
                    break;
                }
            }
        }
    }


    close(sockfd);
    return 0;
}