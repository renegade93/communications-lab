#include "subscriber_common.h"
#include "rudp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

void subscribe_reliable(int sock, const char* topic, struct sockaddr_in* broker_addr) {
    RudpPacket packet_out;
    packet_out.header.type = PKT_SUB;
    packet_out.header.seq_num = 0;
    strncpy(packet_out.payload, topic, MAX_PAYLOAD);

    RudpPacket packet_in;
    printf("Enviando solicitud de suscripción para '%s'...\n", topic);
    while (1) {
        sendto(sock, &packet_out, sizeof(RudpHeader) + strlen(topic) + 1, 0,
               (struct sockaddr*)broker_addr, sizeof(*broker_addr));

        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0, NULL, NULL);

        if (n > 0 && packet_in.header.type == PKT_SUB_ACK) {
            printf("Suscripción confirmada por el broker. Esperando eventos...\n");
            break;
        } else {
            printf("No se recibió confirmación, reintentando suscripción...\n");
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <broker_ip> <puerto> <topic>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* broker_ip = argv[1];
    int broker_port = atoi(argv[2]);
    const char* topic = argv[3];

    int sock = setup_udp_socket(0);
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in broker_addr = {0};
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(broker_port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    subscribe_reliable(sock, topic, &broker_addr);

    RudpPacket packet_in;
    uint32_t last_seq_num_seen = 0;

    while (1) {
        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0, NULL, NULL);

        if (n < sizeof(RudpHeader) || packet_in.header.type != PKT_DATA) {
            continue;
        }

        uint32_t seq_num = ntohl(packet_in.header.seq_num);

        if (seq_num > last_seq_num_seen) {
            char* message_content = strchr(packet_in.payload, '|');
            if (message_content) {
                printf("%s", message_content + 1);
            }
            last_seq_num_seen = seq_num;
        }
    }
    close(sock);
    return 0;
}