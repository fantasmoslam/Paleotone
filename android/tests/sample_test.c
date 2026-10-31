/* SPDX-License-Identifier: GPL-3.0-only */
#include "../native/sample_layer.h"
#include <assert.h>
#include <stdio.h>
static float data[48000],out[512*2];
int main(void){
    for(unsigned i=0;i<48000;++i)data[i]=sinf(i*.04f);
    pt_sample_layer s={.data=data,.frames=48000,.rate=48000,.root=60};
    sample_note(&s,60,127,48000);sample_mix(&s,out,512,48000,1);assert(s.voice[0].step==1);
    double energy=0;for(unsigned i=0;i<512;++i){assert(isfinite(out[2*i]));assert(out[2*i]==out[2*i+1]);energy+=fabsf(out[2*i]);}assert(energy>1);
    sample_note(&s,60,0,48000);for(unsigned i=0;i<5;++i){memset(out,0,sizeof out);sample_mix(&s,out,512,48000,1);}assert(!s.voice[0].active);
    sample_note(&s,72,127,48000);assert(s.voice[0].step==2);sample_panic(&s);
    sample_note(&s,60,127,44100);assert(s.voice[0].step>1.08&&s.voice[0].step<1.09);sample_panic(&s);
    for(unsigned i=0;i<128;++i)sample_note(&s,i,127,48000);
    for(unsigned i=0;i<300;++i){memset(out,0,sizeof out);sample_mix(&s,out,512,48000,1);for(unsigned j=0;j<1024;++j)assert(isfinite(out[j]));}
    for(unsigned i=0;i<8;++i)assert(!s.voice[i].active);
    sample_note(&s,60,127,48000);sample_panic(&s);memset(out,0,sizeof out);sample_mix(&s,out,512,48000,1);for(unsigned i=0;i<1024;++i)assert(out[i]==0);
    puts("PASS: sample voices, stereo mix, attack/release, pitch/rate conversion, stealing, file end and panic");
}
