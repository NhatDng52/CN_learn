// sender.h
#ifndef SENDER_H
#define SENDER_H

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

int send_udp_packet(const char *data,
                    const char *src_ip,
                    const int src_port,
                    const int dst_port,
                    const char *dst_ip,
                    void (*on_send)(const char *payload, int len));

#endif
