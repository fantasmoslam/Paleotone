/* SPDX-License-Identifier: GPL-3.0-only */
/* Same portable C player as the device adapter; file I/O only in this host tool. */
#include "ork.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint8_t *p;uint32_t n;} source;
static uint8_t read_byte(void *ctx,uint32_t p){source *s=(source *)ctx;return p<s->n?s->p[p]:0;}
static void w16(FILE *f,uint16_t v){fputc(v&255,f);fputc(v>>8,f);}
static void w32(FILE *f,uint32_t v){w16(f,(uint16_t)v);w16(f,(uint16_t)(v>>16));}
int main(int argc,char **argv){
 FILE *f,*o;source s;ork_engine e;int16_t out;uint32_t i,id=0,frames=44100*10u;uint8_t words[8]={0,1,2,3,0,2,1,3};
 if(argc!=3){fprintf(stderr,"usage: ork-render BANK OUTPUT.wav\n");return 2;}
 f=fopen(argv[1],"rb");if(!f)return 3;fseek(f,0,SEEK_END);s.n=(uint32_t)ftell(f);rewind(f);s.p=(uint8_t *)malloc(s.n);if(!s.p||fread(s.p,1,s.n,f)!=s.n)return 4;fclose(f);
 ork_init(&e,44100);if(!ork_bank_open(&e.bank,read_byte,&s,s.n)){fprintf(stderr,"invalid bank\n");free(s.p);return 5;}
 o=fopen(argv[2],"wb");if(!o){free(s.p);return 6;}fwrite("RIFF",1,4,o);w32(o,frames*2+36);fwrite("WAVEfmt ",1,8,o);w32(o,16);w16(o,1);w16(o,1);w32(o,44100);w32(o,88200);w16(o,2);w16(o,16);fwrite("data",1,4,o);w32(o,frames*2);
 for(i=0;i<frames;i++){
  if(i>=4410&&i<4410+8u*22050u&&(i-4410)%22050u==0){uint32_t step=(i-4410)/22050u;id=ork_trigger(&e,words[step],(uint8_t)(step<4?38:41),ORK_POWER,115);}
  if(i>=4410&&(i-4410)%22050u==16000u)ork_release(&e,id);
  if(i==44100u*5u)id=ork_trigger(&e,0,38,ORK_MINOR,115);
  if(i==44100u*7u)ork_release(&e,id);
  if(i==44100u*8u)ork_panic(&e);
  ork_render(&e,&out,1);w16(o,(uint16_t)out);
 }
 fclose(o);free(s.p);printf("rendered %u frames at 44100 Hz; same C decoder and loop player as device\n",frames);return 0;
}
