// publisher_tcp.c (simplified, medium-realism)
// Usage: ./publisher_tcp <broker_ip> <port> <topic> [--demo]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#define MAX_LINE 1024
#define EVENTS_PER_MATCH 12
#define STARTERS 6
#define BENCH    4
#define MAX_PLAYERS (STARTERS + BENCH)

static int sock_global = -1;

static void die(const char* msg){ perror(msg); exit(EXIT_FAILURE); }
static void sleep_ms(int ms){ struct timespec ts={ms/1000,(ms%1000)*1000000L}; nanosleep(&ts,NULL); }
static int  rand_in(int lo,int hi){ return lo + rand() % (hi - lo + 1); }
static void send_all(int sock,const char* buf){
    size_t len=strlen(buf), sent=0; while(sent<len){ ssize_t n=send(sock,buf+sent,len-sent,0); if(n<0) die("send"); sent+=(size_t)n; }
}

// ---------- reduced, recognizable rosters (6 starters + 4 bench) ----------
static const char* BAR_ST[STARTERS] = { "Ter Stegen","Kounde","Araujo","Pedri","De Jong","Lewandowski" };
static const char* BAR_BN[BENCH]    = { "Lamine Yamal","Christensen","Balde","Raphinha" };

static const char* PSG_ST[STARTERS] = { "Donnarumma","Hakimi","Marquinhos","Vitinha","Dembele","Mbappe" };
static const char* PSG_BN[BENCH]    = { "Nuno Mendes","Ugarte","Zaire-Emery","Kolo Muani" };

static const char* JUV_ST[STARTERS] = { "Szczesny","Bremer","Rabiot","Locatelli","McKennie","Vlahovic" };
static const char* JUV_BN[BENCH]    = { "Chiesa","Weah","Miretti","Kean" };

static const char* RMA_ST[STARTERS] = { "Courtois","Carvajal","Rudiger","Valverde","Bellingham","Vinicius" };
static const char* RMA_BN[BENCH]    = { "Rodrygo","Camavinga","Modric","Brahim Diaz" };

static const char* BAY_ST[STARTERS] = { "Neuer","Kimmich","Davies","Musiala","Sane","Kane" };
static const char* BAY_BN[BENCH]    = { "Coman","Gnabry","Laimer","Choupo-Moting" };

static const char* LIV_ST[STARTERS] = { "Alisson","Van Dijk","TAA","Mac Allister","Szoboszlai","Salah" };
static const char* LIV_BN[BENCH]    = { "Luis Diaz","Jota","Endo","Elliott" };

typedef struct {
    const char** starters; int n_st;
    const char** bench;    int n_bn;
} TeamRoster;

static int ieq(const char* a,const char* b){ return strcasecmp(a,b)==0; }
static TeamRoster get_team(const char* name){
    TeamRoster r={0};
    if(ieq(name,"Barcelona"))       { r.starters=BAR_ST; r.n_st=STARTERS; r.bench=BAR_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"PSG"))             { r.starters=PSG_ST; r.n_st=STARTERS; r.bench=PSG_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"Juventus"))        { r.starters=JUV_ST; r.n_st=STARTERS; r.bench=JUV_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"RealMadrid"))      { r.starters=RMA_ST; r.n_st=STARTERS; r.bench=RMA_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"BayernMunich"))    { r.starters=BAY_ST; r.n_st=STARTERS; r.bench=BAY_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"Liverpool"))       { r.starters=LIV_ST; r.n_st=STARTERS; r.bench=LIV_BN; r.n_bn=BENCH; return r; }
    return r;
}

typedef struct {
    // indices 0..STARTERS-1 = starters, STARTERS..STARTERS+BENCH-1 = bench
    const char* name[MAX_PLAYERS];
    int is_starter[MAX_PLAYERS]; // 1 if starter
    int on_field[MAX_PLAYERS];   // 1 if currently on pitch
    int yellows[MAX_PLAYERS];    // 0,1 (second yellow => red)
    int sent_off[MAX_PLAYERS];   // 1 if red-carded
    int bench_used[BENCH];       // for distinct bench usage in subs
    int n_st, n_bn;
} TeamState;

static void build_team_state(TeamState* T, TeamRoster r){
    memset(T,0,sizeof(*T));
    T->n_st = r.n_st; T->n_bn = r.n_bn;
    for(int i=0;i<r.n_st;i++){ T->name[i]=r.starters[i]; T->is_starter[i]=1; T->on_field[i]=1; }
    for(int j=0;j<r.n_bn;j++){ int idx = STARTERS + j; T->name[idx]=r.bench[j]; T->is_starter[idx]=0; T->on_field[idx]=0; T->bench_used[j]=0; }
}

