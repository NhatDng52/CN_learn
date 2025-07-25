// main.c
#include <stdlib.h>    
#include <unistd.h>   
#include <stdint.h>    
#include <stdbool.h>   
#include <stdio.h>
#include <pthread.h>
#include "sender.h"
#include "receiver.h"
#define MAX_THREADS_PER_BATCH 500
void* sender_thread(void* arg) {
    void** argv = (void**)arg; // cast về mảng void*

    int src_port = (int)(intptr_t)argv[0];
    int dst_port = (int)(intptr_t)argv[1];
    const char* data = (const char*)argv[2];
    const char* src_ip = (const char*)argv[3];
    const char* dst_ip = (const char*)argv[4];

    send_udp_packet(src_port, dst_port, data, src_ip, dst_ip);
    return NULL;
}
void* receiver_thread(void* arg) {
    void** argv = (void**)arg;

    int port = (int)(intptr_t)argv[0];
    bool (*callback)(const char*, int) = (bool (*)(const char*, int))argv[1];

    udp_listen(port, callback);
    return NULL;
}


void packet_lost_stimulation(int count) {
    int SRC_PORT = 12345;
    int DST_PORT = 9090;
    const char* SRC_IP = "127.0.0.1";
    const char* DST_IP = "127.0.0.1";

    static volatile int received = 0;

    // Callback khi nhận được packet
    bool on_receive(const char* payload, int len) {
        received++;

        return received >= count;
    }

    printf("=== Packet Lost Simulation ===\n");

    // Tạo receiver trước
    void* recv_args[] = {
        (void*)(intptr_t)DST_PORT,
        (void*)on_receive
    };

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receiver_thread, recv_args);
    sleep(1);  // đảm bảo receiver đã chạy

    pthread_t send_threads[MAX_THREADS_PER_BATCH];

for (int i = 0; i < count; i += MAX_THREADS_PER_BATCH) {
    int batch_size = (i + MAX_THREADS_PER_BATCH <= count)
                   ? MAX_THREADS_PER_BATCH
                   : (count - i);

    for (int j = 0; j < batch_size; j++) {
        int idx = i + j;

        char* msg = malloc(32);
        snprintf(msg, 32, "SEQ_%d", idx);

        void** send_args = malloc(sizeof(void*) * 5);
        send_args[0] = (void*)(intptr_t)SRC_PORT;
        send_args[1] = (void*)(intptr_t)DST_PORT;
        send_args[2] = msg;
        send_args[3] = (void*)SRC_IP;
        send_args[4] = (void*)DST_IP;

        pthread_create(&send_threads[j], NULL, sender_thread, send_args);
    }

    // Join batch
    for (int j = 0; j < batch_size; j++) {
        pthread_join(send_threads[j], NULL);
    }

    if (i % 1000 == 0)
        printf("Sent %d packets, received so far: %d\n", i + batch_size, received);
}

    // Join receiver (sẽ tự thoát khi đủ gói)
    pthread_join(recv_thread, NULL);

    printf(" Done: Received %d / %d packets\n", received, count);
    printf("================================\n");
}
void packet_order_stimulation(int count) {
    int SRC_PORT = 12346;
    int DST_PORT = 9091;
    const char* SRC_IP = "127.0.0.1";
    const char* DST_IP = "127.0.0.1";

    static volatile int received = 0;
    static volatile int last_seq = -1;
    static volatile bool in_order = true;

    bool on_receive(const char* payload, int len) {
        int seq = -1;
        sscanf(payload, "SEQ_%d", &seq);

        printf("Received: %s\n", payload);
        fflush(stdout);  // đảm bảo in ngay lập tức

        if (seq != last_seq + 1) {
            printf("# Out of order! Expected SEQ_%d but got SEQ_%d\n", last_seq + 1, seq);
            in_order = false;
        }

        last_seq = seq;
        received++;

        return received >= count;
    }

    // Khởi động receiver
    void* recv_args[] = {
        (void*)(intptr_t)DST_PORT,
        (void*)on_receive
    };

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receiver_thread, recv_args);
    sleep(1);

    pthread_t send_threads[MAX_THREADS_PER_BATCH];
    for (int i = 0; i < count; i += MAX_THREADS_PER_BATCH) {
        int batch_size = (i + MAX_THREADS_PER_BATCH <= count)
                       ? MAX_THREADS_PER_BATCH
                       : (count - i);

        for (int j = 0; j < batch_size; j++) {
            int idx = i + j;
            char* msg = malloc(32);
            snprintf(msg, 32, "SEQ_%d", idx);

            void** send_args = malloc(sizeof(void*) * 5);
            send_args[0] = (void*)(intptr_t)SRC_PORT;
            send_args[1] = (void*)(intptr_t)DST_PORT;
            send_args[2] = msg;
            send_args[3] = (void*)SRC_IP;
            send_args[4] = (void*)DST_IP;

            pthread_create(&send_threads[j], NULL, sender_thread, send_args);
        }

        for (int j = 0; j < batch_size; j++) {
            pthread_join(send_threads[j], NULL);
        }

        if (i % 1000 == 0)
            printf("Sent %d packets, received so far: %d\n", i + batch_size, received);
    }

    pthread_join(recv_thread, NULL);

    printf("================================\n");
    if (in_order)
        printf(" All packets arrived in order\n");
    else
        printf(" Packets arrived out of order\n");
    printf("================================\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <packet_count>\n", argv[0]);
        return 1;
    }

    int count = atoi(argv[1]);
    if (count <= 0) {
        fprintf(stderr, "Invalid packet count: %s\n", argv[1]);
        return 1;
    }

    packet_order_stimulation(count);
    return 0;
}