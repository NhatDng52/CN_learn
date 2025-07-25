// receiver.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include "receiver.h"

int udp_listen(int PORT) {
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

    printf("Raw UDP receiver ready on port %d...\n", PORT);
    len = sizeof(cliaddr);

    while (1) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &len);
        
        if (n > 0) {
            // Parse IP header
            struct iphdr *ip_header = (struct iphdr *)buffer;
            
            // Skip IP header to get UDP header
            struct udphdr *udp_header = (struct udphdr *)(buffer + (ip_header->ihl * 4));
            
            // Check if this UDP packet is for our port
            if (ntohs(udp_header->dest) == PORT) {
                // Skip IP and UDP headers to get payload
                char *payload = buffer + (ip_header->ihl * 4) + sizeof(struct udphdr);
                int payload_len = ntohs(udp_header->len) - sizeof(struct udphdr);
                
                payload[payload_len] = '\0';
                printf("Received UDP packet:\n");
                printf("  From: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(udp_header->source));
                printf("  To Port: %d\n", ntohs(udp_header->dest));
                printf("  Payload: %s\n", payload);
                break; // Exit after receiving one packet
            }
        }
    }

    close(sockfd);
    return 0;
}