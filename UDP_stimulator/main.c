// main.c
#include <stdio.h>
#include <pthread.h>
#include "sender.h"
#include "receiver.h"

void* sender_thread(void* arg) {    // Change signature
    send_udp_packet(12345, 9090, "Hello, UDP!", "127.0.0.1", "127.0.0.1");
    return NULL;
}

void* receiver_thread(void* arg) {  // Change signature
    udp_listen(9090);
    return NULL; 
}


int main() {
    pthread_t tid_send, tid_recv;

    printf("Khởi động mô phỏng UDP...\n");

    pthread_create(&tid_recv, NULL, receiver_thread, NULL);
    sleep(2); // đảm bảo receiver chạy trước
    pthread_create(&tid_send, NULL, sender_thread, NULL);

    pthread_join(tid_send, NULL);
    pthread_join(tid_recv, NULL);

    printf("Hoàn tất mô phỏng UDP.\n");

    return 0;
}
