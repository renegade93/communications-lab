#include "publisher_common.h"
#include "rudp_common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

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
        sendto(sock, &packet_out, sizeof(RudpHeader) + len, 0,
               (struct sockaddr*)&broker_addr, sizeof(broker_addr));

        int n = recvfrom(sock, &packet_in, sizeof(packet_in), 0,
                         (struct sockaddr*)&from_addr, &from_len);

        if (n < 0) {
            continue;
        }

        if (n >= sizeof(RudpHeader) &&
            packet_in.header.type == PKT_ACK &&
            ntohl(packet_in.header.seq_num) == current_seq_num) {
            current_seq_num++;
            break;
        }
    }
}

void run_match_simulation_rudp(const char* topic, const char* teamA, const char* teamB)
{
    const char* events[] = {
        "[GOAL] 10' - Lewandowski (Asist: Pedri)",
        "[CARD] 22' - Hakimi (Yellow)",
        "[HT] 45' - Halftime 1-0",
        "[GOAL] 65' - Mbappe (Asist: Dembele)",
        "[SUB] 70' - Raphinha entra por Lamine Yamal",
        "[FT] 90' - Full time 1-1"
    };
    int num_events = 6;

    for (int i = 0; i < num_events; i++) {
        char payload[MAX_LINE];
        snprintf(payload, sizeof(payload), "%s|%s\n", topic, events[i]);
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

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RUDP_TIMEOUT_MS * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    broker_addr = (struct sockaddr_in){0};
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(port);
    inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr);

    run_match_simulation_rudp(topic, "TeamA", "TeamB");

    close(sock);
    return 0;
}