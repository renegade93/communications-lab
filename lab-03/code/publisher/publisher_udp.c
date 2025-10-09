// publisher_udp.c
// Simulates a sports journalist reporting match events over UDP
// Usage: ./publisher_udp <broker_ip> <port> <topic> [--demo]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

#define MAX_LINE 1024
#define EVENTS_PER_MATCH 12

static int sock_global = -1;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static void handle_interrupt(int sig) {
    if (sock_global != -1) {
        close(sock_global);
        fprintf(stdout, "\n[INFO] Connection closed by user (Ctrl+C).\n");
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <broker_ip> <port> <topic> [--demo]\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_interrupt);

    const char *broker_ip = argv[1];
    int port = atoi(argv[2]);
    const char *topic = argv[3];
    int demo = (argc >= 5 && strcmp(argv[4], "--demo") == 0);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");
    sock_global = sock;

    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", broker_ip);
        close(sock);
        return EXIT_FAILURE;
    }

    time_t now = time(NULL);
    fprintf(stdout, "[%ld] [UDP] Ready to send to %s:%d\n", now, broker_ip, port);

    srand(time(NULL));

    if (demo) {
        const char *players[] = {
            "Haaland", "DeBruyne", "Mbappe", "Messi", "Bellingham",
            "Vinicius", "Modric", "Kane", "Lewandowski", "Pedri",
            "Salah", "Odegaard", "Gavi", "Griezmann", "Rashford"
        };
        int num_players = sizeof(players) / sizeof(players[0]);

        const char *event_types[] = {"GOAL", "CARD", "SUB"};
        int num_events = sizeof(event_types) / sizeof(event_types[0]);

        for (int i = 0; i < EVENTS_PER_MATCH; i++) {
            char payload[MAX_LINE];
            int minute = (rand() % 90) + 1;
            const char *etype = event_types[rand() % num_events];

            if (strcmp(etype, "GOAL") == 0) {
                const char *scorer = players[rand() % num_players];
                const char *assist = players[rand() % num_players];
                snprintf(payload, sizeof(payload),
                         "%s|[GOAL] %s scores (assist: %s) at %d'\n",
                         topic, scorer, assist, minute);
            } else if (strcmp(etype, "CARD") == 0) {
                const char *player = players[rand() % num_players];
                snprintf(payload, sizeof(payload),
                         "%s|[CARD] Yellow card for %s at %d'\n",
                         topic, player, minute);
            } else { // SUB
                const char *out = players[rand() % num_players];
                const char *in = players[rand() % num_players];
                snprintf(payload, sizeof(payload),
                         "%s|[SUB] %s replaced by %s at %d'\n",
                         topic, out, in, minute);
            }

            ssize_t n = sendto(sock, payload, strlen(payload), 0,
                               (struct sockaddr*)&broker_addr, sizeof(broker_addr));
            if (n < 0) die("sendto");

            now = time(NULL);
            fprintf(stdout, "[%ld] Sent: %s", now, payload);
            sleep_ms(1000);
        }
    } else {
        char line[MAX_LINE];
        fprintf(stdout, "Type events manually (Ctrl+D to finish):\n");
        while (fgets(line, sizeof(line), stdin)) {
            char payload[MAX_LINE * 2];
            size_t L = strlen(line);
            if (L > 0 && line[L - 1] == '\n') line[L - 1] = '\0';
            snprintf(payload, sizeof(payload), "%s|%s\n", topic, line);

            ssize_t n = sendto(sock, payload, strlen(payload), 0,
                               (struct sockaddr*)&broker_addr, sizeof(broker_addr));
            if (n < 0) die("sendto");

            now = time(NULL);
            fprintf(stdout, "[%ld] Sent: %s", now, payload);
        }
    }

    close(sock);
    fprintf(stdout, "[UDP] Socket closed.\n");

    return EXIT_SUCCESS;
}