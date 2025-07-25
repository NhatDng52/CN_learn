#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>  
#include <stdint.h>      

void send_to_net(int sock, const char *packet, int udp_len, uint32_t src_addr, uint32_t dst_addr, uint16_t dst_port) {
    printf("Sending packet to network...\n");
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));

    dest.sin_family = AF_INET;
    dest.sin_port = htons(dst_port);  // UDP port, không nhất thiết cần
    dest.sin_addr.s_addr = dst_addr;

    if (sendto(sock, packet, udp_len, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        perror("sendto");
        exit(1);
    }
    printf("Packet sent successfully.\n");
}