// broker_udp.c
// Receives UDP datagrams from publishers and forwards them to subscribers.
// Usage: ./broker_udp <port>

#include "broker_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int bufsize = 262144;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("[Broker-UDP] Listening on port %d...\n", port);

    char buffer[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr*)&client_addr, &client_len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';

        if (strncmp(buffer, "SUBSCRIBE|", 10) == 0) {
            char *topic = buffer + 10;
            topic[strcspn(topic, "\r\n")] = '\0';
            register_subscription_udp(topic, &client_addr);
        } else {
            char *sep = strchr(buffer, '|');
            if (!sep) continue;
            char topic[MAX_TOPIC_LEN];
            strncpy(topic, buffer, sep - buffer);
            topic[sep - buffer] = '\0';
            broker_forward_message_udp(sock, topic, buffer);
        }
    }

    close(sock);
    return 0;
}