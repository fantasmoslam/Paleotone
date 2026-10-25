/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
static void check_key(void)
{
    uint32_t t,i,n;
    for(t=0;t<NTRK;t++)for(i=0;i<NSTEP;i++)for(n=0;n<trk[t].step[i].n;n++)
        assert((SCALE_MASK[dream.scl]>>((trk[t].step[i].note[n]+120-dream.root)%12))&1u);
}
int main(void)
{
    uint32_t i,t,old,peak=0;int32_t out[CTL*2];int16_t keep[10];
    static step_t patterns[NTRK][NSTEP];
    web_nor_erase();web_boot();gb_scene_load(0);
    assert(G_SDROR==68 && G_SMOTION==69 && G_COUNT==79);
    rng_state=42;dream_compose(2);
    for(t=0;t<NTRK;t++)memcpy(patterns[t],trk[t].step,sizeof patterns[t]);
    rng_state=42;dream_compose(2);
    for(t=0;t<NTRK;t++)assert(!memcmp(patterns[t],trk[t].step,sizeof patterns[t]));
    assert(trk[0].p[P_SLEN]==46 && trk[1].p[P_SLEN]==58 && trk[2].p[P_SLEN]==26 && trk[3].p[P_SLEN]==34);
    assert(sd.role[0]==DR_BASS && sd.role[1]==DR_PAD && sd.role[2]==DR_MELODY && sd.role[3]==DR_PERC);
    song.playing=1;transport_req=0;check_key();
    old=sd.chord_pc;sd.elapsed=FS*60u;dream_tick();
    assert(dream.chord==1 && sd.elapsed==0 && old!=sd.chord_pc);check_key();
    /* Chord changes transpose the motif, preserving rhythm and velocities. */
    for(t=0;t<NTRK;t++)for(i=0;i<NSTEP;i++)assert(patterns[t][i].time==trk[t].step[i].time && patterns[t][i].vel==trk[t].step[i].vel);
    for(i=0;i<48;i++){dr_chord();check_key();}
    song.g[G_SCAPE]=80;song.g[G_SCTRK]=1;song.g[G_SCRATE]=127;sc.cwait=0;sc.cnote=0;sc.nlen=0;
    trk[2].seq_idx=0;sc_calls();assert(!sc.cnote);
    trk[2].seq_idx=3;sc_calls();assert(sc.cnote && sc.cnote%12u==sd.chord_pc);
    trk_note_off(&trk[sc.ctrk],sc.cnote);sc.cnote=0;
    song.g[G_SMOTION]=0;for(i=G_SBED;i<=G_SPULSE;i++)song.g[i]=127;
    for(i=0;i<512;i++)sd_block(CTL);
    for(i=0;i<NTRK;i++)assert(sd.gain[i]==32768);
    song.g[G_SMELO]=0;for(i=0;i<512;i++)sd_block(CTL);assert(sd.gain[2]==0);
    old=sd.elapsed;song.g[G_DREAM]=0;for(i=0;i<30;i++){sd_block(CTL);dream_tick();}assert(sd.elapsed==old);
    song.g[G_DREAM]=1;chain.armed=1;dream_tick();sd_block(CTL);assert(sd.elapsed==old);chain.armed=0;dream_tick();
    /* Append-only, byte-bounded settings; the older 33 bytes remain untouched. */
    for(i=0;i<10;i++)song.g[G_SMOTION+i]=GP[G_SMOTION+i].max;
    memcpy(keep,&song.g[G_SMOTION],sizeof keep);scape_side_put(0);
    for(i=0;i<10;i++)song.g[G_SMOTION+i]=0;
    scape_side_get(0);assert(!memcmp(keep,&song.g[G_SMOTION],sizeof keep));
    memset(scape_side[0]+SCAPE_DIR_OFF,0,10);scape_side_get(0);
    for(i=0;i<10;i++)assert(song.g[G_SMOTION+i]==GP[G_SMOTION+i].def);
    /* Full mixer, evolution and shared habitat: 35 seconds within the existing voice budget. */
    gb_scene_load(0);dream_compose(9);song.g[G_SCAPE]=80;song.g[G_SDRN]=60;song.g[G_SCTRK]=1;song.g[G_SCRATE]=127;
    song.g[G_SCHORD]=0;seq_start();
    for(t=0;t<FS*35u;t+=CTL) {
        mix_block(out,CTL);dream_tick();
        assert(voices_busy()<=NVOICE);
        for(i=0;i<CTL*2;i++){uint32_t a=out[i]<0?-out[i]:out[i];assert(a<=32768);if(a>peak)peak=a;}
    }
    assert(peak>100 && dream.chord>=1);dream_forget();assert(!sd.active);
    puts("SCAPE director: deterministic unequal loops, shared harmony/rests, smooth level controls, freeze/chain guard, old/new sidecars and 35-second bounded mix passed");
    return 0;
}
