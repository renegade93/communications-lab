#include "subscriber_common.h"
#include "rudp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <broker_ip> <puerto> <topic>\n", argv[0]);
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
    
    printf("Suscrito a '%s'. Esperando eventos...\n", topic);

    RudpPacket packet_in;
    uint32_t last_seq_num_seen = 0;

    while (1) {
        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0, NULL, NULL);

        // Ignorar si no es un paquete de datos válido con la cabecera
        if (n < sizeof(RudpHeader) || packet_in.header.type != PKT_DATA) {
            continue;
        }

        uint32_t seq_num = ntohl(packet_in.header.seq_num);

        // Si es un paquete nuevo (no un duplicado), lo procesamos
        if (seq_num > last_seq_num_seen) {
            // Buscar el separador '|' para imprimir solo el contenido del evento
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