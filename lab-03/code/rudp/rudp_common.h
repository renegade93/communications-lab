#ifndef RUDP_COMMON_H
#define RUDP_COMMON_H

#include <stdint.h>

#define RUDP_TIMEOUT_MS 1000
#define MAX_PAYLOAD 1024

// --- NUEVOS FLAGS AÑADIDOS ---
typedef enum {
    PKT_DATA,       // De Publisher a Broker
    PKT_ACK,        // De Broker a Publisher
    PKT_SUB,        // De Subscriber a Broker (¡NUEVO!)
    PKT_SUB_ACK     // De Broker a Subscriber (¡NUEVO!)
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