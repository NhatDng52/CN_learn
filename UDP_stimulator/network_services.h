#ifndef NETWORK_SERVICES_H
#define NETWORK_SERVICES_H

#include <stdint.h>

// Function declaration for sending UDP packet to network
void send_to_net(int sock, const char *packet, int udp_len, uint32_t src_addr, uint32_t dst_addr);

#endif // NETWORK_SERVICES_H