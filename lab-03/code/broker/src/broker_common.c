#include "broker_common.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

Topic topics[MAX_TOPICS];
int num_topics = 0;

// --- Utility ---
int same_addr(struct sockaddr_in *a, struct sockaddr_in *b) {
    return (a->sin_addr.s_addr == b->sin_addr.s_addr &&
            a->sin_port == b->sin_port);
}

// --- Find topic ---
int find_topic(const char *name) {
    for (int i = 0; i < num_topics; i++)
        if (strcmp(topics[i].name, name) == 0)
            return i;
    return -1;
}

// --- Add subscriber (TCP) ---
int register_subscription_tcp(const char *topic, int fd) {
    int idx = find_topic(topic);
    if (idx == -1) {
        if (num_topics >= MAX_TOPICS) {
            fprintf(stderr, "[Broker] Topic limit reached!\n");
            return -1;
        }
        idx = num_topics++;
        strcpy(topics[idx].name, topic);
        topics[idx].num_subs_tcp = 0;
        topics[idx].num_subs_udp = 0;
    }

    // avoid duplicates
    for (int i = 0; i < topics[idx].num_subs_tcp; i++)
        if (topics[idx].subs_tcp[i] == fd)
            return idx;

    topics[idx].subs_tcp[topics[idx].num_subs_tcp++] = fd;
    printf("[Broker] Client %d subscribed to %s (%d TCP subs)\n",
           fd, topic, topics[idx].num_subs_tcp);
    return idx;
}

// --- Add subscriber (UDP) ---
int register_subscription_udp(const char *topic, struct sockaddr_in *addr) {
    int idx = find_topic(topic);
    if (idx == -1) {
        if (num_topics >= MAX_TOPICS) {
            fprintf(stderr, "[Broker] Topic limit reached!\n");
            return -1;
        }
        idx = num_topics++;
        strcpy(topics[idx].name, topic);
        topics[idx].num_subs_tcp = 0;
        topics[idx].num_subs_udp = 0;
    }

    // avoid duplicates
    for (int i = 0; i < topics[idx].num_subs_udp; i++)
        if (same_addr(&topics[idx].subs_udp[i], addr))
            return idx;

    topics[idx].subs_udp[topics[idx].num_subs_udp++] = *addr;
    printf("[Broker] UDP subscriber %s:%d subscribed to %s (%d UDP subs)\n",
           inet_ntoa(addr->sin_addr), ntohs(addr->sin_port),
           topic, topics[idx].num_subs_udp);
    return idx;
}

// --- Forward TCP messages ---
void broker_forward_message_tcp(const char *topic, const char *msg) {
    int idx = find_topic(topic);
    if (idx == -1 || topics[idx].num_subs_tcp == 0) return;

    for (int i = 0; i < topics[idx].num_subs_tcp; i++) {
        int fd = topics[idx].subs_tcp[i];
        if (send(fd, msg, strlen(msg), 0) < 0)
            perror("send");
    }

    printf("[Broker-TCP] Forwarded to %d subscriber(s) for %s\n",
           topics[idx].num_subs_tcp, topic);
}

// --- Forward UDP messages ---
void broker_forward_message_udp(int sock, const char *topic, const char *msg) {
    int idx = find_topic(topic);
    if (idx == -1 || topics[idx].num_subs_udp == 0) return;

    for (int i = 0; i < topics[idx].num_subs_udp; i++) {
        struct sockaddr_in *dest = &topics[idx].subs_udp[i];
        if (sendto(sock, msg, strlen(msg), 0,
                   (struct sockaddr*)dest, sizeof(*dest)) < 0)
            perror("sendto");
    }

    printf("[Broker-UDP] Forwarded to %d subscriber(s) for %s\n",
           topics[idx].num_subs_udp, topic);
}