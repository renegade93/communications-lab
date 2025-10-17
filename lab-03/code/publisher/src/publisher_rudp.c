#include "publisher_common.h"
#include "rudp_common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h> // Para revisar el error del timeout

static int sock;
static struct sockaddr_in broker_addr;
static uint32_t current_seq_num = 1;

void send_reliable(const char* data, size_t len) {
    RudpPacket packet_out;
    packet_out.header.type = PKT_DATA;
    packet_out.header.seq_num = htonl(current_seq_num);
    strncpy(packet_out.payload, data, len);
    packet_out.payload[len] = '\0';

    RudpPacket packet_in;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    while (1) {
        printf("[Pub] Intentando enviar DATA(seq=%u)...\n", current_seq_num);
        sendto(sock, &packet_out, sizeof(RudpHeader) + len, 0,
               (struct sockaddr*)&broker_addr, sizeof(broker_addr));

        printf("[Pub] Paquete enviado. Esperando ACK...\n");
        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0,
                         (struct sockaddr*)&from_addr, &from_len);

        if (n < 0) {
            // Comprobamos si el error es por timeout
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("[Pub] Timeout! El ACK no llegó. Retransmitiendo...\n");
                continue; // Vuelve a enviar
            } else {
                perror("[Pub] Error en recvfrom");
                break; // Salir en caso de otro error
            }
        }

        if (n >= sizeof(RudpHeader) &&
            packet_in.header.type == PKT_ACK &&
            ntohl(packet_in.header.seq_num) == current_seq_num) {
            printf("[Pub] ACK(seq=%u) recibido correctamente!\n", current_seq_num);
            current_seq_num++;
            break;
        }
    }
}

// ... La función run_match_simulation_rudp se mantiene igual ...
void run_match_simulation_rudp(const char* topic, const char* teamA, const char* teamB) {
    // ...
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <broker_ip> <port> <topic>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* broker_ip = argv[1];
    int port = atoi(argv[2]);
    const char* topic = argv[3];

    // --- NUEVO: Imprimir los argumentos para verificar ---
    printf("--- Configuración del Publisher ---\n");
    printf("  IP Broker: %s\n", broker_ip);
    printf("  Puerto:    %d\n", port);
    printf("  Topic:     %s\n", topic);
    printf("-----------------------------------\n");


    sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RUDP_TIMEOUT_MS * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error estableciendo timeout");
        return EXIT_FAILURE;
    }


    broker_addr = (struct sockaddr_in){0};
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    run_match_simulation_rudp(topic, "TeamA", "TeamB");

    close(sock);
    printf("[Pub] Simulación completada y socket cerrado.\n");
    return 0;
}