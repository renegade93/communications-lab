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
#include <ctype.h>

// --- Variables y funciones de envío R-UDP ---
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            else { perror("recvfrom"); break; }
        }

        if (n >= sizeof(RudpHeader) && packet_in.header.type == PKT_ACK &&
            ntohl(packet_in.header.seq_num) == current_seq_num) {
            current_seq_num++;
            break;
        }
    }
}

// --- Funciones de ayuda para la simulación (copiadas de publisher_common.c) ---
static int rand_in(int lo, int hi) { return lo + rand() % (hi - lo + 1); }

static int pick_active(const TeamState* T){
    int pool[MAX_PLAYERS], m=0;
    for(int i=0;i<T->n_st+T->n_bn;i++)
        if(T->on_field[i] && !T->sent_off[i]) pool[m++]=i;
    return (m? pool[rand_in(0,m-1)] : -1);
}
static int pick_two_active(const TeamState* T,int* a,int* b){
    int pool[MAX_PLAYERS], m=0;
    for(int i=0;i<T->n_st+T->n_bn;i++)
        if(T->on_field[i] && !T->sent_off[i]) pool[m++]=i;
    if(m<2) return -1;
    *a = pool[rand_in(0,m-1)];
    do { *b = pool[rand_in(0,m-1)]; } while(*b==*a);
    return 0;
}
static int pick_out_starter(const TeamState* T){
    int pool[STARTERS], m=0;
    for(int i=0;i<T->n_st;i++) if(T->on_field[i] && !T->sent_off[i]) pool[m++]=i;
    return (m? pool[rand_in(0,m-1)] : -1);
}
static int pick_in_bench(TeamState* T){
    int pool[BENCH], m=0;
    for(int j=0;j<T->n_bn;j++)
        if(!T->bench_used[j] && !T->sent_off[STARTERS+j] && !T->on_field[STARTERS+j]) pool[m++]=j;
    if(!m) return -1;
    int bj = pool[rand_in(0,m-1)];
    T->bench_used[bj]=1;
    return STARTERS + bj;
}
static void apply_card(TeamState* T,int idx,int minute,char* out,size_t outsz){
    int direct_red = (rand_in(1,100) <= 20);
    if(direct_red){
        T->sent_off[idx]=1; T->on_field[idx]=0;
        snprintf(out,outsz,"[RED] Direct red card for %s at %d (Player expelled)\n", T->name[idx], minute);
        return;
    }
    if(T->yellows[idx]==0){
        T->yellows[idx]=1;
        snprintf(out,outsz,"[CARD] Yellow card for %s at %d'\n", T->name[idx], minute);
    }else{
        T->sent_off[idx]=1; T->on_field[idx]=0;
        snprintf(out,outsz,"[RED] Second yellow for %s at %d' (Player expelled)\n", T->name[idx], minute);
    }
}


// --- Simulación con LÓGICA COMPLETA Y ALEATORIA ---
void run_full_simulation(const char* topic, const char* teamA, const char* teamB)
{
    srand(time(NULL));
    TeamRoster rA = get_team(teamA), rB = get_team(teamB);
    TeamState A, B;
    build_team_state(&A, rA);
    build_team_state(&B, rB);
    int scoreA = 0, scoreB = 0;

    int mins[EVENTS_PER_MATCH] = {rand_in(1, 7), rand_in(8, 14), rand_in(15, 21), rand_in(22, 28),
                                  rand_in(29, 35), rand_in(36, 42), 45, rand_in(50, 56), rand_in(57, 63),
                                  rand_in(64, 70