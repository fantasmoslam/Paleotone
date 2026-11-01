/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#define AUDIO_PROFILE_TARGET 1
#define FELUCCA_CPU2 1
#define FM1_TICKS_PER_US 24u
static uint32_t ticks,worker;
static uint32_t fm1_ticks(void){return ticks;}
#define C1_ON() worker
#include "audio_profile.h"
int main(void){
 ap_stamp_t s=ap_begin();ticks=240;ap_end(0,s);assert(ap_metric[0].calls==0);
 ap_on=1;ticks=0;t5_nested_ticks=0;s=ap_begin();ticks=240;t5_nested_ticks=48;ap_end(0,s);
 assert(ap_metric[0].calls==1 && ap_metric[0].sum_us==8 && ap_metric[0].max_us==8);
 ticks=0xfffffff0u;t5_nested_ticks=0xfffffff0u;s=ap_begin();ticks+=240;t5_nested_ticks+=24;ap_end(0,s);
 assert(ap_metric[0].calls==2 && ap_metric[0].sum_us==17 && ap_metric[0].max_us==9);
 worker=1;s=ap_track_begin();ticks+=240;ap_end(1,s);assert(ap_metric[1].calls==0);
 worker=0;s=ap_track_begin();ticks+=240;ap_end(1,s);assert(ap_metric[1].calls==1);
 return 0;
}
