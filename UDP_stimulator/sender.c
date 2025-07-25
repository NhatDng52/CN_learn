// sender.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include "network_services.h"  // Add this include
#include "sender.h"

// định nghĩa struct, theo lý thuyết nó sẽ tạo ra một khối với các vùng nhớ có thể truy cập bằng tên, điều này giúp dễ truy cập hơn việc nhớ từng byte offset trong bộ nhớ.
struct ip_header {
    unsigned char ihl:4, version:4;
    unsigned char tos;
    unsigned short tot_len;
    unsigned short id;
    unsigned short frag_off;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short check;
    unsigned int saddr;
    unsigned int daddr;
};
struct udp_header {
    unsigned short source;
    unsigned short dest;
    unsigned short len;
    unsigned short check;
};

// Pseudo header cho checksum
struct pseudo_header {
    u_int32_t src_address;
    u_int32_t dst_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_len;
};

// Tính checksum (IP + UDP)
unsigned short checksum(unsigned short *buf, int nwords) {
    unsigned long sum = 0;
    for (; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}


int send_udp_packet(int src_port, int dst_port, const char *data,
                    const char *src_ip, const char *dst_ip) {
    // IP là 32-bit, kiểu phù hợp:
    uint32_t SRC_IP = inet_addr(src_ip);
    uint32_t DST_IP = inet_addr(dst_ip);

    // Port là 16-bit, nên dùng htons để đảm bảo đúng byte order:
    uint16_t SRC_PORT = htons(src_port);
    uint16_t DST_PORT = htons(dst_port);


    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0) {   // thường xảy ra do không có quyền (sudo) hoặc sai hđh ( windows không hỗ trợ raw socket)
        perror("socket() error");
        exit(1);
    }

    // Payload
    int data_len = strlen(data);

    // Tổng kích thước UDP packet
    int udp_len = sizeof(struct udp_header) + data_len;

    // Cấp phát vùng nhớ UDP header + payload
    char packet[udp_len];
    memset(packet, 0, udp_len);

    // Gán UDP header, map 8 byte của struct udp_header vào vùng nhớ packet, để truy cập các trường của UDP header bằng tên trường thay vì dùng offset
    struct udp_header *udp = (struct udp_header *) packet;
    // host to network short (htons) chuyển đổi từ định dạng máy chủ sang định dạng mạng
    // host : một số máy lưu small/big-endian, to network : chuyển về big-endian
    // short : 16 bit vì trường port chỉ có 16 bit nên cần ép kiểu về short
    udp->source = SRC_PORT;
    udp->dest = DST_PORT;
    udp->len = htons(udp_len);
    udp->check = 0; // sẽ gán sau

    // Copy payload
    memcpy(packet + sizeof(struct udp_header), data, data_len);

    // Tạo pseudo header
    struct pseudo_header psh;
    psh.src_address = SRC_IP; // chuyển string thành hexa IP
    psh.dst_address = DST_IP;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_len = htons(udp_len);

    // Gộp pseudo header + UDP để tính checksum
    int psize = sizeof(struct pseudo_header) + udp_len;
    char *pseudogram = malloc(psize);
    memcpy(pseudogram, &psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), packet, udp_len);

    // Vì checksum theo lý thuyết được tính mỗi 16 bit , psize trả về dạng bội số của byte(8 bit) nên cần chia 2 để lấy bội số của 16 bit 
    udp->check = checksum((unsigned short *) pseudogram, psize / 2);
    free(pseudogram);

    // Gửi packet cho network ( vì chúng ta chỉ dừng ở transport )
    send_to_net(sock, packet, udp_len, psh.src_address, psh.dst_address); //Chỉ cần send k cần hand shake 
    close(sock); // Đóng socket sau khi gửi xong
    return 0;
}