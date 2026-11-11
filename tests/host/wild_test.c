/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <math.h>
#include "amb.h"
#include "../../firmware/src/pt/pt.h"
void en_midi_out(uint32_t s,uint32_t a,uint32_t b){(void)s;(void)a;(void)b;}
static int fail;
#define CHECK(x) do { if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);fail++;}}while(0)
static float raw[256];
static int32_t pcm[512];
static void render_seconds(int sec) {
    int b,j;
    for(b=0;b<sec*44100/256;b++) {
        en_render(pcm,256,2048);
        for(j=0;j<512;j++) CHECK(pcm[j]>=-8388608 && pcm[j]<=8388607);
    }
}
int main(void) {
    int k,b,j;
    pt_init();pt_skin_quiet();
    for(k=0;k<3;k++) {
        double e=0;
        pt_skin_quiet();pt_skin_hit(k,100,45,45);
        for(b=0;b<400;b++){pt_skin_render(raw,256);for(j=0;j<256;j++){CHECK(isfinite(raw[j]));e+=(double)raw[j]*(double)raw[j];}}
        printf("skin %d energy %.5f\n",k,e);CHECK(e>0.01);
    }
    for(k=0;k<PT_NCALL;k++) {
        double e=0;
        pt_sound(PT_CALLS,k);CHECK(pt_is(PT_CALLS)==k+1);
        pt_note_on(PT_CALLS,48,100,44100);
        for(b=0;b<400;b++){pt_render(PT_CALLS,raw,256);for(j=0;j<256;j++){CHECK(isfinite(raw[j]));e+=(double)raw[j]*(double)raw[j];}}
        printf("call %s energy %.5f\n",pt_name(PT_CALLS,k),e);CHECK(e>0.01);
        pt_quiet(PT_CALLS);CHECK(!pt_render(PT_CALLS,raw,256));
    }
    en_init();en_set(P_TEMPO,120);en_set(P_CHORD,0);en_set(P_SEQ,0);en_set(P_SEQ2,0);en_set(P_LEAD,0);en_set(P_CRACKLE,0);
    en_set(P_SKLEVEL,70);en_set(P_SKDENS,100);en_set(P_CLLEVEL,60);en_set(P_CLCHANCE,100);en_play(1);
    /* No automatic calls until explicitly enabled. Manual calls also work stopped. */
    render_seconds(8);CHECK(en_call_hits==0);CHECK(en_call_auto==0);
    en_play(0);en_call(2);render_seconds(1);CHECK(en_call_hits==1);
    en_call(-1);render_seconds(1);CHECK(pt_voices()==0);
    en_calls_auto(1);en_play(1);
    render_seconds(64);
    printf("scheduler: %u skins, %u calls in %u sixteenths\n",en_skin_hits,en_call_hits,en_bars);
    CHECK(en_skin_hits>20 && en_skin_hits<80);CHECK(en_call_hits>2 && en_call_hits<10);CHECK(pt_voices()<=8);
    {uint32_t h=en_skin_hits,c=en_call_hits;en_freeze(1);render_seconds(8);CHECK(h==en_skin_hits && c==en_call_hits);}
    en_freeze(0);en_set(P_SKLEVEL,0);en_set(P_CLLEVEL,0);
    {uint32_t h=en_skin_hits,c=en_call_hits;render_seconds(8);CHECK(h==en_skin_hits && c==en_call_hits);}
    en_panic();render_seconds(1);CHECK(pt_voices()==0);
    en_calls_auto(0);render_seconds(1);CHECK(en_call_auto==0);
    {uint32_t c=en_call_hits;en_set(P_CLLEVEL,60);render_seconds(16);CHECK(c==en_call_hits);}
    for(k=0;k<PT_NCALL;k++) {
        uint32_t c=en_call_hits;
        en_call(k);render_seconds(1);CHECK(en_call_hits==c+1);CHECK(pt_voices()<=8);
    }
    en_call(-1);render_seconds(1);CHECK(pt_voices()==0);
    {uint32_t c=en_call_hits;
     en_set(P_CLLEVEL,0);en_call(0);render_seconds(1);CHECK(en_call_hits==c);
     en_set(P_CLLEVEL,60);en_mute(EN_M_WILD);en_call(0);render_seconds(1);CHECK(en_call_hits==c);
     en_mute(0);render_seconds(1);CHECK(pt_voices()==0);}
    for(k=0;k<EN_NPIECES;k++) {
        int16_t p[P_NPARAMS];uint16_t pool;double energy=0,pk=0;int frames=30*44100/256;
        en_init();en_piece(k,p,&pool);
        for(j=0;j<P_NPARAMS;j++){CHECK(p[j]>=en_param_info(j)->lo && p[j]<=en_param_info(j)->hi);en_set(j,p[j]);}
        en_pool(pool);en_piece_theme(k);en_regrow();en_play(1);
        for(b=0;b<frames;b++){
            en_render(pcm,256,2048);
            for(j=0;j<512;j++){double x=(double)pcm[j]/8388608.0;energy+=x*x;if(fabs(x)>pk)pk=fabs(x);}
        }
        energy=sqrt(energy/(frames*512.0));
        printf("preset %-16s rms %.4f peak %.3f\n",en_piece_name(k),energy,pk);
        CHECK(energy>0.01 && pk<0.95);CHECK(pt_voices()<=8);
        if(k==1) {printf("stampede: %u drum hits in %u sixteenths\n",en_skin_hits,en_bars);CHECK(en_skin_hits>=45 && en_skin_hits<90);}
    }
    printf("wild: %s\n",fail?"FAIL":"all passed");return fail!=0;
}
