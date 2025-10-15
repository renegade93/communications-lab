#ifndef SUBSCRIBER_COMMON_H
#define SUBSCRIBER_COMMON_H

#include <arpa/inet.h>

#define MAX_TOPIC_LEN 64
#define BUFFER_SIZE   1024

// ---- Common helpers ----
int connect_tcp(const char *ip, int port);
int setup_udp_socket(void);  
void send_subscription_tcp(int sock, const char *topic);
void send_subscription_udp(int sock, const char *topic,
                           struct sockaddr_in *broker_addr);

#endif