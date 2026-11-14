/* SPDX-License-Identifier: GPL-3.0-only */
/* Desktop scored audition, using the same Paleotone voice DSP as T-WRECKS.
 * This is a fixed musical proof, not a generative firmware change. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#define EN_HOST 1
#include "firmware/src/pt/pt.c"

/* Existing LOG/tom model, lowered to resonant drum pitches. No sample layer. */
static void approach_hit(int lane,int velocity,int semitones,int decay){
 dv_param_t p;dv_default(&p,DV_KTYPE(DV_NKIT,DV_TOM));
 p.tune=(int16_t)(16*semitones);p.tone=76;p.decay=(uint8_t)decay;
 dv_setup(&skin[lane].coef,&p);skin[lane].gain=velocity/(127.0f*32768.0f);dv_trigger(&skin[lane].voice);
}


/* A quiet third compact drum state: low hide-shell and muted rattle. */
static dv_coef_t texture_c;static dv_voice_t texture_v;
static float texture_gain,texture_buf[CTL];static unsigned texture_pos=CTL;
static void texture_hit(int velocity){
 dv_param_t p;dv_default(&p,DV_KTYPE(DV_NKIT,DV_SNARE));
 p.tune=-12*16;p.tone=36;p.decay=30;p.extra=34;
 dv_setup(&texture_c,&p);texture_gain=velocity/(127.0f*32768.0f);dv_trigger(&texture_v);
}
static void texture_render(float*out,unsigned n){unsigned j,i;for(j=0;j<n;j++){
 if(texture_pos>=CTL){int32_t y[CTL];memset(texture_buf,0,sizeof texture_buf);
  if(texture_v.live||texture_v.trig){dv_run(&texture_c,&texture_v,0,y,CTL);for(i=0;i<CTL;i++)texture_buf[i]=y[i]*texture_gain;}
  texture_pos=0;
 }out[j]=texture_buf[texture_pos++];}}

