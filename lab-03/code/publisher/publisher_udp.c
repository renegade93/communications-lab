// publisher_udp.c
// Envia mensajes a un broker usando UDP (datagramas).
// Uso: ./publisher_udp <broker_ip> <puerto> <topic> [--demo]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX_LINE 1024

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void sleep_ms(int ms) {
    usleep(ms * 1000);
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

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
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

    fprintf(stdout, "[UDP] Preparado para enviar a %s:%d\n", broker_ip, port);

    if (demo) {
        for (int i = 1; i <= 10; i++) {
            char payload[MAX_LINE];
            snprintf(payload, sizeof(payload), "%s|Evento rapido #%d", topic, i);
            ssize_t n = sendto(sock, payload, strlen(payload), 0,
                               (struct sockaddr*)&addr, sizeof(addr));
            if (n < 0) die("sendto");
            fprintf(stdout, "Enviado: %s\n", payload);
            sleep_ms(200);
        }
    } else {
        char line[MAX_LINE];
        fprintf(stdout, "Escribe mensajes (Ctrl+D para terminar):\n");
        while (fgets(line, sizeof(line), stdin)) {
            size_t L = strlen(line);
            if (L > 0 && line[L-1] == '\n') line[L-1] = '\0';
            char payload[MAX_LINE * 2];
            snprintf(payload, sizeof(payload), "%s|%s", topic, line);
            ssize_t n = sendto(sock, payload, strlen(payload), 0,
                               (struct sockaddr*)&addr, sizeof(addr));
            if (n < 0) die("sendto");
            fprintf(stdout, "Enviado: %s\n", payload);
        }
    }

    close(sock);
    fprintf(stdout, "[UDP] Socket cerrado.\n");
    return EXIT_SUCCESS;
}
