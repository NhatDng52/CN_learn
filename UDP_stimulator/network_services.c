#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>  
#include <stdint.h>      

int send_to_net(int sock, const char *packet, int udp_len, uint32_t src_addr, uint32_t dst_addr) {
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dst_addr;
    printf("packet sended \n");
    return sendto(sock, packet, udp_len, 0, (struct sockaddr *)&dest, sizeof(dest));
}