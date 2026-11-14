/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#define API __declspec(dllexport)
static CRITICAL_SECTION guard;
static int guard_ready;
void pc_lock(void){if(guard_ready)EnterCriticalSection(&guard);}
void pc_unlock(void){if(guard_ready)LeaveCriticalSection(&guard);}
#else
#define API
void pc_lock(void){}
void pc_unlock(void){}
#endif
#include "../firmware/src/app/app.h"
#include "../firmware/src/pt/pt.h"
int plat_store_load(uint32_t o,void *p,uint32_t n){(void)o;(void)p;(void)n;return -1;}
int plat_store_save(uint32_t o,const void *p,uint32_t n){(void)o;(void)p;(void)n;return 0;}
void en_midi_out(uint32_t a,uint32_t b,uint32_t c){(void)a;(void)b;(void)c;}
#include "../firmware/src/app/project.c"
#include "planner.h"
extern void en_lead_auto(int on);
static pc_planner planner;
static double clock_samples;
static int tick_index,event_index,note=-1,off_tick=-1,playing,manual_note=-1;
static int master_gain=2400;
static int wait_bar;
static float peak;
static const int controls[7][4]={
 {P_CHORD,P_CCOL,P_MW0,P_DRREV},{P_SEQ,P_CUTOFF,P_MW1,P_SQREV},
 {P_LEAD,P_LTONE,P_MW2,P_LDREV},{P_SEQ2,P_S2CUT,P_MW3,P_S2REV},
 {P_CRACKLE,P_NCOL,P_MW4,P_NPLATE},{P_SKLEVEL,P_SKTONE,P_SKDENS,P_SKVERB},
 {P_CLLEVEL,P_CLTONE,P_CLCHANCE,P_CLVERB}};
