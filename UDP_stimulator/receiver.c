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
#include <errno.h>
#include "receiver.h"

int udp_listen( const int PORT,
                const int data_buffer_size_bytes,
                const int queue_buffer_size_bytes,
                const int timeout_sec,
                bool (*stop_listen)(const char *payload, int len),
                void (*on_receive)(const char *payload, int len)) {

    int sockfd;
    char buffer[data_buffer_size_bytes];
    
    struct sockaddr_in cliaddr;
    socklen_t len;

    // Tạo raw socket (phải dùng IPPROTO_UDP nhưng vẫn nhận cả IP header vì Linux không cho xử lý UDP độc lập)
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket creation failed - need sudo");
        exit(1);
    }

    // set buffer nhận ( càng nhỏ càng dễ mất gói )
    int recv_buf_size = queue_buffer_size_bytes;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(recv_buf_size)) < 0) {
        perror("setsockopt SO_RCVBUF failed");
    }

    // Đặt timeout cho recvfrom (3 giây)
    if (timeout_sec >= 0) {
        struct timeval timeout;
        timeout.tv_sec = timeout_sec;      // 3 giây timeout
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt SO_RCVTIMEO failed");
        }
    }
    
    // printf("Raw UDP receiver ready on port %d...\n", PORT);

    // lấy len để recv biết ghi bao nhiêu byte lên biến 
    len = sizeof(cliaddr);

    while (1) {
        // Ở đây dữ liệu nhận vào là string nên sẽ trừ 1 byte để chèn \0 vào cuối cùng, kiểu dữ liệu khác có thể k cần
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &len);
        
        // printf("loop\n");
        // check điều kiện để break
        // Với trường hợp mất gói sẽ vòng thì cần timeout để vòng while không chạy vô hạn
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Timeout xảy ra\n");
            } else {
                fprintf(stderr, "Lỗi recvfrom: %s\n", strerror(errno));
            }
            break;
        }
        // mapping các struct vào dữ liệu nhận được

        // struct iphdr *ip_header = (struct iphdr *)buffer;
        struct udphdr *udp_header = (struct udphdr *)(buffer + sizeof(struct iphdr));
        if (ntohs(udp_header->dest) == PORT) {
            char *payload = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);

            // payload length dùng udp_header-> được lấy từ udp header 
            // UDP header thì lấy length của sender 
            // TUY NHIÊN receiver đã cắt mất số byte vượt quá buffer , nhưng trường length vẫn không thay đổi
            // việc đó khiến truy câp payload bị lỗi ( segmentation fault )
            int payload_len = ntohs(udp_header->len) - sizeof(struct udphdr);

            if (payload_len > 0 && payload_len < data_buffer_size_bytes) {
                // printf("payload length: %d\n", payload_len);
                // printf("actual payload length: %ld\n", n - sizeof(struct iphdr) - sizeof(struct udphdr));
                // Chèn ký tự kết thúc chuỗi, thiếu -1 có thể gây segmentation fault
                payload[payload_len-1] = '\0';
                if (stop_listen) {
                    if (stop_listen(payload, payload_len))
                        break;
                    if (on_receive) {
                        on_receive(payload, payload_len);
                    }
                } else {
                    break;
                }
            }
        }
    }


    close(sockfd);
    return 0;
}