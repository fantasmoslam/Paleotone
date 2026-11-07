/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORK_H
#define ORK_H
#include <stdint.h>
#include <stddef.h>
#define ORK_VOICES 3
#define ORK_FIRST_NOTE 36
#define ORK_NOTES 24
#define ORK_WORDS 4
#define ORK_ENTRY_BYTES 36u
#define ORK_HEADER_BYTES 32u
#define ORK_SLOT_PAYLOAD 81408u
#define ORK_BANK_CAPACITY (3u * ORK_SLOT_PAYLOAD)
typedef uint8_t (*ork_read_fn)(void *,uint32_t);
typedef struct {ork_read_fn read;void *ctx;uint32_t bytes,count,rate,data;uint8_t valid;} ork_bank;
typedef struct {
 uint32_t off,n,ls,le,bytes;int16_t lp;uint8_t li,word,note,flags;
} ork_asset;
typedef struct {
 ork_asset a;uint32_t cursor,phase,step,owner,age;
 int32_t pred,index,current,next,gain,release_step,last,residual;
 uint16_t fade;uint8_t active,held,eof;
} ork_voice;
typedef struct {ork_bank bank;ork_voice voice[ORK_VOICES];uint32_t serial,age,output_rate;} ork_engine;
enum {ORK_SINGLE,ORK_POWER,ORK_MINOR,ORK_MAJOR};
/* open does bounded full validation + CRC. Call while audio is stopped, never in an ISR. */
int ork_bank_open(ork_bank *,ork_read_fn,void *,uint32_t);
int ork_bank_asset(const ork_bank *,uint8_t,uint8_t,ork_asset *);
void ork_voice_start(ork_voice *,const ork_bank *,const ork_asset *,uint32_t,uint8_t,uint32_t);
void ork_voice_release(ork_voice *);
int32_t ork_voice_tick(ork_voice *,const ork_bank *);
void ork_init(ork_engine *,uint32_t);
uint32_t ork_trigger(ork_engine *,uint8_t,uint8_t,uint8_t,uint8_t);
void ork_release(ork_engine *,uint32_t);
void ork_panic(ork_engine *);
void ork_render(ork_engine *,int16_t *,uint32_t);
#endif
