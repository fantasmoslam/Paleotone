/* SPDX-License-Identifier: GPL-3.0-only */
/* Compact mono IMA ADPCM taiko player; included by amb.c. No heap or external storage.
 * Four 256-byte-block recordings, 16 kHz; six independent players, linear interpolation to 44.1 kHz.
 * Asset attribution/license: taiko_data.inc and tools/research/taiko-bank-05/ATTRIBUTION.md. */
#include <stdint.h>
#include "fastmath.h"
#include "taiko_data.inc"
#define TA_VOICES 6
#define TA_BYTES 9984u
static const int16_t ta_step[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const int8_t ta_index[8]={-1,-1,-1,-1,2,4,6,8};
typedef struct {uint32_t pos,frac,step,target,age,limit;int32_t pred,index;int16_t a,b;uint8_t sample,half,active;float gl,gr,lastl,lastr,fade_l,fade_r;uint16_t fade;} ta_voice_t;
static ta_voice_t ta_voice[TA_VOICES];
static uint32_t ta_serial,ta_rr[2];
static float ta_lp_l,ta_lp_r,ta_k;
static int ta_tone=-1;
static int16_t ta_decode(ta_voice_t*v){
 const uint8_t*d=taiko_data[v->sample];int32_t q,step,delta;
 if(v->pos>=TA_BYTES)return 0;
 if(!(v->pos&255u)){v->pred=(int16_t)((uint16_t)d[v->pos]|(uint16_t)d[v->pos+1]<<8);v->index=d[v->pos+2];if(v->index>88)v->index=88;v->pos+=4;v->half=0;return (int16_t)v->pred;}
 q=d[v->pos];if(v->half){q>>=4;v->pos++;v->half=0;}else{q&=15;v->half=1;}
 step=ta_step[v->index];delta=step>>3;if(q&1)delta+=step>>2;if(q&2)delta+=step>>1;if(q&4)delta+=step;
 v->pred+=(q&8)?-delta:delta;if(v->pred>32767)v->pred=32767;if(v->pred< -32768)v->pred=-32768;
 v->index+=ta_index[q&7];if(v->index<0)v->index=0;if(v->index>88)v->index=88;
 return (int16_t)v->pred;
}
static void taiko_quiet(void){int i;for(i=0;i<TA_VOICES;i++)ta_voice[i]=(ta_voice_t){0};ta_lp_l=ta_lp_r=0.0f;ta_serial=ta_rr[0]=ta_rr[1]=0;ta_tone=-1;}
static void taiko_hit(unsigned kind,int velocity,int decay){
 int i,best=-1;uint32_t oldest=~0u;float fl=0,fr=0;ta_voice_t*v;unsigned family=kind==2?0u:1u;
 for(i=0;i<TA_VOICES;i++){if(!ta_voice[i].active){best=i;break;}if(ta_voice[i].age<oldest){oldest=ta_voice[i].age;best=i;}}
 if(best<0)return;
 v=&ta_voice[best];if(v->active){fl=v->lastl;fr=v->lastr;}
 *v=(ta_voice_t){0};v->sample=(uint8_t)(2u*family+(ta_rr[family]++&1u));v->active=1;v->age=++ta_serial;
 v->target=kind==3?33525u:family?27107u:22351u;v->step=kind==4?v->target*3u/4u:v->target;
 if(velocity<1)velocity=1;
 if(velocity>127)velocity=127;
 if(decay<0)decay=0;
 if(decay>100)decay=100;
 {float vel=(float)velocity*(1.0f/127.0f),g=fm_powf(vel,1.35f),pan=family?.25f:-.26f;
 v->gl=g*fm_sqrtf(.5f*(1.0f-pan));v->gr=g*fm_sqrtf(.5f*(1.0f+pan));}
 v->limit=(uint32_t)((19695u*(uint32_t)(40+decay*6/10))/100u);
 v->a=ta_decode(v);v->b=ta_decode(v);v->fade_l=fl;v->fade_r=fr;v->fade=(fl!=0.0f||fr!=0.0f)?96u:0u;
}
static int taiko_render(float*l,float*r,uint32_t n,int tone){uint32_t j;int i,any=0;
 if(tone!=ta_tone){float cut=1200.0f+60.0f*(float)(tone<0?0:tone>100?100:tone);ta_k=1.0f-fm_exp2f(-9.06472f*cut/44100.0f);ta_tone=tone;}
 for(j=0;j<n;j++){float sl=0,sr=0;for(i=0;i<TA_VOICES;i++){ta_voice_t*v=&ta_voice[i];if(v->active){
 float y=((float)v->a+((float)v->b-(float)v->a)*(float)v->frac*(1.0f/65536.0f))*(1.0f/32768.0f);
 uint32_t source=(v->pos/256u)*505u+((v->pos&255u)>4u?((v->pos&255u)-4u)*2u+v->half:0u);
 if(source+1600u>v->limit){float e=source<v->limit?(float)(v->limit-source)*(1.0f/1600.0f):0.0f;y*=e;}
 v->lastl=y*v->gl;v->lastr=y*v->gr;sl+=v->lastl;sr+=v->lastr;
 if(v->step<v->target)v->step+=(v->target-v->step)/500u+1u;
 v->frac+=v->step;while(v->frac>=65536u){v->frac-=65536u;v->a=v->b;v->b=ta_decode(v);}
 if(v->pos>=TA_BYTES||source>=v->limit)v->active=0;
 any=1;
 }if(v->fade){float f=(float)v->fade*(1.0f/96.0f);sl+=v->fade_l*f;sr+=v->fade_r*f;v->fade--;any=1;}}
 ta_lp_l+=ta_k*(sl-ta_lp_l);ta_lp_r+=ta_k*(sr-ta_lp_r);l[j]=ta_lp_l;r[j]=ta_lp_r;}
 ta_lp_l=fm_flush(ta_lp_l);ta_lp_r=fm_flush(ta_lp_r);
 if(fm_fabsf(ta_lp_l)>1e-7f||fm_fabsf(ta_lp_r)>1e-7f)any=1;
 return any;
}
