#include "broker_common.h"
#include "rudp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// --- NUEVA FUNCIÓN DE REENVÍO ---
// Esta función SÍ sabe cómo reenviar nuestros RudpPackets
// porque usa un tamaño explícito en lugar de strlen.
void broker_forward_reliable_packet(int sock, const char* topic, RudpPacket* packet, int packet_size) {
    int idx = find_topic(topic);
    if (idx == -1 || topics[idx].num_subs_udp == 0) return;

    for (int i = 0; i < topics[idx].num_subs_udp; i++) {
        struct sockaddr_in* dest = &topics[idx].subs_udp[i];
        
        // Enviamos el paquete usando el tamaño 'packet_size' que recibimos
        if (sendto(sock, packet, packet_size, 0, 
                   (struct sockaddr*)dest, sizeof(*dest)) < 0) {
            perror("sendto (forward)");
        }
    }
    
    printf("[Broker] Reenviado paquete a %d suscriptor(es) para '%s'\n", 
           topics[idx].num_subs_udp, topic);
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr, from_addr;
    server_addr = (struct sockaddr_in){0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    printf("[Broker Reformado] Escuchando en el puerto %d...\n", port);

    RudpPacket packet_in;
    socklen_t from_len = sizeof(from_addr);
    uint32_t last_pub_seq_num = 0;

    while (1) {
        // 'n' es el número de bytes REALES que se recibieron
        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0,
                         (struct sockaddr*)&from_addr, &from_len);
        if (n <= 0) continue;

        if (n >= sizeof(RudpHeader)) {
            switch(packet_in.header.type) {
                case PKT_DATA: {
                    uint32_t seq_num = ntohl(packet_in.header.seq_num);
                    // Enviar ACK al Publisher
                    RudpPacket ack_out;
                    ack_out.header.type = PKT_ACK;
                    ack_out.header.seq_num = htonl(seq_num);
                    sendto(sock, &ack_out, sizeof(RudpHeader), 0, (struct sockaddr*)&from_addr, from_len);

                    if (seq_num > last_pub_seq_num) {
                        last_pub_seq_num = seq_num;
                        char* sep = strchr(packet_in.payload, '|');
                        if (sep) {
                            char topic[MAX_TOPIC_LEN];
                            strncpy(topic, packet_in.payload, sep - packet_in.payload);
                            topic[sep - packet_in.payload] = '\0';
                            
                            // --- LLAMADA CORREGIDA ---
                            // Llamamos a nuestra NUEVA función de reenvío
                            // y le pasamos el paquete y su tamaño exacto 'n'
                            broker_forward_reliable_packet(sock, topic, &packet_in, n);
                        }
                    }
                    break;
                }
                case PKT_SUB: {
                    char* topic = packet_in.payload;
                    register_subscription_udp(topic, &from_addr);
                    
                    // Enviar SUB_ACK al Subscriber
                    RudpPacket sub_ack_out;
                    sub_ack_out.header.type = PKT_SUB_ACK;
                    sub_ack_out.header.seq_num = 0;
                    sendto(sock, &sub_ack_out, sizeof(RudpHeader), 0, (struct sockaddr*)&from_addr, from_len);
                    break;
                }
                default:
                    break;
            }
        }
    }
    close(sock);
    return 0;
}