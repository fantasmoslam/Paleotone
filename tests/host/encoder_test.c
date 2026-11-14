/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the physical matrix decoder for every FM-1 encoder and resting
 * phase: full detents, slow turns, mid-click pauses, reversal and bounce. */
#include <stdio.h>
#include <string.h>
#include "fm1_input.h"
static int fails;
static unsigned enc,phase;
#define CHECK(c,...) do{if(!(c)){printf("FAIL encoder %u phase %u: ",enc,phase);printf(__VA_ARGS__);puts("");fails++;}}while(0)
static const unsigned cw[4]={0,1,3,2};
static void frame(unsigned state)
{
    const uint8_t *m=FM1_ENC[enc];
    fm1_in.raw[m[0]]=(uint8_t)((fm1_in.raw[m[0]]&~(1u<<m[1]))|(((state>>1)&1u)<<m[1]));
    fm1_in.raw[m[2]]=(uint8_t)((fm1_in.raw[m[2]]&~(1u<<m[3]))|((state&1u)<<m[3]));
    fm1__frame();
}
static void hold(unsigned state,unsigned frames){while(frames--)frame(state);}
static void reset(unsigned rest)
{
    memset((void*)&fm1_in,0,sizeof fm1_in);
    for(unsigned e=0;e<FM1_NENC;e++)fm1_in.enc_prev[e]=fm1_in.enc_last[e]=0xff;
    hold(rest,60);
}
static unsigned at(int offset){return cw[(phase+offset+16)%4];}
static void clicks(int dir,int count,int scans)
{
    for(int d=0;d<count;d++)for(int k=1;k<=4;k++)hold(at(dir*k),(unsigned)scans);
}
int main(void)
{
    for(enc=0;enc<FM1_NENC;enc++)for(phase=0;phase<4;phase++){
        for(int scans=1;scans<=4;scans*=2){
            reset(at(0));clicks(1,10,scans);
            CHECK(fm1_in.enc_steps[enc]==10,"10 clockwise, %d scans/state: %d",scans,fm1_in.enc_steps[enc]);
            clicks(-1,10,scans);
            CHECK(fm1_in.enc_steps[enc]==0,"reverse 10 clicks: %d",fm1_in.enc_steps[enc]);
        }
        /* Human slow turns must not teach the halfway states as detents. */
        reset(at(0));clicks(1,5,80);
        CHECK(fm1_in.enc_steps[enc]==5,"five slow clicks: %d",fm1_in.enc_steps[enc]);
        reset(at(0));
        for(int d=0;d<5;d++){frame(at(1));hold(at(2),60);frame(at(3));frame(at(4));}
        CHECK(fm1_in.enc_steps[enc]==5,"mid-click pause doubled/lost steps: %d",fm1_in.enc_steps[enc]);
        /* An immediately reversed fast click starts without the previous direction. */
        reset(at(0));clicks(1,1,1);fm1_in.enc_steps[enc]=0;
        frame(at(-2));frame(at(-3));frame(at(-4));
        CHECK(fm1_in.enc_steps[enc]==-1,"reversed click with skipped state: %d",fm1_in.enc_steps[enc]);
        /* A skipped phase is recoverable once this click supplies direction. */
        reset(at(0));
        for(int d=0;d<10;d++){frame(at(1));frame(at(3));frame(at(4));}
        CHECK(fm1_in.enc_steps[enc]==10,"observable fast skipped phase: %d",fm1_in.enc_steps[enc]);
        for(int d=0;d<10;d++){frame(at(-1));frame(at(-3));frame(at(-4));}
        CHECK(fm1_in.enc_steps[enc]==0,"observable reversed skipped phase: %d",fm1_in.enc_steps[enc]);
        /* Two opposite sampled states alone contain no direction information.
         * A preceding click must not make them invent continued movement. */
        reset(at(0));clicks(1,1,1);fm1_in.enc_steps[enc]=0;
        for(int d=0;d<9;d++){frame(at(2));frame(at(4));}
        CHECK(fm1_in.enc_steps[enc]==0,"stale direction invented %d steps",fm1_in.enc_steps[enc]);
        /* Bounce and a partially reversed click must not create an item. */
        reset(at(0));frame(at(1));frame(at(0));
        frame(at(1));frame(at(2));frame(at(1));frame(at(0));
        CHECK(fm1_in.enc_steps[enc]==0,"bounce/partial reversal produced %d",fm1_in.enc_steps[enc]);
        /* Booting between detents recovers only after a long stationary park. */
        reset(at(1));hold(at(0),FM1_REST_FRAMES+1);fm1_in.enc_steps[enc]=0;
        clicks(1,5,1);
        CHECK(fm1_in.enc_steps[enc]==5,"parked recovery: %d",fm1_in.enc_steps[enc]);
    }
    printf("encoder: all seven controls/four phases, %d failures\n",fails);
    return fails!=0;
}
