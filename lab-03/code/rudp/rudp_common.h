#ifndef RUDP_COMMON_H
#define RUDP_COMMON_H

#include <stdint.h>

#define RUDP_PORT 1234
#define RUDP_TIMEOUT_MS 500
#define MAX_PAYLOAD 1024

typedef enum {
    PKT_DATA,
    PKT_ACK
} PacketType;

typedef struct {
    uint32_t seq_num;
    PacketType type;
} RudpHeader;

typedef struct {
    RudpHeader header;
    char payload[MAX_PAYLOAD];
} RudpPacket;

#endif