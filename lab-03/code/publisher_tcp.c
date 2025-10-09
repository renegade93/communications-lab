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
#include <signal.h>
#include <time.h>

#define MAX_LINE 1024

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
        fprintf(stdout, "\n[INFO] Conexión cerrada por el usuario (Ctrl+C).\n");
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <broker_ip> <puerto> <topic> [--demo]\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_interrupt);

    const char *broker_ip = argv[1];
    int port = atoi(argv[2]);
    const char *topic = argv[3];
    int demo = (argc >= 5 && strcmp(argv[4], "--demo") == 0);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) die("socket");
    sock_global = sock;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, broker_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "IP invalida: %s\n", broker_ip);
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        die("connect");

    time_t now = time(NULL);
    fprintf(stdout, "[%ld] [TCP] Conectado a %s:%d\n", now, broker_ip, port);

    if (demo) {
        const char *events[] = {
            "Gol del equipo A",
            "Tarjeta amarilla para el #7",
            "Cambio: entra el #9 por el #10",
            "Final del primer tiempo",
            "Inicio del segundo tiempo",
            "Falta peligrosa cerca del área",
            "Gol del equipo B",
            "Tarjeta roja para el portero",
            "Tiempo adicional: 3 minutos",
            "Fin del partido"
        };

        for (int i = 0; i < 10; i++) {
            char payload[MAX_LINE];
            snprintf(payload, sizeof(payload), "%s|%s\n", topic, events[i]);
            size_t len = strlen(payload);
            size_t sent = 0;
            while (sent < len) {
                ssize_t n = send(sock, payload + sent, len - sent, 0);
                if (n < 0) die("send");
                sent += (size_t)n;
            }
            now = time(NULL);
            fprintf(stdout, "[%ld] Enviado: %s", now, payload);
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
            now = time(NULL);
            fprintf(stdout, "[%ld] Enviado: %s", now, payload);
        }
    }

    if (close(sock) < 0)
        perror("close");
    else
        fprintf(stdout, "[TCP] Conexión cerrada correctamente.\n");

    return EXIT_SUCCESS;
}