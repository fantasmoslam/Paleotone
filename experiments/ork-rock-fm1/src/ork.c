/* SPDX-License-Identifier: GPL-3.0-only
 * Bounded recorded-syllable player. No allocation, float, locks, or storage writes.
 * IMA ADPCM step/index convention matches Felucca's sampleio.py (low nibble first).
 */
#include "ork.h"
#ifndef ORK_EMBEDDED
#include <string.h>
#endif
static const int16_t ork_ima_steps[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const int8_t ork_ima_indices[8]={-1,-1,-1,-1,2,4,6,8};
static int32_t ork_clamp_i(int32_t x,int32_t a,int32_t b){return x<a?a:x>b?b:x;}
static uint8_t ork_rd(const ork_bank *b,uint32_t p){return p<b->bytes?b->read(b->ctx,p):0;}
static uint16_t ork_u16(const ork_bank *b,uint32_t p){return (uint16_t)(ork_rd(b,p)|((uint16_t)ork_rd(b,p+1)<<8));}
static uint32_t ork_u32(const ork_bank *b,uint32_t p){return (uint32_t)ork_u16(b,p)|((uint32_t)ork_u16(b,p+2)<<16);}
static uint32_t ork_crc(const ork_bank *b,uint32_t from,uint32_t n){uint32_t c=~0u,i;while(n--){c^=ork_rd(b,from++);for(i=0;i<8;i++)c=(c>>1)^(0xedb88320u & (0u-(c&1u)));}return ~c;}
static void ork_asset_at(const ork_bank *b,uint32_t i,ork_asset *a){
 uint32_t p=ORK_HEADER_BYTES+i*ORK_ENTRY_BYTES;
 memset(a,0,sizeof *a);a->word=ork_rd(b,p);a->note=ork_rd(b,p+1);a->flags=(uint8_t)ork_u16(b,p+2);
 a->off=ork_u32(b,p+4);a->n=ork_u32(b,p+8);a->ls=ork_u32(b,p+12);a->le=ork_u32(b,p+16);
 a->lp=(int16_t)ork_u16(b,p+20);a->li=ork_rd(b,p+22);a->bytes=ork_u32(b,p+28);
}
int ork_bank_open(ork_bank *b,ork_read_fn read,void *ctx,uint32_t n){
 uint32_t total,i,j;ork_asset a,other;memset(b,0,sizeof *b);
 if(!read||n<ORK_HEADER_BYTES||n>ORK_BANK_CAPACITY)return 0;
 b->read=read;b->ctx=ctx;b->bytes=n;
 if(ork_u32(b,0)!=0x424b524fu||ork_u16(b,4)!=1u||ork_u16(b,6)!=ORK_ENTRY_BYTES)return 0;
 b->count=ork_u32(b,8);b->rate=ork_u32(b,12);b->data=ork_u32(b,16);total=ork_u32(b,20);
 if(!b->count||b->count>ORK_WORDS*ORK_NOTES||b->rate<8000||b->rate>24000||total>n||total<ORK_HEADER_BYTES||b->data!=ORK_HEADER_BYTES+b->count*ORK_ENTRY_BYTES||b->data>total)return 0;
 b->bytes=total;
 if(ork_crc(b,ORK_HEADER_BYTES,total-ORK_HEADER_BYTES)!=ork_u32(b,24))return 0;
 for(i=0;i<b->count;i++){
  uint32_t p=ORK_HEADER_BYTES+i*ORK_ENTRY_BYTES;ork_asset_at(b,i,&a);
  if(a.word>=ORK_WORDS||a.note<ORK_FIRST_NOTE||a.note>=ORK_FIRST_NOTE+ORK_NOTES||!a.n||a.n>24000||ork_u16(b,p+2)!=1||a.li>88||a.ls>=a.le||a.le>a.n||a.bytes!=(a.n+1)/2||a.off>total-b->data||a.bytes>total-b->data-a.off||ork_u32(b,p+24)!=b->rate)return 0;
  for(j=0;j<i;j++){ork_asset_at(b,j,&other);if(a.word==other.word&&a.note==other.note)return 0;}
 }
 b->valid=1;return 1;
}
int ork_bank_asset(const ork_bank *b,uint8_t word,uint8_t note,ork_asset *a){uint32_t i;if(!b||!b->valid||!a)return 0;for(i=0;i<b->count;i++){ork_asset_at(b,i,a);if(a->word==word&&a->note==note)return 1;}return 0;}
static int32_t ork_decode(ork_voice *v,const ork_bank *b){
 uint32_t code;int32_t d,step;
 if(v->held&&v->cursor>=v->a.le){v->cursor=v->a.ls;v->pred=v->a.lp;v->index=v->a.li;}
 if(v->cursor>=v->a.n){v->eof=1;return 0;}
 code=ork_rd(b,b->data+v->a.off+v->cursor/2);code=(code>>((v->cursor&1u)*4))&15u;v->cursor++;
 step=ork_ima_steps[v->index];d=step>>3;if(code&1)d+=step>>2;if(code&2)d+=step>>1;if(code&4)d+=step;
 v->pred=ork_clamp_i(v->pred+((code&8)?-d:d),-32768,32767);v->index=ork_clamp_i(v->index+ork_ima_indices[code&7],0,88);return v->pred;
}
void ork_voice_start(ork_voice *v,const ork_bank *b,const ork_asset *a,uint32_t rate,uint8_t velocity,uint32_t owner){
 int32_t residual=v->active?v->last:0;memset(v,0,sizeof *v);v->a=*a;
 if(rate<8000||rate>96000||!b->valid)return;
 v->step=(uint32_t)(((uint64_t)b->rate<<16)/rate);v->owner=owner;v->active=1;v->held=1;
 v->gain=(int32_t)(velocity>127?127:velocity)*86; /* ~-9.5 dB per singer: three cannot overflow by summing. */
 v->release_step=(v->gain+(int32_t)(rate*16u/100u)-1)/(int32_t)(rate*16u/100u);
 if(v->release_step<1)v->release_step=1;
 v->current=ork_decode(v,b);v->next=ork_decode(v,b);v->residual=residual;v->fade=64;
}
void ork_voice_release(ork_voice *v){v->held=0;}
int32_t ork_voice_tick(ork_voice *v,const ork_bank *b){
 int32_t x;if(!b->valid){v->active=0;return 0;}if(!v->active)return 0;
 x=v->current+(int32_t)(((int64_t)(v->next-v->current)*(v->phase&65535u))>>16);
 x=(int32_t)(((int64_t)x*v->gain)>>15);
 if(v->fade){x=(x*(64-v->fade)+v->residual*v->fade)/64;v->fade--;}
 v->phase+=v->step;while(v->phase>=65536u){v->phase-=65536u;v->current=v->next;v->next=ork_decode(v,b);}
 if(!v->held){v->gain-=v->release_step;if(v->gain<=0||v->eof){v->active=0;x=0;}}
 v->last=x;return x;
}
void ork_init(ork_engine *e,uint32_t rate){memset(e,0,sizeof *e);e->output_rate=rate;}
uint32_t ork_trigger(ork_engine *e,uint8_t word,uint8_t root,uint8_t shape,uint8_t velocity){
 static const uint8_t intervals[4][3]={{0,0,0},{0,7,12},{0,3,7},{0,4,7}};
 ork_asset a[3];uint32_t i,j,n=shape==ORK_SINGLE?1u:3u,id;
 if(shape>ORK_MAJOR||!velocity||e->output_rate<8000||e->output_rate>96000)return 0;
 for(i=0;i<n;i++){uint32_t note=root+intervals[shape][i];if(note>127||!ork_bank_asset(&e->bank,word,(uint8_t)note,&a[i]))return 0;}
 id=++e->serial;if(!id)id=++e->serial;
 for(i=0;i<n;i++){
  uint32_t slot=0;for(j=0;j<ORK_VOICES;j++)if(!e->voice[j].active){slot=j;break;}else if(e->voice[j].age<e->voice[slot].age)slot=j;
  ork_voice_start(&e->voice[slot],&e->bank,&a[i],e->output_rate,velocity,id);e->voice[slot].age=++e->age;
 }
 return id;
}
void ork_release(ork_engine *e,uint32_t id){uint32_t i;for(i=0;i<ORK_VOICES;i++)if(e->voice[i].owner==id)ork_voice_release(&e->voice[i]);}
void ork_panic(ork_engine *e){memset(e->voice,0,sizeof e->voice);}
void ork_render(ork_engine *e,int16_t *out,uint32_t n){uint32_t i,j;for(i=0;i<n;i++){int32_t x=0;for(j=0;j<ORK_VOICES;j++)x+=ork_voice_tick(&e->voice[j],&e->bank);out[i]=(int16_t)ork_clamp_i(x,-32768,32767);}}
