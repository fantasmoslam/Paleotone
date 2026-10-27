/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the real USB ring/packet code. Measure publication-to-packet wait,
 * not host/DAW latency. Identical 4 ms producer pauses stress old/new bands;
 * delayed blocks catch up without dropping source frames. No SIE is touched. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_OTA 0
#define FELUCCA_CDC T_CDC
#define FELUCCA_UAC 1
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#ifndef UAC_SOURCE
#define UAC_SOURCE "../firmware/src/usb.c"
#endif
#include UAC_SOURCE
static uint64_t published[100000];
static int scenario(int jitter)
{
    uint64_t t, next_block=0, next_packet=1000000, sum=0, low=~(uint64_t)0, high=0;
    uint32_t produced=0, previous=0, count=0, bad=0, repeats=0;
    int32_t block[64]; uint32_t packet[UA_MAXF];
    memset(&uac,0,sizeof uac); ua_w=ua_r=0; usb.config=1;
    uac_stream(1);uac.flowing=1;
    for(t=0;t<2000000000ULL;t+=10000){
        uint64_t phase=t%200000000ULL;
        int pause=jitter && t>=200000000ULL && phase>=50000000ULL && phase<54000000ULL;
        while(!pause && t>=next_block){
            uint32_t i;
            if(produced%HALF_FRAMES==0)uac_render_start();
            for(i=0;i<32;i++){
                uint32_t id=++produced;
                if(id>=100000)return 1;
                published[id]=t;
                block[2*i]=(int16_t)id; block[2*i+1]=(int16_t)(id>>16);
            }
            uac_tap(block,32);
            next_block=(uint64_t)produced*544000ULL/24ULL; /* 24 MHz / 544 */
        }
        if(t>=next_packet){
            uint32_t n=uac_packet(packet),i;
            for(i=0;i<n;i++){
                uint32_t id=packet[i];uint64_t wait;
                if(!id)continue; /* startup silence */
                if(id==previous){repeats++;continue;}
                if(id!=previous+1||id>produced){bad++;continue;}
                previous=id;
                if(t<200000000ULL)continue; /* exclude settling */
                wait=t-published[id];sum+=wait;count++;
                if(wait<low)low=wait;if(wait>high)high=wait;
            }
            next_packet+=1000000;
        }
        if(ua_w-ua_r>UA_N)return 1;
    }
    printf("band=%u..%u jitter=%d queue_ms mean=%.4f min=%.4f max=%.4f samples=%u underruns=%u overruns=%u repeated=%u bad=%u\n",
        UA_LO,UA_HI,jitter,(double)sum/count/1e6,(double)low/1e6,(double)high/1e6,count,uac.underruns,uac.overruns,repeats,bad);
    if(!count||bad||uac.overruns)return 1;
    if(!jitter)return uac.underruns!=0||repeats!=0;
    return UA_LO>=208 ? (uac.underruns!=0||repeats!=0) : (uac.underruns==0||repeats==0);
}
int main(void){int fail=scenario(0);fail+=scenario(1);printf("USB queue scenarios: %s\n",fail?"FAIL":"PASS");return fail!=0;}
