/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "../../firmware/src/dsp/taiko.c"
#include "../../firmware/src/dsp/taiko_rhythm.h"
static float l[256],r[256];
int main(int argc,char**argv){int i,b,j;char path[1024];unsigned count;static const uint32_t want[4]={0x2486e709u,0xf378165u,0x11f14bu,0x14638ed4u};for(i=0;i<4;i++){ta_voice_t v={0};FILE*f;v.sample=(uint8_t)i;snprintf(path,sizeof path,"%s/taiko-%d.pcm",argc>1?argv[1]:"build/host",i);f=fopen(path,"wb");if(!f)return 2;count=0;{uint32_t hash=2166136261u;while(v.pos<TA_BYTES){int16_t x=ta_decode(&v);uint16_t u=(uint16_t)x;hash=(hash^(u&255u))*16777619u;hash=(hash^(u>>8))*16777619u;fwrite(&x,2,1,f);count++;}fclose(f);if(count!=19695||hash!=want[i])return 3;}}taiko_quiet();for(i=0;i<90;i++){taiko_hit((unsigned)(i%4+1),127,100);for(b=0;b<10;b++){taiko_render(l,r,256,80);for(j=0;j<256;j++)if(!isfinite(l[j])||!isfinite(r[j]))return 4;}}taiko_quiet();taiko_render(l,r,256,80);for(j=0;j<256;j++)if(l[j]||r[j])return 5;for(i=0;i<(int)TAIKO_N_RHYTHMS;i++){unsigned st;double energy=0;taiko_quiet();for(st=0;st<128;st++){unsigned kind=taiko_rhythm_hit((unsigned)i,st,45,0);if(kind)taiko_hit(kind,kind==2?113:76,100);for(b=0;b<48;b++){taiko_render(l,r,256,80);for(j=0;j<256;j++){if(!isfinite(l[j])||!isfinite(r[j]))return 6;energy+=(double)l[j]*(double)l[j]+(double)r[j]*(double)r[j];}}}if(energy<.1)return 7;}puts("taiko reference hashes, 14 rhythms, overlap, finite output and panic passed");return 0;}
