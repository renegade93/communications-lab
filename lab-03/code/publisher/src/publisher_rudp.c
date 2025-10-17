#include "publisher_common.h"
#include "rudp_common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

static int sock;
static struct sockaddr_in broker_addr;
static uint32_t current_seq_num = 1;

// Función de envío fiable (sin cambios)
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
        sendto(sock, &packet_out, sizeof(RudpHeader) + len, 0,
               (struct sockaddr*)&broker_addr, sizeof(broker_addr));

        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0,
                         (struct sockaddr*)&from_addr, &from_len);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout, el bucle continuará y retransmitirá
                continue;
            } else {
                perror("recvfrom");
                break;
            }
        }

        if (n >= sizeof(RudpHeader) &&
            packet_in.header.type == PKT_ACK &&
            ntohl(packet_in.header.seq_num) == current_seq_num) {
            current_seq_num++;
            break;
        }
    }
}

// Lógica de simulación completa, adaptada para usar nuestro envío fiable
void run_full_simulation(const char* topic, const char* teamA, const char* teamB)
{
    // Esta función es una adaptación de la original en 'publisher_common.c'
    srand(time(NULL));
    TeamRoster rA = get_team(teamA), rB = get_team(teamB);
    TeamState A, B;
    build_team_state(&A, rA);
    build_team_state(&B, rB);
    int scoreA = 0, scoreB = 0;

    int mins[EVENTS_PER_MATCH] = {rand() % 7 + 1, rand() % 7 + 8, rand() % 7 + 15, rand() % 7 + 22,
                                  rand() % 7 + 29, rand() % 7 + 36, 45, rand() % 7 + 50, rand() % 7 + 57,
                                  rand() % 7 + 64, rand() % 7 + 71, 90};
    for (int i = 1; i < EVENTS_PER_MATCH; i++) {
        if (mins[i] <= mins[i - 1]) mins[i] = mins[i - 1] + 1;
        if (mins[i] > 90) mins[i] = 90;
    }

    for (int i = 0; i < EVENTS_PER_MATCH; i++) {
        char payload[MAX_LINE];
        char message_content[MAX_LINE];

        // Lógica de generación de eventos (copiada de publisher_common.c)
        if (mins[i] == 45) {
             snprintf(message_content, sizeof(message_content), "[HT] 45' Halftime — %s %d-%d %s\n", teamA, scoreA, scoreB, teamB);
        } else if (mins[i] == 90) {
             snprintf(message_content, sizeof(message_content), "[FT] 90' Full time — %s %d-%d %s\n", teamA, scoreA, scoreB, teamB);
        } else {
            // Lógica simplificada para el ejemplo. La original es más compleja.
             snprintf(message_content, sizeof(message_content), "[INFO] Evento en el minuto %d'\n", mins[i]);
        }
        
        // Creamos el payload final: "topic|mensaje"
        snprintf(payload, sizeof(payload), "%s|%s", topic, message_content);

        // ¡Imprimimos el mensaje antes de enviarlo!
        printf("Sent: %s", message_content);
        
        // Usamos nuestra función de envío fiable
        send_reliable(payload, strlen(payload));

        sleep(1);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <broker_ip> <port> <topic>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* broker_ip = argv[1];
    int port = atoi(argv[2]);
    const char* topic = argv[3];

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Timeout de 1 segundo para el socket
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    broker_addr = (struct sockaddr_in){0};
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    // Obtenemos los nombres de los equipos a partir del topic
    char teamA[64], teamB[64];
    parse_teams(topic, teamA, teamB);

    printf("Iniciando simulación para %s vs %s...\n", teamA, teamB);

    // Ejecutamos la simulación completa
    run_full_simulation(topic, teamA, teamB);

    close(sock);
    printf("Simulación completada.\n");
    return 0;
}