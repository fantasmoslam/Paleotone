/* SPDX-License-Identifier: GPL-3.0-only */
/* Include the adapter to check actual pattern/voice/effect state at its public actions. */
#include "../native/engine.c"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static float out[512*2];
static step_t patterns[NTRK][NSTEP];
static double render(unsigned blocks){
    double energy=0;for(unsigned b=0;b<blocks;++b){pt_render(out,512,48000);for(unsigned i=0;i<1024;++i){assert(isfinite(out[i]));energy+=fabs(out[i]);}}return energy;
}
int main(void){
    pt_boot(NULL,0);pt_scape(PT_SCAPE_STOP_ALL);
    song.g[G_AMB]=song.g[G_CRKL]=song.g[G_TAPE]=song.g[G_CRSH]=0; // exclude intentional fresh noise
    pt_scape(PT_ENV_START);assert(song.g[G_SCAPE]>0&&!pt_playing());
    for(unsigned i=0;i<SC_NL;++i)song.g[G_SL0+i]=0;
    song.g[G_SCTRK]=song.g[G_SDNGR]=0;
    pt_scape(PT_DRONE_START);assert(song.g[G_SDRN]>0);assert(render(100)>.01);
    pt_control_set(14,17);assert(song.g[G_SDRN]==17);render(20);
    pt_scape(PT_DRONE_STOP);assert(song.g[G_SDRN]==0);
    pt_scape(PT_DRONE_START);assert(song.g[G_SDRN]==17);
    pt_control_set(14,0);assert(song.g[G_SDRN]==0);rev_clear();hall_clear();render(30);
    double silent=render(20);assert(silent/(20*1024)<.00001); // below -100 dB after source zero
    pt_control_set(13,61);pt_scape(PT_ENV_STOP);assert(!song.g[G_SCAPE]&&!pt_playing());
    pt_scape(PT_ENV_START);assert(song.g[G_SCAPE]==61);
    for(unsigned i=0;i<SC_NL;++i)assert(song.g[G_SL0+i]==0); // start must preserve deliberate layer mutes
    for(unsigned i=0;i<NTRK;++i)memcpy(patterns[i],trk[i].step,sizeof patterns[i]);
    pt_scape(PT_MUSIC_START);render(10);assert(pt_playing()&&song.g[G_DREAM]);
    for(unsigned i=0;i<NTRK;++i)assert(!memcmp(patterns[i],trk[i].step,sizeof patterns[i]));
    pt_scape(PT_MUSIC_STOP);assert(!pt_playing()&&!song.g[G_DREAM]&&!scape_music.active&&song.g[G_SCAPE]==61);
    pt_scape(PT_MUSIC_START);render(10);pt_control_set(23,0);assert(!pt_playing()&&!song.g[G_DREAM]);
    /* Generation is explicit and must not start transport. */
    pt_scape(PT_SCAPE_GENERATE);assert(!pt_playing()&&dream.valid&&song.g[G_DREAM]);
    pt_scape(PT_DRONE_START);pt_scape(PT_MUSIC_START);render(30);
    pt_scape(PT_SCAPE_STOP_ALL);
    assert(!pt_playing()&&!song.g[G_DREAM]&&!song.g[G_SCAPE]&&!song.g[G_SDRN]);
    for(unsigned i=0;i<DLY_LEN;i++)assert(!dly_buf[i]);
    for(unsigned k=0;k<NPART;++k)for(unsigned i=0;i<NVOICE;++i)assert(!trk[k].v[i].active);
    assert(!tp.on);for(unsigned i=0;i<TP_LEN;i++)assert(!tp_buf[0][i]&&!tp_buf[1][i]);
    assert(render(20)/(20*1024)<.00001); // old effects/DMA/ring/cache audio must not return
    pt_scape(PT_MUSIC_START);render(10);assert(pt_playing()&&!song.g[G_SCAPE]&&!song.g[G_SDRN]);
    pt_scape(PT_MUSIC_STOP);
    for(unsigned i=0;i<NTRK;++i)memcpy(patterns[i],trk[i].step,sizeof patterns[i]);
    chain.armed=1;assert(pt_scape_busy());pt_scape(PT_SCAPE_GENERATE);
    for(unsigned i=0;i<NTRK;++i)assert(!memcmp(patterns[i],trk[i].step,sizeof patterns[i]));
    pt_scape(PT_SCAPE_STOP_ALL);assert(!pt_scape_busy());
    puts("PASS: drone zero/off/restore, environment pause/restore, music stop, legacy off, pattern preservation, explicit generation, Stop All clears voices/effects");
}