#define SR 44100
#define BPM 54.0
#define BPS (SR*60.0/BPM)
#define N 128
static float buf[4][N];
static void u16(FILE*f,unsigned v){fputc(v&255,f);fputc((v>>8)&255,f);}static void u32(FILE*f,unsigned v){u16(f,v);u16(f,v>>16);}
typedef struct {double beat,len;int role,note,vel;} event_t;
/* Roles: 0 bass, 1 supporting fifth, 2 reserved lead (unused), 3 low tom, 4 higher tom, 5 quiet low hide drum. */
static const event_t score[]={
 {0,15.6,0,24,80},
 {0,14.9,1,36,60},
 {16,15.6,0,25,82},
 {16,14.9,1,37,62},
 {32,7.6,0,24,72},
 {32,6.8,1,36,45},
 {40,7.6,0,24,85},
 {40,6.9,1,36,65},
 {48,7.6,0,25,85},
 {48,6.9,1,37,65},
 {56,8,0,24,82},
 {56,8,1,36,62},
 {0,0,3,0,105},
 {1,0,3,0,88},
 {2,0,4,0,96},
 {4,0,3,0,105},
 {5,0,3,0,88},
 {6,0,4,0,96},
 {8,0,3,0,105},
 {9,0,3,0,88},
 {10,0,4,0,96},
 {12,0,3,0,105},
 {13,0,3,0,88},
 {14,0,4,0,96},
 {15,0,4,0,70},
 {16,0,3,0,105},
 {17,0,3,0,88},
 {18,0,4,0,96},
 {20,0,3,0,105},
 {21,0,3,0,88},
 {22,0,4,0,96},
 {24,0,3,0,105},
 {25,0,3,0,88},
 {26,0,4,0,96},
 {28,0,3,0,105},
 {29,0,3,0,88},
 {30,0,4,0,96},
 {31,0,4,0,70},
 {32,0,3,0,87},
 {34,0,3,0,76},
 {36,0,3,0,87},
 {38,0,3,0,76},
 {40,0,3,0,115},
 {41,0,3,0,98},
 {42,0,4,0,108},
 {44,0,3,0,115},
 {45,0,3,0,98},
 {46,0,4,0,108},
 {47,0,4,0,70},
 {48,0,3,0,115},
 {49,0,3,0,98},
 {50,0,4,0,108},
 {52,0,3,0,115},
 {53,0,3,0,98},
 {54,0,4,0,108},
 {56,0,3,0,115},
 {57,0,3,0,98},
 {58,0,4,0,108},
 {60,0,3,0,115},
 {61,0,3,0,98},
 {62,0,4,0,108},
 {63,0,4,0,70},
 {6.5,0,5,0,31},
 {7.25,0,5,0,23},
 {14.5,0,5,0,35},
 {22.5,0,5,0,32},
 {23.25,0,5,0,25},
 {30.5,0,5,0,34},
 {42.5,0,5,0,33},
 {46.5,0,5,0,35},
 {47.25,0,5,0,24},
 {54.5,0,5,0,37},
 {58.5,0,5,0,34},
 {62.5,0,5,0,32}
};
static int cmp(const void*a,const void*b){const event_t*x=a,*y=b;return x->beat<y->beat?-1:x->beat>y->beat?1:x->role-y->role;}
static float edge(double beat,double at,double end,double attack,double release){double a=(beat-at)/attack,b=(end-beat)/release;double x=a<b?a:b;if(x<0)return 0;if(x>1)return 1;return (float)(x*x*(3-2*x));}
int main(int argc,char**argv){int voices=0,ev=0,j,k,n;unsigned s=0,total=(unsigned)(64*BPS+6*SR);double starts[2]={0,0},ends[2]={15.5,14.7};event_t events[sizeof score/sizeof score[0]];FILE*f,*log;if(argc!=3)return 2;f=fopen(argv[1],"wb");log=fopen(argv[2],"w");if(!f||!log)return 2;
memcpy(events,score,sizeof score);qsort(events,sizeof score/sizeof score[0],sizeof(event_t),cmp);
fwrite("RIFF",1,4,f);u32(f,36+total*16);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,3);u16(f,4);u32(f,SR);u32(f,SR*16);u16(f,16);u16(f,32);fwrite("data",1,4,f);u32(f,total*16);
pt_init();pt_skin_quiet();pt_sound(PT_SEQ2,2);pt_sound(PT_DRONE,25);
/* Remove the organ overdrive, key click, rotary smear and sub drawbars. */
trk[PT_DRONE].p[P_E1]=-8;trk[PT_DRONE].p[P_E3]=-1;trk[PT_DRONE].p[P_E5]=0;trk[PT_DRONE].p[P_E6]=0;trk[PT_DRONE].p[P_E7]=0;
fprintf(log,"time_s,role,note,length_beats,velocity\n");
while(s<total){
 while(ev<(int)(sizeof score/sizeof score[0]) && s>=(unsigned)llround(events[ev].beat*BPS)){
  event_t*e=&events[ev++];unsigned gate=(unsigned)(e->len*BPS);fprintf(log,"%.6f,%d,%d,%.3f,%d\n",s/(double)SR,e->role,e->note,e->len,e->vel);
  if(e->role==0){pt_quiet(PT_SEQ2);pt_note_on(PT_SEQ2,e->note,e->vel,gate);starts[0]=e->beat;ends[0]=e->beat+e->len;}
  if(e->role==1){pt_quiet(PT_DRONE);pt_note_on(PT_DRONE,e->note,e->vel,gate);pt_note_on(PT_DRONE,e->note+7,e->vel,gate);starts[1]=e->beat;ends[1]=e->beat+e->len;}
  if(e->role==2){pt_notes_off(PT_LEAD);pt_note_on(PT_LEAD,e->note,e->vel,gate);}
  if(e->role==3)approach_hit(0,e->vel,-12,60);
  if(e->role==4)approach_hit(2,e->vel,-7,52);
  if(e->role==5)texture_hit(e->vel);
 }
 n=total-s<N?(int)(total-s):N;
 if(ev<(int)(sizeof score/sizeof score[0])){unsigned until=(unsigned)llround(events[ev].beat*BPS)-s;if(until<(unsigned)n)n=(int)until;}
 if(n<=0)continue;
 memset(buf,0,sizeof buf);pt_render(PT_SEQ2,buf[0],n);pt_render(PT_DRONE,buf[1],n);texture_render(buf[2],n);pt_skin_render(buf[3],n);
 if(pt_voices()>voices)voices=pt_voices();
 for(j=0;j<n;j++){double beat=(s+j)/BPS;buf[0][j]*=edge(beat,starts[0],ends[0],.06,.35);buf[1][j]*=edge(beat,starts[1],ends[1],.65,.7);for(k=0;k<4;k++){if(!isfinite(buf[k][j]))return 3;fwrite(&buf[k][j],4,1,f);}}
 s+=(unsigned)n;
}
fclose(f);fclose(log);printf("rendered %.3f seconds, %d events, peak melodic voices %d/8\n",total/(double)SR,ev,voices);return voices>8;}