static const int voices[7]={P_CVOICE,P_SVOICE,P_LVOICE,P_S2VOICE,P_NCHAR,-1,P_CLVOICE};
static int env_layer(int tr){return tr==4?EN_ENV_ATMOS:tr==6?PT_CALLS:tr<4?tr:-1;}
static void release_note(void){if(note>=0)en_lead_midi(note,0);note=-1;off_tick=-1;}
static void reset_clock(void){wait_bar=0;release_note();clock_samples=0;tick_index=0;event_index=0;}
static void flush(void){int32_t b[2];en_render(b,1,0);}
API void pc_init(void){
#ifdef _WIN32
 if(!guard_ready){InitializeCriticalSection(&guard);guard_ready=1;}
#endif
 pc_lock();en_init();project_defaults();project_piece(2);proj.par[P_LVOICE]=3;
 pc_planner_init(&planner);playing=0;manual_note=-1;project_apply();en_regrow();en_play(0);en_lead_auto(1);reset_clock();flush();pc_unlock();
}
API int pc_scene_count(void){return NPIECES;}
API const char *pc_scene_name(int i){return en_piece_name(i);}
API int pc_scene_current(void){return proj.piece;}
API void pc_scene(int i){if(i<0||i>=NPIECES)return;pc_lock();release_note();project_piece(i);project_apply();en_regrow();en_play(playing);reset_clock();flush();pc_unlock();}
API int pc_param_count(void){return P_NPARAMS;}
API const char *pc_param_name(int p){return p>=0&&p<P_NPARAMS?en_param_info(p)->name:"";}
API int pc_get(int p){return p>=0&&p<P_NPARAMS?proj.par[p]:0;}
API int pc_min(int p){return p>=0&&p<P_NPARAMS?en_param_info(p)->lo:0;}
API int pc_max(int p){return p>=0&&p<P_NPARAMS?en_param_info(p)->hi:0;}
API void pc_set(int p,int v){if(p<0||p>=P_NPARAMS)return;pc_lock();const en_param_t *q=en_param_info(p);v=v<q->lo?q->lo:v>q->hi?q->hi:v;proj.par[p]=(int16_t)v;en_set(p,v);if(p==P_MODE){proj.pool=EN_MODE_POOL[v];en_pool(proj.pool);}pc_unlock();}
API int pc_track_param(int tr,int col){return tr>=0&&tr<7&&col>=0&&col<4?controls[tr][col]:-1;}
API int pc_voice_param(int tr){return tr>=0&&tr<7?voices[tr]:-1;}
API void pc_value_text(int p,int v,char *out){if(p>=0&&p<P_NPARAMS)en_param_text(p,v,out);else out[0]=0;}
API uint32_t pc_envelope_ms10(int v){return pt_envelope_ms10(v);}
API int pc_master_get(void){return master_gain;}
API int pc_envelope_get(int tr,int s){int l=env_layer(tr);return l>=0&&s>=0&&s<4?project_envelope(l,s):-1;}
API void pc_envelope_set(int tr,int s,int v){int l=env_layer(tr);if(l<0||s<0||s>3)return;pc_lock();project_envelope_set(l,s,v);pc_unlock();}
API int pc_beat(void){return project_drum_pattern();}
API const char *pc_beat_name(int i){return en_beat_name(i);}
API void pc_beat_set(int i){pc_lock();project_drum_pattern_set(i);en_drum_pattern(project_drum_pattern());pc_unlock();}
API int pc_percussion_decay_id(void){return P_SKDECAY;}
API int pc_global_id(int i){static const int ids[]={P_TEMPO,P_ROOT,P_MODE,P_MOTION,P_ECHO,P_REVERB,P_CLOUD,P_LDIN,P_BRIDGE,P_ARC,P_LMEM};return i>=0&&i<11?ids[i]:-1;}
API void pc_master(int v){pc_lock();master_gain=v<0?0:v>4096?4096:v;pc_unlock();}
API void pc_play(int on){pc_lock();playing=!!on;if(!playing){release_note();if(manual_note>=0)en_lead_midi(manual_note,0);manual_note=-1;}en_play(playing);pc_unlock();}
API int pc_playing(void){return playing;}
API int pc_pad_note(int degree){int scale[12],n=en_scale(proj.pool,scale);return 48+proj.par[P_ROOT]+(n?scale[degree%n]+12*(degree/n):2*degree);}
API void pc_note(int n,int on){pc_lock();if(on){release_note();if(manual_note>=0)en_lead_midi(manual_note,0);manual_note=n;en_lead_midi(n,1);}else if(n<0||n==manual_note){if(manual_note>=0)en_lead_midi(manual_note,0);manual_note=-1;}pc_unlock();}
API void pc_call(int n){pc_lock();en_call(n);pc_unlock();}
API void pc_panic(void){pc_lock();release_note();manual_note=-1;playing=0;en_play(0);en_panic();pc_unlock();}
API int pc_planner_get(int p){switch(p){case 0:return planner.enabled;case 1:return planner.variation;case 2:return planner.space;case 3:return planner.memory;case 4:return planner.register_oct;case 5:return planner.seed;}return 0;}
API void pc_planner_set(int p,int v){
 pc_lock();if(p==0){planner.enabled=!!v;en_lead_auto(!planner.enabled);reset_clock();if(planner.enabled&&playing)wait_bar=(int)en_bars+1;}
 else if(p>=1&&p<=3){v=v<0?0:v>100?100:v;if(p==1)planner.variation=v;if(p==2)planner.space=v;if(p==3)planner.memory=v;}
 else if(p==4)planner.register_oct=v<-2?-2:v>2?2:v;
 else if(p==5){planner.seed=v;planner.rng=(uint32_t)v? (uint32_t)v:1;planner.theme_count=planner.theme_next=0;pc_plan(&planner);reset_clock();}
 pc_unlock();
}
API void pc_replan(void){pc_lock();pc_plan(&planner);reset_clock();pc_unlock();}
API int pc_plan_count(void){return planner.count;}
API int pc_plan_event(int i,int field){if(i<0||i>=planner.count)return -1;pc_event *e=&planner.events[i];return field==0?e->tick:field==1?e->duration:field==2?e->degree:e->section;}
API int pc_tick(void){return tick_index;}
API int pc_cycle(void){return (int)planner.cycles;}
API float pc_peak(void){return peak;}
static void planner_tick(void){
 if(tick_index>=PC_PLAN_TICKS){release_note();pc_plan(&planner);tick_index=event_index=0;}
 if(off_tick>=0&&tick_index>=off_tick)release_note();
 while(event_index<planner.count&&planner.events[event_index].tick<=tick_index){
  pc_event *e=&planner.events[event_index++];
  if(e->tick==tick_index&&manual_note<0){
   int scale[12],n=en_scale(proj.pool,scale),deg=e->degree;
   int root=proj.par[P_ROOT];
   if(!(e->section==3&&e->tick==500))deg+=en_deg_of(proj.pool,((int)en_chord_root-root+12)%12);
   int semi=n?scale[deg%n]+12*(deg/n):deg*2;
   release_note();note=48+root+semi+12*planner.register_oct;en_lead_midi(note,1);off_tick=tick_index+e->duration;
  }
 }
 tick_index++;
}
API void pc_render(float *out,int frames){
 int32_t b[256];int done=0,j;pc_lock();peak=0;
 while(done<frames){int n=frames-done;if(n>128)n=128;
  if(wait_bar&&(int)en_bars>=wait_bar)wait_bar=0;
  if(planner.enabled&&playing&&!wait_bar){double length=EN_SR*60.0/(proj.par[P_TEMPO]*4.0);if(clock_samples<=0){planner_tick();clock_samples+=length;}if(n>(int)ceil(clock_samples))n=(int)ceil(clock_samples);clock_samples-=n;}
  en_render(b,(uint32_t)n,(uint32_t)master_gain);
  for(j=0;j<2*n;j++){float x=b[j]*(1.0f/8388608.0f);out[done*2+j]=x;if(fabsf(x)>peak)peak=fabsf(x);}
  done+=n;
 }
 pc_unlock();
}
/* Versioned desktop project: all scenes plus motif bank and planner settings. */
typedef struct {uint32_t magic,version,size,hash;project_t project;pc_planner plan;int32_t gain;} pc_saved;
static uint32_t checksum(const uint8_t *p,int n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
API int pc_save_size(void){return sizeof(pc_saved);}
API int pc_save(uint8_t *out,int cap){pc_saved s;if(cap<(int)sizeof s)return -1;pc_lock();keep();memset(&s,0,sizeof s);s.magic=0x3150434e;s.version=1;s.size=sizeof s;s.project=proj;s.plan=planner;s.gain=master_gain;s.hash=checksum((uint8_t*)&s+16,sizeof s-16);memcpy(out,&s,sizeof s);pc_unlock();return sizeof s;}
API int pc_load(const uint8_t *data,int len){
 pc_saved s;project_t prior;int i,l,k;if(len!=(int)sizeof s)return -1;memcpy(&s,data,sizeof s);
 if(s.magic!=0x3150434e||s.version!=1||s.size!=sizeof s||s.hash!=checksum(data+16,len-16))return -1;
 if(s.plan.count<0||s.plan.count>PC_PLAN_EVENTS||s.plan.theme_count>8||s.plan.theme_next>=8||s.plan.enabled<0||s.plan.enabled>1||s.plan.variation<0||s.plan.variation>100||s.plan.space<0||s.plan.space>100||s.plan.memory<0||s.plan.memory>100||s.plan.register_oct<-2||s.plan.register_oct>2||s.gain<0||s.gain>4096)return -1;
 for(i=0;i<NPIECES;i++)for(l=0;l<EN_ENV_LAYERS;l++)for(k=0;k<4;k++)if(s.project.envelope[i][l][k]>128)return -1;
 for(i=0;i<s.plan.count;i++)if(s.plan.events[i].tick>=PC_PLAN_TICKS||s.plan.events[i].degree>6||!s.plan.events[i].duration||s.plan.events[i].section>3)return -1;
 pc_lock();prior=proj;proj=s.project;if(!project_valid()){proj=prior;pc_unlock();return -1;}
 en_panic();planner=s.plan;master_gain=s.gain;manual_note=-1;playing=0;project_apply();en_regrow();en_lead_auto(!planner.enabled);en_play(0);reset_clock();flush();pc_unlock();return 0;
}
