#include "subscriber_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

// --- TCP connect ---
int connect_tcp(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", ip);
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("[Subscriber-TCP] Connected to broker %s:%d\n", ip, port);
    return sock;
}

// --- UDP setup (always random port) ---
int setup_udp_socket(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    // random port
    addr.sin_port = htons(0);  

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

// --- Send SUBSCRIBE (TCP) ---
void send_subscription_tcp(int sock, const char *topic) {
    char msg[MAX_TOPIC_LEN + 20];
    snprintf(msg, sizeof(msg), "SUBSCRIBE|%s\n", topic);
    send(sock, msg, strlen(msg), 0);

    struct sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);
    getsockname(sock, (struct sockaddr*)&local_addr, &len);

    printf("[Subscriber-TCP] Subscribed to topic: %-25s | Local port: %d\n",
           topic, ntohs(local_addr.sin_port));
}

// --- Send SUBSCRIBE (UDP) ---
void send_subscription_udp(int sock, const char *topic,
                           struct sockaddr_in *broker_addr) {
    char msg[MAX_TOPIC_LEN + 20];
    snprintf(msg, sizeof(msg), "SUBSCRIBE|%s\n", topic);
    sendto(sock, msg, strlen(msg), 0,
           (struct sockaddr*)broker_addr, sizeof(*broker_addr));

    struct sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);
    getsockname(sock, (struct sockaddr*)&local_addr, &len);

    printf("[Subscriber-UDP] Subscribed to topic: %-25s | Local port: %d\n",
           topic, ntohs(local_addr.sin_port));
}