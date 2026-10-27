/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare real mixed audio and RNG evolution with coefficient reuse and with
 * keys invalidated before every block (the original recalculation behavior).
 * Each child starts from the same untouched process image. No production
 * bypass or alternative coefficient equations are introduced for the test. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <sys/wait.h>

static void hash_word(uint64_t *h, uint32_t x)
{
    uint32_t i;
    for (i=0;i<4u;i++) { *h^=(x>>(8u*i))&255u; *h*=1099511628211ull; }
}
static void release_all(void)
{
    uint32_t t,n;
    for(t=0;t<NTRK;t++) for(n=0;n<128u;n++) trk_note_off(&trk[t],n);
}
static void render_scenes(uint64_t hashes[SC_COUNT], int uncached)
{
    uint32_t sc,t,b,i,k,j;
    int32_t out[2u*CTL];
    for(sc=0;sc<SC_COUNT;sc++) {
        uint64_t h=14695981039346656037ull;
        memset(trk,0,sizeof trk);
        memset(saur_slot,0,sizeof saur_slot);
        saur_age=0;
        host_tracks_init();
        song.master_q12=1800;
        for(t=0;t<NTRK;t++) {
            host_preset(&trk[t],ENGI_SAURUS,t%ENG_SAURUS.npresets);
            trk[t].p[P_E0]=(int16_t)sc;
            trk[t].p[P_LEVEL]=65;
            trk[t].p[P_VOICE]=V_POLY;
            trk[t].p[P_ATK]=0;
            trk[t].p[P_SUS]=100;
            trk[t].p[P_REL]=50;
            trk_note_on(&trk[t],48u+t*5u,40u+t*25u);
        }
        for(b=0;b<2048u;b++) {
            if(b==96u) for(t=0;t<NTRK;t++)
                trk_note_on(&trk[t],48u+t*5u,110u); /* same-key retriggers */
            if(b==192u) for(t=0;t<NTRK;t++) {
                trk_note_on(&trk[t],55u+t*5u,95u);
                trk_note_on(&trk[t],60u+t*5u,80u); /* exceed the four throat slots */
            }
            if(b==320u) for(t=0;t<NTRK;t++) trk[t].p[P_E0]=(int16_t)((sc+7u)%SC_COUNT);
            if(b==448u || b==576u) for(t=0;t<NTRK;t++) {
                trk[t].p[P_E1]=b==448u?0:127;
                trk[t].p[P_E6]=b==448u?127:0;
            }
            if(b>=704u && b<832u) for(t=0;t<NTRK;t++)
                trk[t].p[P_E6]=(int16_t)((b*13u+t*19u)&127u);
            if(b==832u) for(t=0;t<NTRK;t++)
                trk_note_on(&trk[t],24u+t*17u,127u); /* wide key tracking */
            if(b==960u) for(t=0;t<NTRK;t++) {
                trk[t].p[P_E0]=(int16_t)sc;
                trk[t].p[P_E1]=64;
                trk[t].p[P_E6]=64;
            }
            if(b==1088u || b==1408u) release_all();
            if(b==1216u) for(t=0;t<NTRK;t++)
                trk_note_on(&trk[t],60u+t,64u); /* reuse after release/stealing */
            if(uncached) for(k=0;k<SAUR_SLOTS;k++)
                for(i=0;i<SAUR_NL;i++) for(j=0;j<3u;j++)
                    saur_slot[k].l[i].res_f[j]=0; /* impossible real frequency */
            mix_block(out,CTL);
            for(i=0;i<2u*CTL;i++) hash_word(&h,(uint32_t)out[i]);
            hash_word(&h,(uint32_t)formant_nz);
        }
        hashes[sc]=h;
    }
}
static int capture(int uncached,uint64_t hashes[SC_COUNT])
{
    int fd[2],status;
    pid_t pid;
    size_t got=0,bytes=sizeof(uint64_t)*SC_COUNT;
    if(pipe(fd)) return 1;
    pid=fork();
    if(pid<0) { close(fd[0]); close(fd[1]); return 1; }
    if(!pid) {
        uint64_t result[SC_COUNT];
        ssize_t written;
        close(fd[0]);
        render_scenes(result,uncached);
        written=write(fd[1],result,bytes);
        close(fd[1]);
        _exit(written==(ssize_t)bytes?0:2);
    }
    close(fd[1]);
    while(got<bytes) {
        ssize_t n=read(fd[0],(uint8_t *)hashes+got,bytes-got);
        if(n<=0) break;
        got+=(size_t)n;
    }
    close(fd[0]);
    if(waitpid(pid,&status,0)!=pid) return 1;
    return got!=bytes || !WIFEXITED(status) || WEXITSTATUS(status);
}
int main(void)
{
    uint64_t cached[SC_COUNT],fresh[SC_COUNT];
    uint32_t sc,bad=0;
    if(capture(0,cached)||capture(1,fresh)) return 2;
    for(sc=0;sc<SC_COUNT;sc++) {
        int same=cached[sc]==fresh[sc];
        printf("saurus cache: %-8s audio/RNG %s\n",N_SAUR_CALL[sc],same?"identical":"DIFF");
        bad+=!same;
    }
    printf("saurus cache: %u of %u lifecycle scenes differ\n",bad,(unsigned)SC_COUNT);
    return bad?1:0;
}
