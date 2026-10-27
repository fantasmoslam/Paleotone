/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#define FELUCCA_OTA 1
#define FELUCCA_CDC 0
#define FELUCCA_UAC 0
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static void fm1_delay_ms(uint32_t n){(void)n;}
#include "../firmware/src/usb.c"
static uint32_t clock_ms;
static uint32_t ota_now_ms(void){return clock_ms++;}
static void ota_idle(void){}
int main(void){
 const uint8_t frame[]={240,125,70,76,29,2,247};unsigned i;int bad=0;
 for(i=0;i<sizeof frame;i++)sysex_byte(frame[i]);
 bad|=midi_trace.rx_start!=1||midi_trace.rx_done!=1||midi_trace.rx_busy!=0||midi_trace.rx_cmd!=29||midi_trace.rx_track!=2;
 for(i=0;i<sizeof frame;i++)sysex_byte(frame[i]);
 bad|=midi_trace.rx_start!=2||midi_trace.rx_busy!=1||midi_trace.rx_done!=1;
 ota_frame_done();for(i=0;i<sizeof frame;i++)sysex_byte(frame[i]);
 bad|=midi_trace.rx_done!=2;
 usb.config=1;bad|=ota_wire_send(frame,sizeof frame)!=0;
 bad|=midi_trace.tx29!=1||midi_trace.done29!=1;
 so_w=so_r+SXQ;bad|=ota_wire_send(frame,sizeof frame)!=-1;
 bad|=midi_trace.tx29!=2||midi_trace.done29!=1||midi_trace.tx_timeout!=1||midi_trace.tx_fail_cmd!=29;
 printf("MIDI trace accepted/busy RX and queued/timeout TX: %s\n",bad?"FAIL":"PASS");return bad;
}
