/* SPDX-License-Identifier: GPL-3.0-only */
/* Desktop scored audition, using the same Paleotone voice DSP as T-WRECKS.
 * This is a fixed musical proof, not a generative firmware change. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "firmware/src/pt/pt.h"
#define SR 44100
#define BPM 54.0
#define BPS (SR*60.0/BPM)
#define N 128
static float buf[4][N];
static void u16(FILE*f,unsigned v){fputc(v&255,f);fputc((v>>8)&255,f);}static void u32(FILE*f,unsigned v){u16(f,v);u16(f,v>>16);}
typedef struct {double beat,len;int role,note,vel;} event_t;
/* Roles: 0 bass, 1 chord/body (root), 2 horn, 3 earth drum, 4 low tom. */
static const event_t score[]={
 {0,15.5,0,24,105},{0,14.7,1,36,95},
 {4,3.5,2,43,91},{8,3.5,2,51,88},{12,2.0,2,49,82},{14,1.5,2,48,86},
 {16,15.5,0,25,105},{16,14.7,1,37,98},
 {20,3.5,2,44,92},{24,3.5,2,53,87},{28,2.0,2,51,82},{30,1.5,2,49,85},
 {32,7.5,0,24,90},{32,6.8,1,36,55},
 {40,7.5,0,24,113},{40,6.8,1,36,110},
 {40,3.5,2,43,100},{44,1.5,2,51,100},{46,1.5,2,49,89},
 {48,7.5,0,25,112},{48,6.8,1,37,110},
 {48,3.5,2,44,100},{52,1.5,2,53,97},{54,1.5,2,51,92},
 {56,8,0,24,110},{56,8,1,36,110},
 {56,3.5,2,51,94},{60,1.5,2,49,85},{62,2,2,48,92},
 {0,0,3,0,105},{4,0,4,0,65},{8,0,3,0,94},{12,0,4,0,70},
 {16,0,3,0,111},{20,0,4,0,72},{24,0,3,0,101},{28,0,4,0,76},
 {40,0,3,0,116},{42,0,4,0,76},{44,0,3,0,103},{46,0,4,0,82},
 {48,0,3,0,121},{50,0,4,0,78},{52,0,3,0,108},{54,0,4,0,85},
 {56,0,3,0,119},{58,0,4,0,80},{60,0,3,0,105},{62,0,4,0,76}
};
static int cmp(const void*a,const void*b){const event_t*x=a,*y=b;return x->beat<y->beat?-1:x->beat>y->beat?1:x->role-y->role;}
static float edge(double beat,double at,double end,double attack,double release){double a=(beat-at)/attack,b=(end-beat)/release;double x=a<b?a:b;if(x<0)return 0;if(x>1)return 1;return (float)(x*x*(3-2*x));}
int main(int argc,char**argv){int voices=0,ev=0,j,k,n;unsigned s=0,total=(unsigned)(64*BPS+6*SR);double starts[2]={0,0},ends[2]={15.5,14.7};event_t events[sizeof score/sizeof score[0]];FILE*f,*log;if(argc!=3)return 2;f=fopen(argv[1],"wb");log=fopen(argv[2],"w");if(!f||!log)return 2;
memcpy(events,score,sizeof score);qsort(events,sizeof score/sizeof score[0],sizeof(event_t),cmp);
fwrite("RIFF",1,4,f);u32(f,36+total*16);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,3);u16(f,4);u32(f,SR);u32(f,SR*16);u16(f,16);u16(f,32);fwrite("data",1,4,f);u32(f,total*16);
pt_init();pt_skin_quiet();pt_sound(PT_SEQ2,8);pt_sound(PT_DRONE,25);pt_sound(PT_LEAD,10);
fprintf(log,"time_s,role,note,length_beats,velocity\n");
while(s<total){
 while(ev<(int)(sizeof score/sizeof score[0]) && s>=(unsigned)llround(events[ev].beat*BPS)){
  event_t*e=&events[ev++];unsigned gate=(unsigned)(e->len*BPS);fprintf(log,"%.6f,%d,%d,%.3f,%d\n",s/(double)SR,e->role,e->note,e->len,e->vel);
  if(e->role==0){pt_quiet(PT_SEQ2);pt_note_on(PT_SEQ2,e->note,e->vel,gate);starts[0]=e->beat;ends[0]=e->beat+e->len;}
  if(e->role==1){int third=e->beat>=40 && e->beat!=48?15:12;pt_quiet(PT_DRONE);pt_note_on(PT_DRONE,e->note,e->vel,gate);pt_note_on(PT_DRONE,e->note+7,e->vel,gate);if(e->beat!=32)pt_note_on(PT_DRONE,e->note+third,e->vel,gate);starts[1]=e->beat;ends[1]=e->beat+e->len;}
  if(e->role==2){pt_notes_off(PT_LEAD);pt_note_on(PT_LEAD,e->note,e->vel,gate);}
  if(e->role==3)pt_skin_hit(0,e->vel,24,72);
  if(e->role==4)pt_skin_hit(2,e->vel,22,76);
 }
 n=total-s<N?(int)(total-s):N;
 if(ev<(int)(sizeof score/sizeof score[0])){unsigned until=(unsigned)llround(events[ev].beat*BPS)-s;if(until<(unsigned)n)n=(int)until;}
 if(n<=0)continue;
 memset(buf,0,sizeof buf);pt_render(PT_SEQ2,buf[0],n);pt_render(PT_DRONE,buf[1],n);pt_render(PT_LEAD,buf[2],n);pt_skin_render(buf[3],n);
 if(pt_voices()>voices)voices=pt_voices();
 for(j=0;j<n;j++){double beat=(s+j)/BPS;buf[0][j]*=edge(beat,starts[0],ends[0],.06,.35);buf[1][j]*=edge(beat,starts[1],ends[1],.65,.7);for(k=0;k<4;k++){if(!isfinite(buf[k][j]))return 3;fwrite(&buf[k][j],4,1,f);}}
 s+=(unsigned)n;
}
fclose(f);fclose(log);printf("rendered %.3f seconds, %d events, peak melodic voices %d/8\n",total/(double)SR,ev,voices);return voices>8;}
