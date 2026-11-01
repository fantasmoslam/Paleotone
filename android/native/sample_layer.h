/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <math.h>
#include <string.h>
/* A single shared sample, eight bounded voices, no allocation on the audio thread. */
typedef struct { double pos,step;float amp,velocity;unsigned active,gate,note; } pt_sample_voice;
typedef struct {
    const float *data;unsigned frames,rate,root,age;
    pt_sample_voice voice[8];
} pt_sample_layer;
static inline void sample_panic(pt_sample_layer *s){memset(s->voice,0,sizeof s->voice);}
static inline void sample_note(pt_sample_layer *s,unsigned note,unsigned velocity,unsigned rate){
    if(!velocity){for(unsigned i=0;i<8;++i)if(s->voice[i].note==note)s->voice[i].gate=0;return;}
    if(!s->data||s->frames<2)return;
    unsigned slot=8;for(unsigned i=0;i<8;++i)if(!s->voice[i].active){slot=i;break;}
    if(slot==8)slot=s->age++%8;
    s->voice[slot]=(pt_sample_voice){.pos=0,.step=pow(2.0,((int)note-(int)s->root)/12.0)*s->rate/rate,
        .amp=0,.velocity=velocity/127.0f,.active=1,.gate=1,.note=note};
}
static inline void sample_mix(pt_sample_layer *s,float *out,unsigned frames,unsigned rate,float gain){
    float attack=1.0f/(.005f*rate),release=1.0f/(.04f*rate);
    for(unsigned i=0;i<frames;++i){
        float sum=0;
        for(unsigned j=0;j<8;++j){pt_sample_voice *v=&s->voice[j];if(!v->active)continue;
            unsigned at=(unsigned)v->pos;
            if(at+1>=s->frames){v->active=0;continue;}
            float fraction=(float)(v->pos-at);
            v->amp+=v->gate?attack:-release;
            if(v->amp>1)v->amp=1;if(v->amp<=0){v->active=0;continue;}
            /* Fade the final 5 ms even for a one-shot held beyond the file end. */
            double remain=(s->frames-1-v->pos)/v->step;
            float tail=remain<rate*.005? (float)(remain/(rate*.005)):1;
            sum+=(s->data[at]+(s->data[at+1]-s->data[at])*fraction)*v->amp*v->velocity*tail*.35f*gain;
            v->pos+=v->step;
        }
        out[2*i]+=sum;out[2*i+1]+=sum;
    }
}
