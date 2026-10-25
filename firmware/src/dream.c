/* SPDX-License-Identifier: GPL-3.0-only */
/* SCAPE: four editable, unequal phrases under one harmonic and activity director.
 * Main-loop composition only. The mixer shares the slow arc with the habitat. */
#define DR_LEN 64u
static const uint8_t DR_MOOD_SCL[6]={2,3,8,7,1,4};
static const uint8_t DR_PROG[4][4]={{0,5,6,0},{0,3,0,4},{0,6,5,4},{0,2,5,0}};
/* 1/8-note steps: bass 23 beats, bed 29, melody 13, pulse 17. */
static const uint8_t DR_LOOP[5]={64,46,58,26,34};
static struct {
    uint8_t valid,asked,root,scl,deg[4],role[NTRK],passes[NTRK],last[NTRK],chord,density[NTRK];
    uint32_t seed;
} dream;
static void dream_forget(void) { dream.valid=0; dream.asked=0; scape_music.active=0; }
static uint32_t dr_rnd(uint32_t n) { return n ? noise32(&dream.seed)%n : 0; }
static uint32_t dr_note(int32_t degree,int32_t oct)
{
    uint8_t pc[12]; uint32_t i,n=0,mask=SCALE_MASK[dream.scl%NELEM(SCALE_MASK)]; int32_t o;
    for(i=0;i<12;i++) if((mask>>i)&1u) pc[n++]=(uint8_t)i;
    if(!n) return 60;
    o=degree>=0?degree/(int32_t)n:-(((int32_t)n-1-degree)/(int32_t)n);
    return (uint32_t)clamp(12*(oct+1+o)+dream.root+pc[degree-o*(int32_t)n],24,100);
}
static void dr_put(track_t *t,uint32_t at,int32_t degree,uint32_t oct,uint32_t voices,uint32_t hold,uint32_t chance)
{
    uint32_t i; step_t *s=&t->step[at]; step_clear(s);
    s->time=ST_NOTE; s->vel=(uint8_t)(64u+dr_rnd(20)); s->n=(uint8_t)voices; step_set_chance(s,chance);
    for(i=0;i<voices;i++)s->note[i]=(uint8_t)dr_note(degree+(int32_t)(2*i),(int32_t)oct);
    for(i=1;i<hold && at+i<(uint32_t)t->p[P_SLEN];i++) {step_clear(&t->step[at+i]);t->step[at+i].time=ST_TIE;}
}
static void dr_roles(void)
{
    uint32_t i; for(i=0;i<NTRK;i++)scape_music.role[i]=dream.role[i]=(uint8_t)scm_role(i);
}
static void dr_phrase(uint32_t ti,int evolve)
{
    track_t *t=&trk[ti]; uint32_t i,r=dream.role[ti],len=DR_LOOP[r];
    uint32_t dens=(uint32_t)clamp(song.g[G_DDENS],1,8),spacing=dens>=6?4u:dens>=3?8u:16u;
    int32_t degree=dream.deg[dream.chord];
    step_t opening[4];
    if(evolve)memcpy(opening,t->step,sizeof opening);
    for(i=0;i<NSTEP;i++)step_clear(&t->step[i]);
    t->p[P_SLEN]=(int16_t)len;
    if(r==DR_DRONE) dr_put(t,0,degree,3,1,len,100);
    else if(r==DR_PAD) {
        dr_put(t,0,degree,3,3,len-8u,100); /* a deliberate breathing gap */
    } else if(r==DR_BASS) {
        dr_put(t,0,degree,2,1,12,100);
        if(dens>=3)dr_put(t,24,degree+4,2,1,8,100); /* root and fifth, then space */
        if(dens>=6)dr_put(t,38,degree,2,1,4,85);
    } else if(r==DR_PERC) {
        uint32_t pulses=1u+dens, rotation=(uint32_t)dr_rnd(3);
        for(i=0;i<len;i++) if(i==0 || (((i+rotation)*pulses)%len<pulses)) {
            if(t->eng_req%NENGINES==ENGI_DRUM) {
                grid_hit(t,i,i?DV_TOM:DV_KICK,1); t->step[i].vel=(uint8_t)(i?60+dr_rnd(16):88);
                step_set_chance(&t->step[i],100);
            } else dr_put(t,i,degree,3,1,1,100);
        }
    } else {
        static const int8_t MOTIF[4]={0,2,4,1};
        for(i=0;i+3u<len;i+=spacing) {
            int32_t d=MOTIF[(i/spacing)%4u];
            if(i && evolve)d+=(int32_t)dr_rnd(3)-1;
            dr_put(t,i,degree+d,4,1,2,100); /* rests are real, so calls can answer */
        }
    }
    if(evolve)memcpy(t->step,opening,sizeof opening);
    dream.density[ti]=(uint8_t)dens;
}
static void dr_arm(void)
{
    uint32_t i; scape_music.elapsed=scape_music.phase=0; scape_music.chord_pc=(uint8_t)(dr_note(dream.deg[dream.chord],3)%12u);
    for(i=0;i<NTRK;i++)scape_music.gain[i]=32768;
    scape_music.suspended=0;scape_music.active=1;
}
static void dream_compose(uint32_t root)
{
    uint32_t i; dream.root=(uint8_t)(root%12u);dream.scl=DR_MOOD_SCL[clamp(song.g[G_DMOOD],0,5)];
    dream.seed=rng()|1u; dream.chord=0;
    memcpy(dream.deg,DR_PROG[dr_rnd(NELEM(DR_PROG))],sizeof dream.deg);dr_roles();
    for(i=0;i<NTRK;i++) {
        track_t *t=&trk[i]; motion_reset(t);t->p[P_SDIV]=1;t->p[P_ROOT]=dream.root;t->p[P_SCALE]=dream.scl;
        t->p[P_CHRD]=t->p[P_AMODE]=0;
        if(t->eng_req%NENGINES==ENGI_GURDY)t->p[P_E0]=dream.root;
        dr_phrase(i,0);dream.last[i]=dream.passes[i]=0;
    }
    dream.valid=1;song.g[G_DREAM]=1;dr_arm();
    if(song.playing)transport_req=3;else if(!chain_busy())transport_req=1;
}
static void dr_rebuild(void)
{
    dream.root=(uint8_t)(trk[0].p[P_ROOT]%12);dream.scl=(uint8_t)trk[0].p[P_SCALE];
    dream.seed=0x53434150u;dream.chord=0;memcpy(dream.deg,DR_PROG[0],sizeof dream.deg);dr_roles();
    memset(dream.last,0,sizeof dream.last);memset(dream.passes,0,sizeof dream.passes);
    dream.valid=1;dr_arm();
}
static void dream_evolve(void)
{
    uint32_t ti;
    if(!dream.valid)dr_rebuild();ti=dr_rnd(NTRK);
    if(dream.role[ti]!=DR_DRONE)dr_phrase(ti,1);
}
static void dr_chord(void)
{
    uint32_t t,i,j,k,n=0;uint8_t pc[12];
    int32_t old=dream.deg[dream.chord],delta;
    dream.chord=(uint8_t)((dream.chord+1u)%4u);delta=(int32_t)dream.deg[dream.chord]-old;
    for(i=0;i<12;i++)if((SCALE_MASK[dream.scl%NELEM(SCALE_MASK)]>>i)&1u)pc[n++]=(uint8_t)i;
    if(!n)return;
    for(t=0;t<NTRK;t++)for(i=0;i<NSTEP;i++)for(j=0;j<trk[t].step[i].n;j++) {
        int32_t note=(int32_t)trk[t].step[i].note[j]-dream.root,oct=note/12-1;
        for(k=0;k<n && pc[k]!=(uint32_t)(note%12);k++);
        if(k<n)trk[t].step[i].note[j]=(uint8_t)dr_note((int32_t)k+delta,oct);
    }
    scape_music.chord_pc=(uint8_t)(dr_note(dream.deg[dream.chord],3)%12u);scape_music.elapsed=0;
}
static void dream_go(uint32_t root)
{
    char b[16];if(chain_busy()){ui_message("STOP THE SONG FIRST");return;}
    dream_compose(root);str_cpy(b,N_GURDY_DRN[root%12],4);str_cpy(b+str_len(b)," ",2);
    str_cpy(b+str_len(b),N_DMOOD[clamp(song.g[G_DMOOD],0,5)],6);ui_say("SCAPE IN ",b);
}
static void dream_keys(uint32_t notes)
{
    uint32_t k,i,j,any=0;for(k=0;k<27 && !((notes>>k)&1u);k++);if(k==27)return;
    for(i=0;i<NTRK && !any;i++)for(j=0;j<NSTEP && !any;j++)any=trk[i].step[j].n||trk[i].step[j].hit;
    if(any && !dream.asked && !chain_busy())confirm_open(CF_DREAM,(53+k)%12);else dream_go((53+k)%12);
}
static void dream_tick(void)
{
    static const uint16_t SECONDS[8]={20,30,45,60,90,120,180,300};
    uint32_t i,every=9u-(uint32_t)clamp(song.g[G_DDRFT],1,8);
    scape_music.suspended=(uint8_t)chain_busy();
    if(!song.g[G_DREAM] || !song.playing || scape_music.suspended)return;
    if(!dream.valid)dr_rebuild();
    if(scape_music.elapsed >= FS*(uint32_t)SECONDS[clamp(song.g[G_SCHORD],0,7)])dr_chord();
    for(i=0;i<NTRK;i++) {
        uint32_t idx=trk[i].seq_idx;
        if(idx<dream.last[i]) {
            uint32_t role=scm_role(i);
            if(role!=dream.role[i]) {scape_music.role[i]=dream.role[i]=(uint8_t)role;dr_phrase(i,0);}
            else if(dream.density[i]!=(uint32_t)song.g[G_DDENS])dr_phrase(i,0);
            else if(++dream.passes[i]>=every) {
                dream.passes[i]=0;
                if(dream.role[i]!=DR_DRONE && scm_activity()>60u && dr_rnd(3u)==0u)dr_phrase(i,1);
            }
        }
        dream.last[i]=(uint8_t)idx;
    }
}
