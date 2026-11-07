/* SPDX-License-Identifier: GPL-3.0-only */
/* Included by engines.c AFTER eng_sample.c. Target: the inspected Paleotone
 * engine_t API. Prepare validates the bank before audio starts. Three existing
 * user sample slots hold the bank; no storage is written by this adapter.
 */
#define ORK_EMBEDDED 1
#include "ork/ork.c"
#undef ORK_EMBEDDED
typedef struct {ork_voice voice[NVOICE];} ork_part_state;
ENG_MEM_FITS("ORK",sizeof(ork_part_state));
static ork_bank ork_device_bank;
static const char *const ork_words[]={"GRAH","BOM","DOOM","RUM"};
static uint8_t ork_device_read(void *ctx,uint32_t offset){
 uint32_t slot=offset/ORK_SLOT_PAYLOAD,within=offset%ORK_SLOT_PAYLOAD;(void)ctx;
 if(slot>=SMP_USER_SLOTS)return 0;
 return smp_user_xip(slot)[SMP_USER_DATA+within];
}
/* Main thread, with audio stopped. Refresh after bank uploads requires reboot
 * in this first prototype; CRC validation must not run inside the audio ISR. */
static void ork_felucca_prepare(void){
 uint32_t i;int valid=1;
 for(i=0;i<3;i++){
  const smp_user_hdr_t *h=(const smp_user_hdr_t *)smp_user_xip(i);
  if(h->magic!=SMP_USER_MAGIC||h->version!=1||h->nz!=1||h->name[0]!='O'||h->name[1]!='R'||h->name[2]!='K'||h->name[3]!='B'||h->name[4]!=(char)('1'+i))valid=0;
 }
 memset(&ork_device_bank,0,sizeof ork_device_bank);
 if(valid)(void)ork_bank_open(&ork_device_bank,ork_device_read,0,ORK_BANK_CAPACITY);
}
static ork_voice *ork_part_voice(track_t *t,voice_t *v){return &((ork_part_state *)ENG_MEM((uint32_t)(t-trk)))->voice[(uint32_t)(v-t->v)];}
static void ork_note_on_adapter(track_t *t,voice_t *v){
 ork_asset a;ork_voice *voice=ork_part_voice(t,v);uint32_t note=(uint32_t)(v->pitch16+8)/16u,word=(uint32_t)t->p[P_E0]%ORK_WORDS;
 if(note>127||!ork_bank_asset(&ork_device_bank,(uint8_t)word,(uint8_t)note,&a)){memset(voice,0,sizeof *voice);return;}
 ork_voice_start(voice,&ork_device_bank,&a,FS,v->vel,1);
}
static int ork_done_adapter(track_t *t,voice_t *v){return !ork_part_voice(t,v)->active;}
static void ork_render_adapter(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m){
 uint32_t i;ork_voice *voice=ork_part_voice(t,v);
 if(!v->gate)ork_voice_release(voice);
 for(i=0;i<n;i++)out[i]+=voice_amp(ork_voice_tick(voice,&ork_device_bank),m,i);
}
static const preset_t ORK_PRESETS[]={
 {"GRAH",{0,0,0,0,0,0,0,0},{0,0,127,0},0,0,FX(0,0,0,0),PAT(16)},
 {"BOM", {1,0,0,0,0,0,0,0},{0,0,127,0},0,0,FX(0,0,0,0),PAT(16)},
 {"DOOM",{2,0,0,0,0,0,0,0},{0,0,127,0},0,0,FX(0,0,0,0),PAT(16)},
 {"RUM", {3,0,0,0,0,0,0,0},{0,0,127,0},0,0,FX(0,0,0,0),PAT(16)}
};
static const engine_t ENG_ORK={
 .name="ORK",.page_title={"WORD","BANK"},
 .edit={{"WORD",F_ENUM,0,3,0,ork_words,0},{"--",F_INT,0,0,0,0,0},{"--",F_INT,0,0,0,0,0},{"--",F_INT,0,0,0,0,0},{"--",F_INT,0,0,0,0,0},{"--",F_INT,0,0,0,0,0},{"--",F_INT,0,0,0,0,0},{"--",F_INT,0,0,0,0,0}},
 .presets=ORK_PRESETS,.npresets=4,.poly=3,.sampled=1,.knob={P_E0,P_E0,P_E0,P_E0},.note_on=ork_note_on_adapter,.render=ork_render_adapter,.ownenv=1,.done=ork_done_adapter,.mem=sizeof(ork_part_state)
};
