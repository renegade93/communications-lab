// subscriber_udp.c
// Subscribes to multiple topics and prints UDP messages from broker.
// Usage: ./subscriber_udp <broker_ip> <broker_port> <topic1> [topic2 ...]

#include "subscriber_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <broker_ip> <broker_port> <topic1> [topic2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *broker_ip = argv[1];
    int broker_port = atoi(argv[2]);

    int sock = setup_udp_socket();  

    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(broker_port);
    if (inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid broker IP: %s\n", broker_ip);
        exit(EXIT_FAILURE);
    }

    // Subscribe to all topics provided
    for (int i = 3; i < argc; i++) {
        send_subscription_udp(sock, argv[i], &broker_addr);
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);

    while (1) {
        ssize_t n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr*)&src, &srclen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';
        // Ensure clean output
        if (buffer[n - 1] != '\n')
            printf("%s\n", buffer);
        else
            printf("%s", buffer);

        fflush(stdout);
    }

    close(sock);
    return 0;
}