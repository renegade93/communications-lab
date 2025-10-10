#ifndef BROKER_COMMON_H
#define BROKER_COMMON_H

#include <arpa/inet.h>

#define MAX_TOPICS     50
#define MAX_SUBS       100
#define MAX_TOPIC_LEN  64

typedef struct {
    char name[MAX_TOPIC_LEN];
    struct sockaddr_in subs_udp[MAX_SUBS];  // For UDP subscribers
    int subs_tcp[MAX_SUBS];                 // For TCP subscribers (fds)
    int num_subs_udp;
    int num_subs_tcp;
} Topic;

extern Topic topics[MAX_TOPICS];
extern int num_topics;

// ---- Shared topic management ----
int find_topic(const char *name);
int same_addr(struct sockaddr_in *a, struct sockaddr_in *b);

// ---- Subscription helpers ----
int register_subscription_tcp(const char *topic, int fd);
int register_subscription_udp(const char *topic, struct sockaddr_in *addr);

// ---- Forwarding helpers ----
void broker_forward_message_tcp(const char *topic, const char *msg);
void broker_forward_message_udp(int sock, const char *topic, const char *msg);

#endif