/* SPDX-License-Identifier: GPL-3.0-only
 * PC-only lookahead planner. Four eight-bar phrases, prepared before playback.
 * Stores scale degrees rather than fixed pitches so harmony can change underneath. */
#pragma once
#include <stdint.h>
#include <string.h>
#define PC_PLAN_TICKS 512
#define PC_PLAN_EVENTS 192
typedef struct {uint16_t tick;uint8_t duration,degree,section;} pc_event;
typedef struct {
 uint32_t rng,cycles;
 int enabled,variation,space,memory,register_oct,seed;
 uint8_t themes[8][8],theme_count,theme_next;
 pc_event events[PC_PLAN_EVENTS];int count;
} pc_planner;
static uint32_t pc_random(pc_planner *p){uint32_t x=p->rng;x^=x<<13;x^=x>>17;x^=x<<5;return p->rng=x?x:1;}
static void pc_plan(pc_planner *p){
 uint8_t theme[8];int i,s,k,offset;
 if(p->theme_count && pc_random(p)%100u<(unsigned)p->memory){
  memcpy(theme,p->themes[pc_random(p)%p->theme_count],8);
 }else{
  static const uint8_t anchor[5]={0,2,4,2,0};
  int pos=0;
  for(i=0;i<8;i++){
   if(!(i%2))pos=anchor[pc_random(p)%5];
   else{pos+=(int)(pc_random(p)%3)-1;if(pos<0)pos=0;if(pos>6)pos=6;}
   theme[i]=(uint8_t)pos;
  }
  memcpy(p->themes[p->theme_next],theme,8);
  p->theme_next=(uint8_t)((p->theme_next+1)%8);if(p->theme_count<8)p->theme_count++;
 }
 p->count=0;
 for(s=0;s<4;s++){
  /* Each phrase has two statements with breathing room, then an ending. */
  for(k=0;k<3;k++)for(i=0;i<8;i++){
   int local=k*40+i*4,degree=theme[i],dur=3;
   if(local>=112)continue;
   if(i%2 && pc_random(p)%100u<(unsigned)p->space)continue;
   if(s==1)degree=6-degree; /* response: invert contour */
   if(s==2 && pc_random(p)%100u<(unsigned)p->variation)
    degree=(degree+((k&1)?2:5))%7; /* develop motif by a diatonic shift */
   if(s==3)degree=theme[7-i]; /* return through the contour */
   offset=s*128+local;
   if(s==2 && p->variation>50 && i%2)offset++; /* modest syncopation */
   if(p->count<PC_PLAN_EVENTS)p->events[p->count++]=(pc_event){(uint16_t)offset,(uint8_t)dur,(uint8_t)degree,(uint8_t)s};
  }
  /* Answer and resolution close on tonic; call/development remain open. */
  if(p->count<PC_PLAN_EVENTS)p->events[p->count++]=(pc_event){(uint16_t)(s*128+116),8,(uint8_t)((s==1||s==3)?0:4),(uint8_t)s};
 }
 p->cycles++;
}
static void pc_planner_init(pc_planner *p){
 memset(p,0,sizeof *p);p->seed=7391;p->rng=7391;p->variation=45;p->space=60;p->memory=75;p->register_oct=0;pc_plan(p);
}
