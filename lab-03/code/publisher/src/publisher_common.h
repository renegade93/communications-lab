#ifndef PUBLISHER_COMMON_H
#define PUBLISHER_COMMON_H

#include <netinet/in.h>

#define MAX_LINE 1024
#define EVENTS_PER_MATCH 12
#define STARTERS 6
#define BENCH 4
#define MAX_PLAYERS (STARTERS + BENCH)

// ---------------- Struct Definitions ----------------
typedef struct {
    const char** starters;
    int n_st;
    const char** bench;
    int n_bn;
} TeamRoster;

typedef struct {
    const char* name[MAX_PLAYERS];
    int on_field[MAX_PLAYERS];
    int yellows[MAX_PLAYERS];
    int sent_off[MAX_PLAYERS];
    int bench_used[BENCH];
    int n_st, n_bn;
} TeamState;

// ---------------- Function Prototypes ----------------
void parse_teams(const char* topic, char* A, char* B);
TeamRoster get_team(const char* name);
void build_team_state(TeamState* T, TeamRoster r);
void run_match_simulation(int sock, struct sockaddr_in* addr, int use_udp,
                          const char* topic, const char* teamA, const char* teamB);

#endif