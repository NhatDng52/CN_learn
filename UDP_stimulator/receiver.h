// receiver.h
#include <stdbool.h>
#ifndef RECEIVER_H
#define RECEIVER_H

int udp_listen(int PORT, bool (*on_receive)(const char *payload, int len));

#endif
