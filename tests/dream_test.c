/* SPDX-License-Identifier: GPL-3.0-only */
/* SCAPE's compact music composer: real UI, editable patterns, safety, persistence and evolution. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static int piece_ok(void)
{
    uint32_t t, i, n, prev;
    for (t=0;t<NTRK;t++) {
        track_t *tr=&trk[t];
        if (tr->p[P_SLEN]!=DR_LOOP[dream.role[t]] || tr->p[P_SDIV]!=1 || tr->p[P_ROOT]!=dream.root ||
            tr->p[P_SCALE]!=dream.scl || tr->p[P_CHRD] || tr->p[P_AMODE]) return 0;
        prev=0;
        for(i=0;i<(uint32_t)tr->p[P_SLEN];i++) {
            step_t *s=&tr->step[i];
            if(s->time==ST_TIE && !prev) return 0;
            if(s->time==ST_REST) prev=0;
            if(s->time==ST_NOTE) {
                prev=1;
                if(tr->eng_req%NENGINES==ENGI_DRUM ? s->n || !s->hit : !s->n || s->hit) return 0;
                for(n=0;n<s->n;n++)
                    if(s->note[n]<24 || s->note[n]>100 ||
                       !(SCALE_MASK[dream.scl]>>(s->note[n]+120-dream.root)%12&1u)) return 0;
            }
        }
    }
    return 1;
}
static uint32_t note_count(void)
{
    uint32_t t,i,n=0;
    for(t=0;t<NTRK;t++)for(i=0;i<64;i++)n+=trk[t].step[i].time==ST_NOTE;
    return n;
}
static void scene(uint32_t s)
{
    ui_power_on();gb_scene_load(s);song.playing=0;transport_req=0;dream.asked=0;dream_forget();
}
int main(void)
{
    int bad=0,ok=1;
    uint32_t sc,m,d,k,i,t,lo=0,hi=0,changed=0;
    static step_t before[NTRK][NSTEP];
    scene(0);go_title("SCAPE MUSIC");
    ok=!ui.home && cur_page()->id[0]==G_DDENS && cur_page()->id[3]==G_SCHORD && G_SMOTION==69 && G_COUNT==82;
    bad+=check("SCAPE MUSIC owns the existing composition controls and stable stored global IDs",ok);
    go_title("SCAPE SCENE");
    ok=!ui.home && cur_page()->id[0]==G_SCHOLD && cur_page()->id[1]==G_SCNEXT &&
       cur_page()->id[2]==G_SCNEW && cur_page()->id[3]==G_SCHORD;
    bad+=check("SCAPE SCENE gives each performance action its own control",ok);
    /* HOLD, NEXT and NEW */
    song.playing=1;song.g[G_DREAM]=1;song.g[G_SCHOLD]=song.g[G_SCNEXT]=song.g[G_SCNEW]=0;dream_compose(2);
    scape_music.suspended=0;
    {uint8_t c0=dream.chord;song.g[G_SCNEXT]=1;dream_tick();
     ok=dream.chord==(uint8_t)((c0+1u)&3u) && !song.g[G_SCNEXT];}
    bad+=check("NEXT advances the harmony once and returns to --",ok);
    {uint32_t s0,root0=dream.root;dream.seed=0x01234567u;s0=dream.seed;song.g[G_SCNEW]=1;dream_tick();
     ok=dream.root==root0 && dream.seed!=s0 && !song.g[G_SCNEW] && piece_ok();}
    bad+=check("NEW starts a new piece in the same key",ok);
    {uint32_t e0;int32_t g0;song.g[G_SCHOLD]=1;e0=scape_music.elapsed;g0=scape_music.gain[0];
     scm_block(FS/2u);ok=scape_music.elapsed==e0 && scape_music.gain[0]==g0;
     song.g[G_SCHOLD]=0;scm_block(FS/2u);ok=ok && scape_music.elapsed!=e0;}
    bad+=check("HOLD keeps the state it reached, released it moves on again",ok);
    go_title("SCAPE MUSIC");
    for(sc=0,ok=1;sc<GB_NSCENE;sc++)
        for(m=0;m<6;m++)
            for(d=1;d<=8;d+=d==1?3:4)
                for(k=0;k<12;k++) {
                    scene(sc);rng_state=1+sc*97+m*19+d*7+k;
                    song.g[G_DMOOD]=(int16_t)m;song.g[G_DDENS]=(int16_t)d;
                    dream_compose(k);
                    ok&=dream.valid && !song.playing && !transport_req && piece_ok();
                }
    bad+=check("every scene, mood, density and root: ordinary unequal-length patterns, in key, valid ties/drums",ok);
    for(k=0;k<8;k++) {
        scene(2);song.g[G_DDENS]=1;dream_compose(2);lo+=note_count();
        scene(2);song.g[G_DDENS]=8;dream_compose(2);hi+=note_count();
    }
    bad+=check("density increases musical activity",hi>lo);
    scene(0);go_title("SCAPE MUSIC");frame();
    tap_key(9);ok=ui.confirm==CF_DREAM && !dream.valid;
    press(B_OCTDN);ok&=!ui.confirm && !dream.valid;
    tap_key(9);press(B_OCTUP);ok&=dream.valid && dream.asked && dream.root==2 && piece_ok() && !song.playing && !transport_req;
    tap_key(11);ok&=!ui.confirm && dream.root==4 && !song.playing && !transport_req;
    bad+=check("key composition confirms replacement and leaves stopped playback stopped",ok);
    scene(0);dream_compose(2);song.playing=0;transport_req=0;song.g[G_SCNEW]=1;dream_tick();
    bad+=check("SCENE NEW prepares another piece without starting playback",!song.playing && !transport_req && !song.g[G_SCNEW] && piece_ok());
    for(i=0,ok=1;i<2;i++)for(k=0;k<4;k++){
        song.playing=(uint8_t)i;transport_req=(uint8_t)k;song.tick=12345;
        for(t=0;t<NTRK;t++)trk[t].seq_idx=7;
        dream_compose(5);
        ok&=song.playing==i && transport_req==k && song.tick==12345;
        for(t=0;t<NTRK;t++)ok&=trk[t].seq_idx==7;
    }
    bad+=check("composing preserves running/stopped transport, pending commands and song position",ok);
    scene(2);dream_compose(2);song.g[G_DREAM]=1;song.g[G_DDRFT]=8;song.playing=1;
    for(k=0,ok=1;k<80;k++) {
        for(t=0;t<NTRK;t++)memcpy(before[t],trk[t].step,sizeof before[t]);
        for(t=0;t<NTRK;t++)trk[t].seq_idx=20;dream_tick();for(t=0;t<NTRK;t++)trk[t].seq_idx=0;dream_tick();
        for(t=0;t<NTRK;t++) {
            changed+=memcmp(before[t],trk[t].step,sizeof before[t])!=0;
            ok&=!memcmp(before[t],trk[t].step,4*sizeof(step_t));
            if(dream.role[t]==DR_DRONE)ok&=!memcmp(before[t],trk[t].step,sizeof before[t]);
        }
        ok&=piece_ok();
    }
    bad+=check("evolution changes later phrases, preserves opening motifs and held creatures, stays in key",ok && changed>10);
    for(t=0;t<NTRK;t++)memcpy(before[t],trk[t].step,sizeof before[t]);
    for(i=0,ok=1;i<2;i++) {
        song.g[G_DREAM]=(int16_t)i;song.playing=!i;
        for(k=0;k<30;k++){trk[0].seq_idx=40;dream_tick();trk[0].seq_idx=0;dream_tick();}
        for(t=0;t<NTRK;t++)ok&=!memcmp(before[t],trk[t].step,sizeof before[t]);
    }
    bad+=check("no evolution while disabled or transport stopped",ok);
    {
        project_t q;project_store_t st;
        scene(1);song.g[G_DMOOD]=2;dream_compose(9);song.g[G_DREAM]=1;
        song.g[G_DDENS]=8;song.g[G_DDRFT]=1;
        project_capture(&q);
        ok=proj_pack(&st,&q)==1 && proj_import(&q,&st,sizeof st) && q.g[G_DREAM]==1 &&
           q.g[G_DDENS]==8 && q.g[G_DDRFT]==1 && q.g[G_DMOOD]==2;
        scene(0);dream_compose(2);project_restore_runtime(&q);
        ok&=!dream.valid;dr_rebuild();
        ok&=dream.root==9 && dream.scl==DR_MOOD_SCL[2];
        st.raw[PROJ_DREAM_OFF]=st.raw[PROJ_DREAM_OFF+1]=0;
        k=proj_hash(st.raw,PROJ_STORE_SIZE-4);memcpy(st.raw+PROJ_STORE_SIZE-4,&k,4);
        ok&=proj_import(&q,&st,sizeof st) && !q.g[G_DREAM] && q.g[G_DDENS]==4 && q.g[G_DDRFT]==6;
        bad+=check("settings/project round trip; loaded key replaces cached key; legacy projects default safely",ok);
    }
    alarm(60);
    for(sc=0,ok=1;sc<NELEM(SCALE_MASK);sc++) {
        scene(0);for(t=0;t<NTRK;t++)trk[t].p[P_SCALE]=(int16_t)sc;
        dream_forget();dr_rebuild();rng_state=0;
        for(t=0;t<NTRK;t++)memcpy(before[t],trk[t].step,sizeof before[t]);
        for(k=0;k<40;k++)dream_evolve();
        for(t=0;t<NTRK;t++)for(i=16;i<64;i++)
            if(memcmp(&before[t][i],&trk[t].step[i],sizeof(step_t)) && trk[t].step[i].time==ST_NOTE)for(d=0;d<trk[t].step[i].n;d++)
                ok&=(SCALE_MASK[sc]>>(trk[t].step[i].note[d]+120-dream.root)%12&1u)!=0;
    }
    alarm(0);
    bad+=check("all stored scales and a stuck random source: bounded generation, no hang",ok);
    printf(bad ? "scape music: %d FAILED\n" : "scape music: all ok\n",bad);
    return bad!=0;
}
