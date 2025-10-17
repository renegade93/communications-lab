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

void send_reliable(const char* data, size_t len) {
    RudpPacket packet_out;
    packet_out.header.type = PKT_DATA;
    packet_out.header.seq_num = htonl(current_seq_num);
    strncpy(packet_out.payload, data, len);
    packet_out.payload[len] = '\0';

    RudpPacket packet_in;
    while (1) {
        
        sendto(sock, &packet_out, sizeof(RudpHeader) + len, 0,
               (struct sockaddr*)&broker_addr, sizeof(broker_addr));

        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0, NULL, NULL);

        if (n > 0 && packet_in.header.type == PKT_ACK &&
            ntohl(packet_in.header.seq_num) == current_seq_num) {
            current_seq_num++;
            break;
        }
    }
}

void run_match_simulation_reformed(const char* topic, const char* teamA, const char* teamB) {
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <broker_ip> <puerto> <topic>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* broker_ip = argv[1];
    int port = atoi(argv[2]);
    const char* topic = argv[3];

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    broker_addr = (struct sockaddr_in){0};
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    char teamA[64], teamB[64];
    parse_teams(topic, teamA, teamB);
    
    run_match_simulation(sock, &broker_addr, 1, topic, teamA, teamB);

    close(sock);
    printf("Simulación completada.\n");
    return 0;
}