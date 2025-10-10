// subscriber_tcp.c
// Connects to a TCP broker and prints incoming messages for multiple topics.
// Usage: ./subscriber_tcp <broker_ip> <port> <topic1> [topic2 topic3 ...]

#include "subscriber_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <broker_ip> <port> <topic1> [topic2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *broker_ip = argv[1];
    int port = atoi(argv[2]);

    int sock = connect_tcp(broker_ip, port);

    // Subscribe to all topics provided
    for (int i = 3; i < argc; i++) {
        send_subscription_tcp(sock, argv[i]);
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            if (n == 0)
                printf("[Subscriber-TCP] Broker closed connection.\n");
            else
                perror("recv");
            break;
        }
        buffer[n] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }

    close(sock);
    return 0;
}