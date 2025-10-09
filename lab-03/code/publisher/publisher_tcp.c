#include "publisher_common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc,char* argv[]){
    if(argc<4){ fprintf(stderr,"Usage: %s <broker_ip> <port> <topic>\n",argv[0]); return EXIT_FAILURE; }

    const char* broker_ip=argv[1]; int port=atoi(argv[2]); const char* topic=argv[3];
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr={0}; addr.sin_family=AF_INET; addr.sin_port=htons(port);
    inet_pton(AF_INET,broker_ip,&addr.sin_addr);
    connect(sock,(struct sockaddr*)&addr,sizeof(addr));
    printf("[TCP] Connected to %s:%d\n",broker_ip,port);

    char teamA[64], teamB[64];
    parse_teams(topic,teamA,teamB);
    run_match_simulation(sock,&addr,0,topic,teamA,teamB);

    close(sock);
    printf("[TCP] Connection closed.\n");
    return 0;
}