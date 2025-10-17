#include "subscriber_common.h"
#include "rudp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <broker_ip> <broker_port> <topic>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* broker_ip = argv[1];
    int broker_port = atoi(argv[2]);
    const char* topic = argv[3];

    int sock = setup_udp_socket(0);

    struct sockaddr_in broker_addr = {0};
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(broker_port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    char sub_msg[MAX_TOPIC_LEN + 20];
    snprintf(sub_msg, sizeof(sub_msg), "SUBSCRIBE|%s\n", topic);
    sendto(sock, sub_msg, strlen(sub_msg), 0,
           (struct sockaddr*)&broker_addr, sizeof(broker_addr));

    RudpPacket packet_in;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    uint32_t last_seq_num_seen = 0;

    while (1) {
        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0,
                         (struct sockaddr*)&from_addr, &from_len);

        if (n < sizeof(RudpHeader) || packet_in.header.type != PKT_DATA) {
            continue;
        }

        uint32_t seq_num = ntohl(packet_in.header.seq_num);

        RudpPacket ack_out;
        ack_out.header.type = PKT_ACK;
        ack_out.header.seq_num = htonl(seq_num);

        sendto(sock, &ack_out, sizeof(RudpHeader), 0,
               (struct sockaddr*)&from_addr, from_len);

        if (seq_num > last_seq_num_seen) {
            printf("%s", packet_in.payload);
            last_seq_num_seen = seq_num;
        }
    }

    close(sock);
    return 0;
}