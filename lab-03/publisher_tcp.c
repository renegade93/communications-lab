// publisher_tcp.c
// Envia mensajes a un broker usando TCP.
// Uso: ./publisher_tcp <broker_ip> <puerto> <topic> [--demo]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define MAX_LINE 1024

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

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <broker_ip> <puerto> <topic> [--demo]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *broker_ip = argv[1];
    int port = atoi(argv[2]);
    const char *topic = argv[3];
    int demo = (argc >= 5 && strcmp(argv[4], "--demo") == 0);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) die("socket");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, broker_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "IP invalida: %s\n", broker_ip);
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        die("connect");
    }
    fprintf(stdout, "[TCP] Conectado a %s:%d\n", broker_ip, port);

    if (demo) {
        for (int i = 1; i <= 10; i++) {
            char payload[MAX_LINE];
            snprintf(payload, sizeof(payload), "%s|Gol o evento #%d\n", topic, i);
            size_t len = strlen(payload);
            size_t sent = 0;
            while (sent < len) {
                ssize_t n = send(sock, payload + sent, len - sent, 0);
                if (n < 0) die("send");
                sent += (size_t)n;
            }
            fprintf(stdout, "Enviado: %s", payload);
            sleep_ms(1000);
        }
    } else {
        char line[MAX_LINE];
        fprintf(stdout, "Escribe mensajes (Ctrl+D para terminar):\n");
        while (fgets(line, sizeof(line), stdin)) {
            char payload[MAX_LINE * 2];
            size_t L = strlen(line);
            if (L > 0 && line[L-1] == '\n') line[L-1] = '\0';
            snprintf(payload, sizeof(payload), "%s|%s\n", topic, line);
            size_t len = strlen(payload);
            size_t sent = 0;
            while (sent < len) {
                ssize_t n = send(sock, payload + sent, len - sent, 0);
                if (n < 0) die("send");
                sent += (size_t)n;
            }
            fprintf(stdout, "Enviado: %s", payload);
        }
    }

    close(sock);
    fprintf(stdout, "[TCP] Conexión cerrada.\n");
    return EXIT_SUCCESS;
}
