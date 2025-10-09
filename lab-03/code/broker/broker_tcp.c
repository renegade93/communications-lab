// broker_tcp.c
// Receives messages from publishers and forwards them to subscribers.
// Usage: ./broker_tcp <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#define MAX_CLIENTS 100
#define MAX_TOPICS  50
#define MAX_TOPIC_LEN 64
#define BUFFER_SIZE 1024

typedef struct {
    char name[MAX_TOPIC_LEN];
    int subs[MAX_CLIENTS];
    int num_subs;
} Topic;

Topic topics[MAX_TOPICS];
int num_topics = 0;

int find_topic(const char *name) {
    for (int i = 0; i < num_topics; i++) {
        if (strcmp(topics[i].name, name) == 0)
            return i;
    }
    return -1;
}

void add_subscription(const char *topic, int client_fd) {
    int idx = find_topic(topic);
    if (idx == -1) {
        if (num_topics >= MAX_TOPICS) return;
        idx = num_topics++;
        strcpy(topics[idx].name, topic);
        topics[idx].num_subs = 0;
    }

    // avoid duplicates
    for (int i = 0; i < topics[idx].num_subs; i++)
        if (topics[idx].subs[i] == client_fd)
            return;

    topics[idx].subs[topics[idx].num_subs++] = client_fd;
    printf("[Broker] Client %d subscribed to %s\n", client_fd, topic);
}

void forward_to_subs(const char *topic, const char *msg) {
    int idx = find_topic(topic);
    if (idx == -1) return; // no subscribers
    for (int i = 0; i < topics[idx].num_subs; i++) {
        int sub_fd = topics[idx].subs[i];
        send(sub_fd, msg, strlen(msg), 0);
    }
    printf("[Broker] Forwarded to %d subscriber(s) for topic %s\n",
           topics[idx].num_subs, topic);
}

void remove_client(int fd, fd_set *master) {
    for (int i = 0; i < num_topics; i++) {
        for (int j = 0; j < topics[i].num_subs; j++) {
            if (topics[i].subs[j] == fd) {
                topics[i].subs[j] = topics[i].subs[--topics[i].num_subs];
                break;
            }
        }
    }
    close(fd);
    FD_CLR(fd, master);
    printf("[Broker] Client %d disconnected\n", fd);
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
        perror("bind"); exit(EXIT_FAILURE);
    }

    if (listen(listener, 10) < 0) { perror("listen"); exit(EXIT_FAILURE); }
    printf("[Broker] Listening on port %d...\n", port);

    FD_ZERO(&master);
    FD_SET(listener, &master);
    fdmax = listener;

    while (1) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select"); exit(EXIT_FAILURE);
        }

        for (int fd = 0; fd <= fdmax; fd++) {
            if (!FD_ISSET(fd, &read_fds)) continue;

            if (fd == listener) {
                // New connection
                addrlen = sizeof(client_addr);
                newfd = accept(listener, (struct sockaddr*)&client_addr, &addrlen);
                if (newfd == -1) { perror("accept"); continue; }
                FD_SET(newfd, &master);
                if (newfd > fdmax) fdmax = newfd;
                printf("[Broker] New connection from %s:%d (fd=%d)\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), newfd);
            } else {
                // Existing client sent data
                int nbytes = recv(fd, buf, sizeof(buf)-1, 0);
                if (nbytes <= 0) {
                    if (nbytes == 0) printf("[Broker] Client %d hung up\n", fd);
                    else perror("recv");
                    remove_client(fd, &master);
                } else {
                    buf[nbytes] = '\0';

                    // Handle message
                    if (strncmp(buf, "SUBSCRIBE|", 10) == 0) {
                        char *topic = buf + 10;
                        topic[strcspn(topic, "\r\n")] = '\0'; // remove newline
                        add_subscription(topic, fd);
                    } else {
                        // Publisher message
                        char *sep = strchr(buf, '|');
                        if (!sep) continue;
                        char topic[MAX_TOPIC_LEN];
                        strncpy(topic, buf, sep - buf);
                        topic[sep - buf] = '\0';
                        forward_to_subs(topic, buf);
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}