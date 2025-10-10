// broker_tcp.c
// Receives messages from publishers and forwards them to TCP subscribers.
// Usage: ./broker_tcp <port>

#include "broker_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

void remove_client(int fd, fd_set *master) {
    for (int i = 0; i < num_topics; i++) {
        for (int j = 0; j < topics[i].num_subs_tcp; j++) {
            if (topics[i].subs_tcp[j] == fd) {
                topics[i].subs_tcp[j] = topics[i].subs_tcp[--topics[i].num_subs_tcp];
                break;
            }
        }
    }
    close(fd);
    FD_CLR(fd, master);
    printf("[Broker-TCP] Client %d disconnected\n", fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int listener, newfd, fdmax;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen;
    char buf[BUFFER_SIZE];
    fd_set master, read_fds;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, 10) < 0) { perror("listen"); exit(EXIT_FAILURE); }
    printf("[Broker-TCP] Listening on port %d...\n", port);

    FD_ZERO(&master);
    FD_SET(listener, &master);
    fdmax = listener;

    while (1) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int fd = 0; fd <= fdmax; fd++) {
            if (!FD_ISSET(fd, &read_fds)) continue;

            if (fd == listener) {
                addrlen = sizeof(client_addr);
                newfd = accept(listener, (struct sockaddr*)&client_addr, &addrlen);
                if (newfd == -1) { perror("accept"); continue; }
                FD_SET(newfd, &master);
                if (newfd > fdmax) fdmax = newfd;
                printf("[Broker-TCP] New connection from %s:%d (fd=%d)\n",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port), newfd);
            } else {
                int nbytes = recv(fd, buf, sizeof(buf)-1, 0);
                if (nbytes <= 0) {
                    if (nbytes == 0)
                        printf("[Broker-TCP] Client %d closed connection\n", fd);
                    else perror("recv");
                    remove_client(fd, &master);
                } else {
                    buf[nbytes] = '\0';

                    if (strncmp(buf, "SUBSCRIBE|", 10) == 0) {
                        char *topic = buf + 10;
                        topic[strcspn(topic, "\r\n")] = '\0';
                        register_subscription_tcp(topic, fd);
                    } else {
                        char *sep = strchr(buf, '|');
                        if (!sep) continue;
                        char topic[MAX_TOPIC_LEN];
                        strncpy(topic, buf, sep - buf);
                        topic[sep - buf] = '\0';
                        broker_forward_message_tcp(topic, buf);
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}