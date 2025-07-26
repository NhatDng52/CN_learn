// main.c
#include <stdlib.h>    
#include <unistd.h>   
#include <stdint.h>    
#include <stdbool.h>   
#include <stdio.h>
#include <pthread.h>
#include "sender.h"
#include "receiver.h"
#include <string.h>
#define MAX_THREADS_PER_BATCH 500
void* sender_thread(void* arg) {
    void** argv = (void**)arg; // cast về mảng void*

    const int src_port = (int)(intptr_t)argv[0];
    const int dst_port = (int)(intptr_t)argv[1];
    const char* data = (const char*)argv[2];
    const char* src_ip = (const char*)argv[3];
    const char* dst_ip = (const char*)argv[4];
    void (*on_send)(const char *payload, int len) = (void (*)(const char *, int))argv[5];
    
    // Actually send the UDP packet
    send_udp_packet(data, src_ip, src_port, dst_port, dst_ip, on_send);
    
    return NULL;
}
void* receiver_thread(void* arg) {
    void** argv = (void**)arg;

    int port = (int)(intptr_t)argv[0];
    int data_buffer_size_bytes = (int)(intptr_t)argv[1];
    int queue_buffer_size_bytes = (int)(intptr_t)argv[2];
    int timeout_sec = (int)(intptr_t)argv[3];
    bool (*stop_listen)(const char *, int) = (bool (*)(const char *, int))argv[4];
    void (*on_receive)(const char *, int) = (void (*)(const char *, int))argv[5];

    udp_listen(port, data_buffer_size_bytes, queue_buffer_size_bytes, timeout_sec, stop_listen, on_receive);
    return NULL;
}


void packet_lost_stimulation(int count) {
    int SRC_PORT = 12345;
    int DST_PORT = 9090;
    const char* SRC_IP = "127.0.0.1";
    const char* DST_IP = "127.0.0.1";
    int timeout_sec = 3; // Thời gian timeout cho recvfrom
    int queue_buffer_size_bytes = 1024; // Không cần queue buffer size cho test này
    int data_buffer_size_bytes = 1024; // Kích thước buffer cho dữ liệu
    static volatile int received = 0;

    // Callback khi nhận được packet
    void on_receive(const char* payload, int len) {
        received++;
    }

    bool stop_listen(const char* payload, int len) {
        // Dừng khi đã nhận đủ gói
        return received >= count;
    }
    printf("=== Packet Lost Simulation ===\n");

    // Tạo receiver trước
    void* recv_args[] = {
        (void*)(intptr_t)DST_PORT,
        (void*)(intptr_t)data_buffer_size_bytes,  // data_buffer_size_bytes
        (void*)(intptr_t)queue_buffer_size_bytes,     // queue_buffer_size_bytes
        (void*)(intptr_t)timeout_sec,     // timeout_sec
        (void*)stop_listen,
        (void*)on_receive
   
    };

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receiver_thread, recv_args);
    sleep(1);  // đảm bảo receiver đã chạy

    pthread_t send_threads[MAX_THREADS_PER_BATCH];

