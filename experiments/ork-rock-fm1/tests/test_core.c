/* SPDX-License-Identifier: GPL-3.0-only */
#include "ork.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef struct {uint8_t *data;uint32_t n;} source;
static uint8_t read_byte(void *ctx,uint32_t p){source *s=(source *)ctx;assert(p<s->n);return s->data[p];}
static void fix_crc(source *s){uint32_t c=~0u,i,j;for(i=32;i<s->n;i++){c^=s->data[i];for(j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}c=~c;for(i=0;i<4;i++)s->data[24+i]=(uint8_t)(c>>(8*i));}
static uint32_t active(const ork_engine *e){uint32_t n=0,i;for(i=0;i<ORK_VOICES;i++)n+=e->voice[i].active!=0;return n;}
int main(int argc,char **argv){
 FILE *f;source s;ork_engine e,a,b;ork_bank bad;int16_t x[257],one[257];uint32_t id,old,i,nz=0;uint8_t *copy;
 assert(argc==2);f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);s.n=(uint32_t)ftell(f);rewind(f);s.data=(uint8_t *)malloc(s.n);assert(s.data&&fread(s.data,1,s.n,f)==s.n);fclose(f);copy=(uint8_t *)malloc(s.n);assert(copy);memcpy(copy,s.data,s.n);
 ork_init(&e,44100);assert(ork_bank_open(&e.bank,read_byte,&s,s.n));assert(!ork_bank_open(&bad,read_byte,&s,10));
 s.data[0]^=1;assert(!ork_bank_open(&bad,read_byte,&s,s.n));memcpy(s.data,copy,s.n);
 s.data[s.n-1]^=1;assert(!ork_bank_open(&bad,read_byte,&s,s.n));memcpy(s.data,copy,s.n);
 s.data[32+22]=89;fix_crc(&s);assert(!ork_bank_open(&bad,read_byte,&s,s.n));memcpy(s.data,copy,s.n);
 memset(s.data+32+4,255,4);fix_crc(&s);assert(!ork_bank_open(&bad,read_byte,&s,s.n));memcpy(s.data,copy,s.n);
 s.data[32+36+1]=s.data[32+1];fix_crc(&s);assert(!ork_bank_open(&bad,read_byte,&s,s.n));memcpy(s.data,copy,s.n);
 memset(s.data+32+16,0,4);fix_crc(&s);assert(!ork_bank_open(&bad,read_byte,&s,s.n));memcpy(s.data,copy,s.n);
 id=ork_trigger(&e,0,38,ORK_POWER,127);assert(id&&active(&e)==3);
 for(i=0;i<80;i++){ork_render(&e,x,257);for(uint32_t k=0;k<257;k++){assert(x[k]!=32767&&x[k]!=-32768);nz+=x[k]!=0;}}assert(nz>1000&&active(&e)==3);
 old=id;id=ork_trigger(&e,0,41,ORK_MINOR,100);assert(id&&id!=old);ork_release(&e,old);assert(active(&e)==3);for(i=0;i<3;i++)assert(e.voice[i].held);
 assert(!ork_trigger(&e,0,59,ORK_POWER,100));for(i=0;i<3;i++)assert(e.voice[i].held); /* failed chord admission is atomic */
 ork_release(&e,id);for(i=0;i<60;i++)ork_render(&e,x,257);assert(!active(&e));for(i=0;i<257;i++)assert(!x[i]);
 id=ork_trigger(&e,0,38,ORK_SINGLE,100);assert(id);ork_panic(&e);ork_render(&e,x,257);for(i=0;i<257;i++)assert(!x[i]);
 ork_init(&a,44100);assert(ork_bank_open(&a.bank,read_byte,&s,s.n));b=a;assert(ork_trigger(&a,0,43,ORK_SINGLE,100));assert(ork_trigger(&b,0,43,ORK_SINGLE,100));ork_render(&a,x,257);for(i=0;i<257;i++)ork_render(&b,&one[i],1);assert(!memcmp(x,one,sizeof x));
 ork_init(&a,1);assert(ork_bank_open(&a.bank,read_byte,&s,s.n));assert(!ork_trigger(&a,0,43,ORK_SINGLE,100));
 printf("PASS: malformed banks, bounded reads, held loops, chord admission, ownership, stealing, release, panic, block invariance, rate validation\n");
 printf("Engine RAM: %zu bytes; voice: %zu bytes; bank metadata: %zu bytes\n",sizeof(ork_engine),sizeof(ork_voice),sizeof(ork_bank));free(copy);free(s.data);return 0;
}
