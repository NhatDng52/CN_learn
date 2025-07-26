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



// Tính checksum (IP + UDP)
unsigned short checksum(unsigned short *buf, int nwords) {
    unsigned long sum = 0;
    for (; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}


int send_udp_packet(const char *data,
                    const char *src_ip,
                    const int src_port,
                    const int dst_port,
                    const char *dst_ip,
                    void (*on_send)(const char *payload, int len)) {

    // chuyển đổi địa chỉ IP từ string sang dạng hexa
    uint32_t SRC_IP = inet_addr(src_ip);
    uint32_t DST_IP = inet_addr(dst_ip);

    // host to network short (htons) chuyển đổi từ định dạng máy chủ sang định dạng mạng
    // host : một số máy lưu small/big-endian, to network : chuyển về big-endian
    // short : 16 bit vì trường port chỉ có 16 bit nên cần ép kiểu về short
    uint16_t SRC_PORT = htons(src_port);
    uint16_t DST_PORT = htons(dst_port);


    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0) {   // thường xảy ra do không có quyền (sudo) hoặc sai hđh ( windows không hỗ trợ raw socket)
        perror("socket() error");
        exit(1);
    }

    // lấy độ dài lấy tỏng dộ dài packet để tinh checksum
    int data_len = strlen(data);
    // datalen chỉ đến ký tự '\0' nên cần +1 để tính cả ký tự kết thúc chuỗi
    // Quên cái này có thể sẽ gây mất dữ liệu nếu receiver set ký tự có nghĩa ở cuối thành \0
    int udp_len = sizeof(struct udp_header) + data_len +1  ; 

    // Cấp phát vùng nhớ UDP packet , clear vùng nhớ 
    char packet[udp_len];
    memset(packet, 0, udp_len);

    // Gán UDP header, map 8 byte của struct udp_header vào vùng nhớ packet, để truy cập các trường của UDP header bằng tên trường thay vì dùng offset
    struct udp_header *udp = (struct udp_header *) packet;
  
    udp->source = SRC_PORT;
    udp->dest = DST_PORT;
    udp->len = htons(udp_len);
    udp->check = 0; // Chưa tính checksum, sẽ tính sau

    // Copy payload(data)
    memcpy(packet + sizeof(struct udp_header), data, data_len);

    // Tạo pseudo header để tính checksum
    struct pseudo_header psh;
    psh.src_address = SRC_IP; 
    psh.dst_address = DST_IP;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_len = htons(udp_len);
    // printf("data : %s\n", data);
    // printf("payload length: %d\n", data_len);
    // printf("udp length: %d\n", udp_len);
    
    // Gộp pseudo header + UDP để tính checksum
    int psize = sizeof(struct pseudo_header) + udp_len;
    char *pseudogram = malloc(psize);
    memcpy(pseudogram, &psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), packet, udp_len);

    // Vì checksum theo lý thuyết được tính mỗi 16 bit , psize trả về dạng bội số của byte(8 bit) nên cần chia 2 để lấy bội số của 16 bit 
    udp->check = checksum((unsigned short *) pseudogram, psize / 2);
    free(pseudogram);

    // Gửi packet cho network ( vì chúng ta chỉ dừng ở transport )
    printf("size of packet: %ld\n", sizeof(packet));
    int result = send_to_net(sock, packet, udp_len, psh.src_address, psh.dst_address); //Chỉ cần send k cần hand shake
    if (result < 0) {
        perror("Error sending packet to network layer");
        close(sock);
        return -1;
    }
    close(sock); // Đóng socket sau khi gửi xong

    if (on_send) {
        on_send(data, data_len);
    } 
    return 0;
}