for (int i = 0; i < count; i += MAX_THREADS_PER_BATCH) { // lặp count/max_threads_per_batch lần 
    
    int batch_size = (i + MAX_THREADS_PER_BATCH <= count)
                   ? MAX_THREADS_PER_BATCH
                   : (count - i);

    // Arrays to store allocated pointers for cleanup
    char* messages[MAX_THREADS_PER_BATCH];
    void** args_arrays[MAX_THREADS_PER_BATCH];

    for (int j = 0; j < batch_size; j++) {
        int idx = i + j;

        char* msg = malloc(32);
        snprintf(msg, 32, "SEQ_%d", idx);
        messages[j] = msg;

        void** send_args = malloc(sizeof(void*) * 6);
        send_args[0] = (void*)(intptr_t)SRC_PORT;
        send_args[1] = (void*)(intptr_t)DST_PORT;
        send_args[2] = msg;
        send_args[3] = (void*)SRC_IP;
        send_args[4] = (void*)DST_IP;
        send_args[5] = NULL; // on_send callback
        args_arrays[j] = send_args;

        pthread_create(&send_threads[j], NULL, sender_thread, send_args);
    }

    // Join batch
    for (int j = 0; j < batch_size; j++) {
        pthread_join(send_threads[j], NULL);
    }

    // Free allocated memory for this batch
    for (int j = 0; j < batch_size; j++) {
        free(messages[j]);
        free(args_arrays[j]);
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
    int timeout_sec = 3; // Thời gian timeout cho recvfrom
    int queue_buffer_size_bytes = 1024 *256; // tăng queue vì test này k test packet lost
    int data_buffer_size_bytes = 1024; // Kích thước buffer cho dữ liệu

    static volatile int received = 0;
    static volatile int last_seq = -1;
    static volatile bool in_order = true;

    void on_receive(const char* payload, int len) {
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
    }
    bool stop_listen(const char* payload, int len) {
        // Dừng khi đã nhận đủ gói
        return received >= count;
    }
    void* recv_args[] = {
        (void*)(intptr_t)DST_PORT,
        (void*)(intptr_t)data_buffer_size_bytes,  // data_buffer_size_bytes
        (void*)(intptr_t)queue_buffer_size_bytes,     // queue_buffer_size_bytes
        (void*)(intptr_t)timeout_sec,     // timeout_sec
        (void*)stop_listen,
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

        // Arrays to store allocated pointers for cleanup
        char* messages[MAX_THREADS_PER_BATCH];
        void** args_arrays[MAX_THREADS_PER_BATCH];

        for (int j = 0; j < batch_size; j++) {
            int idx = i + j;
            char* msg = malloc(32);
            snprintf(msg, 32, "SEQ_%d", idx);
            messages[j] = msg;

            void** send_args = malloc(sizeof(void*) * 6);
            send_args[0] = (void*)(intptr_t)SRC_PORT;
            send_args[1] = (void*)(intptr_t)DST_PORT;
            send_args[2] = msg;
            send_args[3] = (void*)SRC_IP;
            send_args[4] = (void*)DST_IP;
            send_args[5] = NULL; // on_send callback
            args_arrays[j] = send_args;

            pthread_create(&send_threads[j], NULL, sender_thread, send_args);
        }

        for (int j = 0; j < batch_size; j++) {
            pthread_join(send_threads[j], NULL);
        }

        // Free allocated memory for this batch
        for (int j = 0; j < batch_size; j++) {
            free(messages[j]);
            free(args_arrays[j]);
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
void packet_size_stimulation(int payload_len) {
    /*  RẤT HAY, nên tìm hiểu
        
        UDP có trường length đó là tổng độ dài của application data + UDP header(8 byte : 64 bit).
        
        Trường length dài 16 bit, tức udp có thể chứa tối đa 65527 byte (2^16 - 8) dữ liệu, vì UDP header có độ dài cố định là 8 byte.
        
        Do đó, khi tạo gói tin UDP, cần đảm bảo rằng kích thước của payload không vượt quá giới hạn này.

        TUY NHIÊN ở test này nếu bạn truyền hơn 65507 byte thì sẽ bị lỗi ở tầng network : message too long

        Điều này xảy ra vì dưới tầng network cũng dùng trường length là tổng application data + IP header(20 byte : 160 bit) + UDP header(8 byte : 64 bit).

        TUY NHIÊN, trường length đó vẫn là 16 bit, nên tổng độ dài của gói tin IP + UDP không được vượt quá 65535 byte (2^16 - 1).

        Nên khi 1 gói tin có application data > 65507, thêm IP và UDP header sẽ vượt quá 65535, khiến tầng network không thể xử lý được, dẫn đến lỗi "message too long".

        Đó là một trong những giới hạn của thiết kế IPv4, kiến trúc mạng mà ví dụ này đang sử dụng.
    */
    int SRC_PORT = 12345;
    int DST_PORT = 9090;
    const char* SRC_IP = "127.0.0.1";
    const char* DST_IP = "127.0.0.1";
    int timeout_sec = 5; // Thời gian timeout cho recvfrom
    int queue_buffer_size_bytes = payload_len + 28; // Đủ lớn để chứa payload
    int data_buffer_size_bytes = payload_len +28; // Kích thước payload + udp header(8 byte) + ip header(20 byte)

    printf("=== Packet Size Test ===\n");

    // Callback đơn giản (không cần in ra)
    void on_receive(const char* payload, int len) {
        printf(" Received payload of length: %d bytes\n", len);
    }
    bool stop_listen(const char* payload, int len) {
        // Không cần dừng, chạy để hàm on_receive được gọi
        return false;
    }
    void* recv_args[] = {
        (void*)(intptr_t)DST_PORT,
        (void*)(intptr_t)data_buffer_size_bytes,  // data_buffer_size_bytes
        (void*)(intptr_t)queue_buffer_size_bytes,     // queue_buffer_size_bytes
        (void*)(intptr_t)timeout_sec,     // timeout_sec
        (void*)stop_listen,
        (void*)on_receive
   
    };

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receiver_thread, recv_args);
    sleep(1);

    // Tạo payload lớn
    char* payload = malloc(payload_len -1);
    memset(payload, 'A', payload_len);
    payload[payload_len -1] = '\0';

    printf("Sending payload of size: %d bytes for A with one more byte for \\0 \n", payload_len -1);
    // printf("Payload content: %.20s\n", payload); // In 20 ký tự đầu tiên
    void* send_args[] = {
        (void*)(intptr_t)SRC_PORT,
        (void*)(intptr_t)DST_PORT,
        payload,
        (void*)SRC_IP,
        (void*)DST_IP
    };

    pthread_t send_thread;
    pthread_create(&send_thread, NULL, sender_thread, send_args);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    free(payload);

    printf("=== Packet Size Test Done ===\n");
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <test_type: 1=lost, 2=order, 3=size> [count_or_size]\n", argv[0]);
        return 1;
    }

    int mode = atoi(argv[1]);

    switch (mode) {
        case 1: {  // Packet Loss
            if (argc < 3) {
                fprintf(stderr, "Usage: %s 1 <packet_count>\n", argv[0]);
                return 1;
            }
            int count = atoi(argv[2]);
            packet_lost_stimulation(count);
            break;
        }
        case 2: {  // Packet Order
            if (argc < 3) {
                fprintf(stderr, "Usage: %s 2 <packet_count>\n", argv[0]);
                return 1;
            }
            int count = atoi(argv[2]);
            packet_order_stimulation(count);
            break;
        }
        case 3: {  // Packet Size
            int size = (argc >= 3) ? atoi(argv[2]) : 65507;
            packet_size_stimulation(size);
            break;
        }
        default:
            fprintf(stderr, "Invalid mode. Use 1 (lost), 2 (order), or 3 (size).\n");
            return 1;
    }

    return 0;
}
