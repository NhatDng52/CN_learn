// sender.h
#ifndef SENDER_H
#define SENDER_H

int send_udp_packet(int SRC_PORT, int DST_PORT, const char *data,
                    const char *SRC_IP, const char *DST_IP);

#endif
