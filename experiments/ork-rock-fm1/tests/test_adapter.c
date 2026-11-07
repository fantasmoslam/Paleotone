/* SPDX-License-Identifier: GPL-3.0-only */
/* Type-contract compilation of the real adapter. This is not a board emulator. */
#include <stdint.h>
#include <string.h>
#define NVOICE 12
#define FS 44100
#define SMP_USER_SLOTS 3
#define SMP_USER_DATA 512
#define SMP_USER_MAGIC 0x504d5346u
#define ENG_MEM_FITS(n,s) _Static_assert((s)<12888,n)
#define F_ENUM 1
#define F_INT 0
#define P_E0 0
#define FX(a,b,c,d) .fx={0,0,0,0}
#define PAT(a) .pat=0
typedef struct {int32_t pitch16;uint8_t vel,gate;} voice_t;
typedef struct track {int16_t p[8];voice_t v[NVOICE];} track_t;
typedef struct {int32_t unused;} vmod_t;
typedef struct {const char *name;int p[8],env[4];int a,b,fx[4],pat;} preset_t;
typedef struct {const char *label;int fmt,min,max,def;const char *const *names;const char *unit;} desc_t;
typedef struct {const char *name;const char *page_title[2];desc_t edit[8];const preset_t *presets;uint32_t npresets,poly,sampled;uint8_t knob[4];void(*note_on)(track_t*,voice_t*);void(*render)(track_t*,voice_t*,int32_t*,uint32_t,const vmod_t*);uint8_t ownenv;int(*done)(track_t*,voice_t*);uint16_t mem;} engine_t;
typedef struct {uint32_t magic;uint16_t version;uint8_t nz,rsv;char name[8];} smp_user_hdr_t;
static track_t trk[4];static uint8_t mem[4][12888],slots[3][81920];
#define ENG_MEM(p) mem[p]
static const uint8_t *smp_user_xip(uint32_t n){return slots[n];}
static int32_t voice_amp(int32_t x,const vmod_t *m,uint32_t i){(void)m;(void)i;return x;}
#include "paleotone_adapter.c"
int main(void){ork_felucca_prepare();return ENG_ORK.poly==3&&ENG_ORK.mem<12888?0:1;}
