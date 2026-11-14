/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <math.h>
#include "../../firmware/src/dsp/amb.h"
#include "../../firmware/src/pt/pt.h"
static int fail;
void en_midi_out(uint32_t s,uint32_t a,uint32_t b){(void)s;(void)a;(void)b;}
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);fail++;}}while(0)
static double audio(int samples){int32_t b[256];double sum=0;int i,n,total=samples;while(samples){n=samples>128?128:samples;en_render(b,n,2800);for(i=0;i<2*n;i++){double v=b[i]/8388608.0;CHECK(isfinite(v));sum+=v*v;}samples-=n;}return sqrt(sum/(2*total));}
static void init(int voice){
 en_init();en_play(0);en_set(P_CHORD,0);en_set(P_SEQ,0);en_set(P_SEQ2,0);en_set(P_CRACKLE,0);en_set(P_SKLEVEL,0);en_set(P_CLLEVEL,0);
 en_set(P_ECHO,0);en_set(P_REVERB,0);en_set(P_CLOUD,0);en_set(P_MOTION,0);en_set(P_LEAD,60);en_set(P_LVOICE,voice);
 en_set(P_LDREV,0);en_set(P_LDECHO,0);en_set(P_VIB,0);audio(1024);
}
static double measure(int voice,int stage,int value,int afterOff){
 init(voice);en_envelope(PT_LEAD,0,0);en_envelope(PT_LEAD,1,0);en_envelope(PT_LEAD,2,127);en_envelope(PT_LEAD,3,0);en_envelope(PT_LEAD,stage,value);
 if(stage==1)en_envelope(PT_LEAD,2,0);
 audio(512);en_lead_midi(60,1);
 if(afterOff){audio(44100);en_lead_midi(60,0);audio(4410);}
 else if(stage!=0)audio(4410);
 return audio(4410);
}
static double layer_attack(int layer,int value){
 init(3);audio(44100*5);en_set(P_LEAD,0);en_envelope(layer,0,value);en_envelope(layer,1,0);en_envelope(layer,2,127);en_envelope(layer,3,0);
 if(layer==PT_DRONE){en_set(P_CVOICE,0);en_set(P_CHORD,60);}
 else if(layer==PT_SEQ){en_set(P_SVOICE,0);en_set(P_SEQ,60);}
 else if(layer==PT_SEQ2){en_set(P_S2VOICE,0);en_set(P_SEQ2,60);}
 else if(layer==EN_ENV_ATMOS){en_set(P_NCHAR,1);en_set(P_CRACKLE,60);}
 else en_set(P_CLLEVEL,60);
 en_play(1);audio(1024);if(layer==PT_CALLS)en_call(0);
 return audio(22050);
}
int main(void){
 int voices[]={3,EN_NBV+13,EN_NBV};unsigned i;
 for(i=0;i<sizeof voices/sizeof voices[0];i++){
  double fast=measure(voices[i],0,0,0),slow=measure(voices[i],0,110,0);
  printf("voice %d attack fast %.6f slow %.6f\n",voices[i],fast,slow);CHECK(fast>slow*3);CHECK(fast>.00001);
  double full=measure(voices[i],2,127,0),zero=measure(voices[i],2,0,0);CHECK(full>zero*4);CHECK(full>.00001);
  double shortD=measure(voices[i],1,0,0),longD=measure(voices[i],1,100,0);CHECK(longD>shortD*4);
  double shortR=measure(voices[i],3,0,1),longR=measure(voices[i],3,100,1);
  printf("voice %d release short %.6f long %.6f\n",voices[i],shortR,longR);CHECK(longR>shortR*4);CHECK(longR>.00001);
 }
 {int layers[]={PT_DRONE,PT_SEQ,PT_SEQ2,PT_CALLS,EN_ENV_ATMOS};
  for(i=0;i<sizeof layers/sizeof layers[0];i++){
   double fast=layer_attack(layers[i],0),slow=layer_attack(layers[i],110);
   printf("layer %d attack fast %.6f slow %.6f\n",layers[i],fast,slow);
   CHECK(fast>.00001);CHECK(fast>slow*2);
   en_play(0);en_call(-1);audio(44100*3);CHECK(audio(4410)<.0001);
  }
 }
 for(i=0;i<PT_NPART;i++){
  pt_init();{int l;for(l=0;l<PT_NPART;l++)pt_quiet(l);}pt_sound((int)i,0);int base=pt_envelope_value(i,0);
  pt_envelope(i,0,100);CHECK(pt_envelope_value(i,0)==100);pt_sound(i,1);CHECK(pt_envelope_value(i,0)==100);
  pt_envelope(i,0,-1);CHECK(pt_envelope_value(i,0)>=0);(void)base;
  /* A manual sustain must still release, including ring/drum-style engines. */
  pt_envelope(i,0,0);pt_envelope(i,1,0);pt_envelope(i,2,127);pt_envelope(i,3,70);
  pt_note_on(i,48,100,0);
  {float out[128];int k;for(k=0;k<100;k++)pt_render(i,out,128);
   pt_notes_off(i);for(k=0;k<44100*12/128;k++)pt_render(i,out,128);}
  printf("layer %u remaining voices %d\n",i,pt_voices());CHECK(pt_voices()==0);
 }
 printf("envelope audio tests: %s\n",fail?"FAIL":"passed");return fail?1:0;
}
