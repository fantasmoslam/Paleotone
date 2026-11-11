/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/app.h"
#include "../../firmware/src/app/project.c"
static uint8_t saved[OBJ_NOBJ][3840];static int lens[OBJ_NOBJ], fail_obj=-1, fails;
int plat_store_load(uint32_t o,void *p,uint32_t n){int k=lens[o];if(k<=0)return -1;if(k>(int)n)k=(int)n;memcpy(p,saved[o],(size_t)k);return k;}
int plat_store_save(uint32_t o,const void *p,uint32_t n){if((int)o==fail_obj)return -1;if(n>3840)return -1;memcpy(saved[o],p,n);lens[o]=(int)n;return 0;}
void en_midi_out(uint32_t s,uint32_t a,uint32_t b){(void)s;(void)a;(void)b;}
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);fails++;}}while(0)
/* Frozen format-10 wire layout, from the installed b7ccd84 predecessor. */
typedef struct {int16_t par[131];uint16_t pool;uint8_t pings[82],pings_ok,screen;} legacy_slot;
typedef struct {uint32_t magic,format;int16_t par[131];uint16_t pool;uint8_t leds,piece,screen,rsv0;legacy_slot slot[10];uint8_t rsv[16];} legacy_project;
int main(void){
    legacy_project old;slot_t preserved[20];int i,k;int16_t par[P_NPARAMS];uint16_t pool;
    _Static_assert(sizeof(legacy_project)==3772,"old format size");
    en_init();memset(&old,0,sizeof old);old.magic=PROJ_MAGIC;old.format=10;
    memcpy(saved[OBJ_PROJ],&old,sizeof old);lens[OBJ_PROJ]=sizeof old;
    CHECK(project_load()<0);CHECK(proj.format==11);CHECK(proj.piece==0);
    CHECK(!strcmp(en_piece_name(0),"EXTINCTION"));CHECK(NPIECES==20);
    proj.par[P_CHORD]=55;proj.slot[4].par[P_SEQLEN]=12;
    for(i=1;i<NPIECES;i++)proj.slot[i].par[P_SKLEVEL]=(int16_t)(i*3%101);
    for(i=0;i<NPIECES;i++){proj.piece=(uint8_t)i;project_drum_pattern_set(i%EN_NBEAT);}proj.piece=0;
    CHECK(project_save()==0);memset(&proj,0,sizeof proj);CHECK(project_load()==0);
    for(i=0;i<NPIECES;i++){proj.piece=(uint8_t)i;CHECK(project_drum_pattern()==i%EN_NBEAT);}proj.piece=0;
    {int n=lens[OBJ_PROJ];lens[OBJ_PROJ]=sizeof(project_head_t);CHECK(project_load()==0);CHECK(proj.par[P_CHORD]==55);for(i=0;i<NPIECES;i++){proj.piece=(uint8_t)i;CHECK(project_drum_pattern()==en_piece_beat(i));}lens[OBJ_PROJ]=n;CHECK(project_load()==0);}
    CHECK(proj.par[P_CHORD]==55);CHECK(proj.slot[4].par[P_SEQLEN]==12);
    for(i=1;i<NPIECES;i++)if(!en_piece_replaced(i))CHECK(proj.slot[i].par[P_SKLEVEL]==i*3%101);
    lens[OBJ_BANK0+1]=0;CHECK(project_load()==0);
    CHECK(proj.slot[4].par[P_SEQLEN]==12);
    for(i=10;i<20;i++){en_piece(i,par,&pool);for(k=0;k<P_NPARAMS;k++)CHECK(proj.slot[i].par[k]==par[k]);}
    CHECK(project_save()==0);en_piece(15,par,&pool);
    fail_obj=OBJ_BANK0+1;proj.slot[15].par[P_SKLEVEL]=40;CHECK(project_save()<0);
    fail_obj=-1;CHECK(project_load()==0);CHECK(proj.slot[15].par[P_SKLEVEL]==par[P_SKLEVEL]);
    /* Old revision refreshes exactly six slots, preserving RAPTOR NIGHT edits. */
    project_defaults();project_piece(2);proj.par[P_TEMPO]=130;proj.par[P_LVOICE]=3;
    project_piece(3);proj.par[P_TEMPO]=99;proj.slot[12].par[P_TEMPO]=99;project_drum_pattern_set(13);
    for(i=0;i<NPIECES;i++)if(!en_piece_replaced(i)){proj.piece=(uint8_t)i;project_drum_pattern_set(i%EN_NBEAT);}
    proj.piece=3;
    CHECK(project_save()==0);memcpy(preserved,proj.slot,sizeof preserved);
    {saved_head_t ext;memcpy(&ext,saved[OBJ_PROJ],sizeof ext);ext.bank_rev=0;memcpy(saved[OBJ_PROJ],&ext,sizeof ext);}
    fail_obj=OBJ_BANK0+1;CHECK(project_load()<0);
    {saved_head_t ext;memcpy(&ext,saved[OBJ_PROJ],sizeof ext);CHECK(ext.bank_rev==0);}
    fail_obj=-1;
    CHECK(project_load()==0);CHECK(proj.par[P_TEMPO]==124);CHECK(project_drum_pattern()==11);
    CHECK(proj.slot[2].par[P_TEMPO]==130);CHECK(proj.slot[2].par[P_LVOICE]==3);
    for(i=0;i<NPIECES;i++)if(!en_piece_replaced(i)){CHECK(!memcmp(&proj.slot[i],&preserved[i],sizeof(slot_t)));proj.piece=(uint8_t)i;CHECK(project_drum_pattern()==i%EN_NBEAT);}
    proj.piece=3;
    for(i=0;i<NPIECES;i++)if(en_piece_replaced(i)){en_piece(i,par,&pool);CHECK(!memcmp(proj.slot[i].par,par,sizeof par));}
    proj.par[P_TEMPO]=125;project_drum_pattern_set(13);CHECK(project_save()==0);
    CHECK(project_load()==0);CHECK(proj.par[P_TEMPO]==125);CHECK(project_drum_pattern()==13);
    printf("clean bank and saved-data recovery: %s\n",fails?"FAIL":"all passed");return fails!=0;
}
