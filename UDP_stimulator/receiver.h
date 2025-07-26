// receiver.h
#include <stdbool.h>
#ifndef RECEIVER_H
#define RECEIVER_H

int udp_listen(int PORT,
               int data_buffer_size_bytes,
               int queue_buffer_size_bytes,
               int timeout_sec,
               bool (*stop_listen)(const char *payload, int len),
               void (*on_receive)(const char *payload, int len));

#endif
