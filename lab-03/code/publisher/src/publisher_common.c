#include "publisher_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

// ---------------- Utility ----------------
static int rand_in(int lo, int hi) { return lo + rand() % (hi - lo + 1); }
static void sleep_ms(int ms){ struct timespec ts={ms/1000,(ms%1000)*1000000L}; nanosleep(&ts,NULL); }
static int ieq(const char* a,const char* b){ return strcasecmp(a,b)==0; }

// ---------------- Reduced Realistic Rosters ----------------
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

// ---------------- Shared Functions ----------------
void parse_teams(const char* topic, char* A, char* B) {
    const char* p = strstr(topic, "_vs_");
    if (!p) { strcpy(A,"TeamA"); strcpy(B,"TeamB"); return; }
    size_t la = (size_t)(p - topic);
    strncpy(A, topic, la); A[la] = '\0';
    strcpy(B, p+4);
}

TeamRoster get_team(const char* name){
    TeamRoster r={0};
    if(ieq(name,"Barcelona"))       { r.starters=BAR_ST; r.n_st=STARTERS; r.bench=BAR_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"PSG"))             { r.starters=PSG_ST; r.n_st=STARTERS; r.bench=PSG_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"Juventus"))        { r.starters=JUV_ST; r.n_st=STARTERS; r.bench=JUV_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"RealMadrid"))      { r.starters=RMA_ST; r.n_st=STARTERS; r.bench=RMA_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"BayernMunich"))    { r.starters=BAY_ST; r.n_st=STARTERS; r.bench=BAY_BN; r.n_bn=BENCH; return r; }
    if(ieq(name,"Liverpool"))       { r.starters=LIV_ST; r.n_st=STARTERS; r.bench=LIV_BN; r.n_bn=BENCH; return r; }
    return r;
}

void build_team_state(TeamState* T, TeamRoster r){
    memset(T,0,sizeof(*T));
    T->n_st=r.n_st; T->n_bn=r.n_bn;
    for(int i=0;i<r.n_st;i++){ T->name[i]=r.starters[i]; T->on_field[i]=1; }
    for(int j=0;j<r.n_bn;j++){ T->name[STARTERS+j]=r.bench[j]; }
}

// ---------------- Helper selectors ----------------
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
static void apply_card(TeamState* T,int idx,int minute,char* out,size_t outsz,const char* topic){
    int direct_red = (rand_in(1,100) <= 20);
    if(direct_red){
        T->sent_off[idx]=1; T->on_field[idx]=0;
        snprintf(out,outsz,"%s|[RED] Direct red card for %s at %d (Player expelled)\n", topic, T->name[idx], minute);
        return;
    }
    if(T->yellows[idx]==0){
        T->yellows[idx]=1;
        snprintf(out,outsz,"%s|[CARD] Yellow card for %s at %d'\n", topic, T->name[idx], minute);
    }else{
        T->sent_off[idx]=1; T->on_field[idx]=0;
        snprintf(out,outsz,"%s|[RED] Second yellow for %s at %d' (Player expelled)\n", topic, T->name[idx], minute);
    }
}

// ---------------- Main Simulation ----------------
void run_match_simulation(int sock, struct sockaddr_in* addr, int use_udp,
                          const char* topic, const char* teamA, const char* teamB)
{
    srand(time(NULL));
    TeamRoster rA=get_team(teamA), rB=get_team(teamB);
    TeamState A,B; build_team_state(&A,rA); build_team_state(&B,rB);
    int scoreA=0, scoreB=0;

    int mins[EVENTS_PER_MATCH]={rand_in(1,7),rand_in(8,14),rand_in(15,21),rand_in(22,28),
        rand_in(29,35),rand_in(36,42),45,rand_in(50,56),rand_in(57,63),
        rand_in(64,70),rand_in(71,77),90};
    for(int i=1;i<EVENTS_PER_MATCH;i++){ if(mins[i]<=mins[i-1]) mins[i]=mins[i-1]+1; if(mins[i]>90) mins[i]=90; }

    socklen_t alen = sizeof(*addr);

    for(int i=0;i<EVENTS_PER_MATCH;i++){
        char payload[MAX_LINE];

        if(mins[i]==45){
            snprintf(payload,sizeof(payload),"%s|[HT] 45' Halftime — %s %d-%d %s\n",topic,teamA,scoreA,scoreB,teamB);
        }else if(mins[i]==90){
            snprintf(payload,sizeof(payload),"%s|[FT] 90' Full time — %s %d-%d %s\n",topic,teamA,scoreA,scoreB,teamB);
        }else{
            int allow_sub=(mins[i]>55);
            int et=allow_sub?rand_in(0,2):rand_in(0,1);
            if(et==0){
                int team=rand_in(0,1); TeamState* T=(team==0?&A:&B);
                int s,a;
                if(pick_two_active(T,&s,&a)!=0){
                    snprintf(payload,sizeof(payload),"%s|[INFO] Tactical reset at %d'\n",topic,mins[i]);
                }else{
                    if(team==0) scoreA++; else scoreB++;
                    snprintf(payload,sizeof(payload),
                        "%s|[GOAL] %s scores (assist: %s) at %d' — %s %d-%d %s\n",
                        topic,T->name[s],T->name[a],mins[i],teamA,scoreA,scoreB,teamB);
                }
            }else if(et==1){
                int team=rand_in(0,1); TeamState* T=(team==0?&A:&B);
                int p=pick_active(T);
                if(p<0) snprintf(payload,sizeof(payload),"%s|[INFO] Foul but no card at %d'\n",topic,mins[i]);
                else apply_card(T,p,mins[i],payload,sizeof(payload),topic);
            }else{
                int team=rand_in(0,1); TeamState* T=(team==0?&A:&B);
                int out=pick_out_starter(T); int in=pick_in_bench(T);
                if(out<0||in<0)
                    snprintf(payload,sizeof(payload),"%s|[INFO] Substitution attempt failed at %d'\n",topic,mins[i]);
                else{
                    T->on_field[out]=0; T->on_field[in]=1;
                    snprintf(payload,sizeof(payload),"%s|[SUB] %s replaced by %s at %d'\n",
                             topic,T->name[out],T->name[in],mins[i]);
                }
            }
        }

        // send via chosen protocol
        if(use_udp)
            send_reliable(payload, strlen(payload)); 
        else
            send(sock,payload,strlen(payload),0);

        printf("Sent: %s",payload);
        sleep_ms(1000);
    }
}