#ifndef RUDP_COMMON_H
#define RUDP_COMMON_H

#include <stdint.h>

#define RUDP_TIMEOUT_MS 1000
#define MAX_PAYLOAD 1024


typedef enum {
    PKT_DATA,       
    PKT_ACK,        
    PKT_SUB,        
    PKT_SUB_ACK     
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