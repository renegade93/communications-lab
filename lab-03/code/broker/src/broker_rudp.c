#include "broker_common.h"
#include "rudp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr, client_addr;
    server_addr = (struct sockaddr_in){0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    printf("[Broker Reformado] Escuchando en el puerto %d...\n", port);

    RudpPacket packet_in;
    socklen_t client_len = sizeof(client_addr);
    uint32_t last_pub_seq_num = 0;

    while (1) {
        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0,
                         (struct sockaddr*)&client_addr, &client_len);
        if (n <= 0) continue;

        // Caso 1: Es un paquete de datos fiable del Publisher
        if (n >= sizeof(RudpHeader) && packet_in.header.type == PKT_DATA) {
            uint32_t seq_num = ntohl(packet_in.header.seq_num);

            // Responder inmediatamente al Publisher con un ACK
            RudpPacket ack_out;
            ack_out.header.type = PKT_ACK;
            ack_out.header.seq_num = htonl(seq_num);
            sendto(sock, &ack_out, sizeof(RudpHeader), 0,
                   (struct sockaddr*)&client_addr, client_len);

            // Si es un paquete duplicado, no lo reenvíes a los suscriptores
            if (seq_num <= last_pub_seq_num) {
                continue;
            }
            last_pub_seq_num = seq_num;

            // Extraer el topic y reenviar el paquete a los suscriptores
            char* sep = strchr(packet_in.payload, '|');
            if (!sep) continue;
            char topic[MAX_TOPIC_LEN];
            strncpy(topic, packet_in.payload, sep - packet_in.payload);
            topic[sep - packet_in.payload] = '\0';
            broker_forward_message_udp(sock, topic, (const char*)&packet_in); // Reenvía el paquete completo
        }
        // Caso 2: Es un mensaje de suscripción simple de un Subscriber
        else if (strncmp(packet_in.payload, "SUBSCRIBE|", 10) == 0) {
            char* topic = packet_in.payload + 10;
            topic[strcspn(topic, "\r\n")] = '\0';
            register_subscription_udp(topic, &client_addr);
        }
    }
    close(sock);
    return 0;
}