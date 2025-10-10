// broker_udp.c
// Receives UDP datagrams from publishers and forwards them to subscribers by topic.
// Usage: ./broker_udp <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define MAX_TOPICS     50
#define MAX_SUBS       100
#define MAX_TOPIC_LEN  64
#define BUFFER_SIZE    1024

typedef struct {
    char name[MAX_TOPIC_LEN];
    struct sockaddr_in subs[MAX_SUBS];
    int num_subs;
} Topic;

Topic topics[MAX_TOPICS];
int num_topics = 0;

/* Utility: compare two sockaddr_in addresses (IPv4 only) */
int same_addr(struct sockaddr_in *a, struct sockaddr_in *b) {
    return (a->sin_addr.s_addr == b->sin_addr.s_addr &&
            a->sin_port == b->sin_port);
}

/* Find existing topic */
int find_topic(const char *name) {
    for (int i = 0; i < num_topics; i++) {
        if (strcmp(topics[i].name, name) == 0)
            return i;
    }
    return -1;
}

/* Add a subscriber to a topic */
void add_subscriber(const char *topic, struct sockaddr_in *addr) {
    int idx = find_topic(topic);
    if (idx == -1) {
        if (num_topics >= MAX_TOPICS) return;
        idx = num_topics++;
        strcpy(topics[idx].name, topic);
        topics[idx].num_subs = 0;
    }

    // avoid duplicates
    for (int i = 0; i < topics[idx].num_subs; i++) {
        if (same_addr(&topics[idx].subs[i], addr))
            return;
    }

    topics[idx].subs[topics[idx].num_subs++] = *addr;
    printf("[Broker-UDP] Subscriber %s:%d subscribed to %s\n",
           inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), topic);
}

/* Forward a message to all subscribers of the topic */
void forward_to_subs(int sock, const char *topic, const char *msg) {
    int idx = find_topic(topic);
    if (idx == -1 || topics[idx].num_subs == 0)
        return;

    for (int i = 0; i < topics[idx].num_subs; i++) {
        struct sockaddr_in *dest = &topics[idx].subs[i];
        sendto(sock, msg, strlen(msg), 0,
               (struct sockaddr*)dest, sizeof(*dest));
    }

    printf("[Broker-UDP] Forwarded to %d subscriber(s) for topic %s\n",
           topics[idx].num_subs, topic);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); exit(EXIT_FAILURE); }

    // Increase socket buffer size (avoid packet drops)
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
        // Handle subscription or message
        if (strncmp(buffer, "SUBSCRIBE|", 10) == 0) {
            char *topic = buffer + 10;
            topic[strcspn(topic, "\r\n")] = '\0';
            add_subscriber(topic, &client_addr);
        } else {
            // Publisher message: extract topic before first '|'
            char *sep = strchr(buffer, '|');
            if (!sep) continue;
            char topic[MAX_TOPIC_LEN];
            strncpy(topic, buffer, sep - buffer);
            topic[sep - buffer] = '\0';
            forward_to_subs(sock, topic, buffer);
        }
    }

    close(sock);
    return 0;
}