static void parse_teams(const char* topic,char* A,char* B){
    const char* p=strstr(topic,"_vs_"); if(!p){ strcpy(A,"TeamA"); strcpy(B,"TeamB"); return; }
    size_t la=(size_t)(p-topic); strncpy(A,topic,la); A[la]='\0'; strcpy(B,p+4);
}

// pick any active player (on_field && !sent_off); return index or -1
static int pick_active(const TeamState* T){
    int pool[MAX_PLAYERS]; int m=0;
    for(int i=0;i<T->n_st+T->n_bn;i++)
        if(T->on_field[i] && !T->sent_off[i]) pool[m++]=i;
    if(m==0) return -1;
    return pool[rand_in(0,m-1)];
}

// pick two distinct active players (for scorer/assist); returns 0 on success
static int pick_two_active(const TeamState* T,int* a,int* b){
    int pool[MAX_PLAYERS]; int m=0;
    for(int i=0;i<T->n_st+T->n_bn;i++)
        if(T->on_field[i] && !T->sent_off[i]) pool[m++]=i;
    if(m<2) return -1;
    int i1 = rand_in(0,m-1);
    int i2;
    do { i2 = rand_in(0,m-1); } while (i2==i1);
    *a = pool[i1]; *b = pool[i2]; return 0;
}

// pick a starter to sub OUT (on_field, starter, not sent off)
static int pick_out_starter(const TeamState* T){
    int pool[STARTERS]; int m=0;
    for(int i=0;i<T->n_st;i++)
        if(T->on_field[i] && !T->sent_off[i]) pool[m++]=i;
    if(m==0) return -1;
    return pool[rand_in(0,m-1)];
}

// pick an unused bench to sub IN
static int pick_in_bench(TeamState* T){
    int pool[BENCH]; int m=0;
    for(int j=0;j<T->n_bn;j++)
        if(!T->bench_used[j] && !T->sent_off[STARTERS+j] && !T->on_field[STARTERS+j])
            pool[m++]=j;
    if(m==0) return -1;
    int bj = pool[rand_in(0,m-1)];
    T->bench_used[bj]=1;
    return STARTERS + bj;
}

// apply yellow/red logic; returns a message snippet into buf
static void apply_card(TeamState* T,int idx,int minute,char* out,size_t outsz){
    // 20% direct red
    int direct_red = (rand_in(1,100) <= 20);
    if(direct_red){
        T->sent_off[idx]=1; T->on_field[idx]=0;
        snprintf(out,outsz,"[RED] Direct red card for %s at %d'\n", T->name[idx], minute);
        return;
    }
    // yellow or second yellow -> red
    if(T->yellows[idx]==0){
        T->yellows[idx]=1;
        snprintf(out,outsz,"[CARD] Yellow card for %s at %d'\n", T->name[idx], minute);
    }else{
        T->sent_off[idx]=1; T->on_field[idx]=0;
        snprintf(out,outsz,"[RED] Second yellow for %s at %d'\n", T->name[idx], minute);
    }
}

int main(int argc,char* argv[]){
    if(argc<4){ fprintf(stderr,"Usage: %s <broker_ip> <port> <topic> [--demo]\n", argv[0]); return EXIT_FAILURE; }
    signal(SIGINT, SIG_IGN); // simplified; Ctrl+C exits process, OS closes socket
    srand((unsigned)time(NULL));

    const char* broker_ip = argv[1];
    int port  = atoi(argv[2]);
    const char* topic = argv[3];
    int demo  = (argc>=5 && strcmp(argv[4],"--demo")==0);

    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock<0) die("socket");
    sock_global = sock;

    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET; addr.sin_port=htons(port);
    if(inet_pton(AF_INET,broker_ip,&addr.sin_addr)!=1){ fprintf(stderr,"Invalid IP\n"); close(sock); return EXIT_FAILURE; }
    if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))<0) die("connect");
    printf("[TCP] Connected to %s:%d\n", broker_ip, port);

    // parse teams and get rosters
    char teamA[64], teamB[64]; parse_teams(topic,teamA,teamB);
    TeamRoster rA = get_team(teamA), rB = get_team(teamB);
    if(!rA.starters || !rB.starters){ fprintf(stderr,"Unknown team(s) in topic. Use e.g. Barcelona_vs_PSG\n"); close(sock); return EXIT_FAILURE; }

    // build state
    TeamState A,B; build_team_state(&A,rA); build_team_state(&B,rB);
    int scoreA=0, scoreB=0;

    // minutes buckets with forced 45 & 90 and strict ascending
    int mins[EVENTS_PER_MATCH] = {
        rand_in(1,7), rand_in(8,14), rand_in(15,21), rand_in(22,28),
        rand_in(29,35), rand_in(36,42), 45, rand_in(50,56),
        rand_in(57,63), rand_in(64,70), rand_in(71,77), 90
    };
    for(int i=1;i<EVENTS_PER_MATCH;i++){ if(mins[i]<=mins[i-1]) mins[i]=mins[i-1]+1; if(mins[i]>90) mins[i]=90; }

    if(!demo){
        // manual passthrough mode unchanged
        char line[MAX_LINE];
        while(fgets(line,sizeof(line),stdin)){
            char buf[MAX_LINE*2]; size_t L=strlen(line); if(L>0 && line[L-1]=='\n') line[L-1]='\0';
            snprintf(buf,sizeof(buf),"%s|%s\n", topic, line);
            send_all(sock,buf); printf("Sent: %s", buf);
        }
        close(sock); return 0;
    }

    // demo mode: generate 12 events
    for(int i=0;i<EVENTS_PER_MATCH;i++){
        char payload[MAX_LINE];

        // halftime notification
        if(mins[i]==45){
            snprintf(payload,sizeof(payload),"%s|[HT] 45' Halftime — %s %d-%d %s\n", topic, teamA, scoreA, scoreB, teamB);
            send_all(sock,payload); printf("Sent: %s",payload); sleep_ms(1000); continue;
        }

        // fulltime notification
        if(mins[i]==90){
            snprintf(payload,sizeof(payload),"%s|[FT] 90' Full time — %s %d-%d %s\n", topic, teamA, scoreA, scoreB, teamB);
            send_all(sock,payload); printf("Sent: %s",payload); sleep_ms(1000); continue;
        }

        // choose event type
        int allow_sub = (mins[i] > 55);
        int et = allow_sub ? rand_in(0,2) : rand_in(0,1); // 0=GOAL,1=CARD,2=SUB(if allowed)

        if(et==0){ // GOAL
            int team = rand_in(0,1); // 0=A,1=B
            int s_idx, a_idx;
            TeamState* T = (team==0 ? &A : &B);
            if(pick_two_active(T,&s_idx,&a_idx)!=0){
                // fallback: card if not enough active players
                et=1;
            }else{
                if(team==0) scoreA++; else scoreB++;
                snprintf(payload,sizeof(payload),
                    "%s|[GOAL] %s scores (assist: %s) at %d' — %s %d-%d %s\n",
                    topic, T->name[s_idx], T->name[a_idx], mins[i],
                    teamA, scoreA, scoreB, teamB);
                send_all(sock,payload); printf("Sent: %s",payload); sleep_ms(1000); continue;
            }
        }

        if(et==1){ // CARD
            int team = rand_in(0,1);
            TeamState* T = (team==0 ? &A : &B);
            int p = pick_active(T);
            if(p<0){
                // no active players (rare) -> harmless fallback message
                snprintf(payload,sizeof(payload),"%s|[INFO] Temporary stoppage at %d'\n", topic, mins[i]);
            }else{
                apply_card(T,p,mins[i],payload,sizeof(payload));
            }
            send_all(sock,payload); printf("Sent: %s",payload); sleep_ms(1000); continue;
        }

        // SUB (only after 55')
        int team = rand_in(0,1);
        TeamState* T = (team==0 ? &A : &B);
        int out = pick_out_starter(T);
        int in  = pick_in_bench(T);
        if(out<0 || in<0){
            // fallback if cannot sub
            snprintf(payload,sizeof(payload),"%s|[CARD] Delay of restart at %d'\n", topic, mins[i]);
        }else{
            T->on_field[out] = 0;
            T->on_field[in]  = 1;
            snprintf(payload,sizeof(payload),"%s|[SUB] %s replaced by %s at %d'\n",
                     topic, T->name[out], T->name[in], mins[i]);
        }
        send_all(sock,payload); printf("Sent: %s",payload); sleep_ms(1000);
    }

    close(sock); printf("[TCP] Connection closed.\n");
    return 0;
}