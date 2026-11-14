/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Optional native amplitude ADSR. With no override the original render path is untouched. */
typedef struct {float a,d,s,r; uint8_t enabled;} nc_env_cfg_t;
typedef struct {float level; uint32_t left; uint8_t stage;} nc_env_t;
static void nc_env_start(nc_env_t *v,uint32_t gate){v->stage=1;v->left=gate;}
static float nc_env_tick(nc_env_t *v,const nc_env_cfg_t *c,int gate){
    if(!gate && v->stage && v->stage!=3)v->stage=3;
    if(v->stage==1){v->level+=c->a;if(v->level>=1.0f){v->level=1.0f;v->stage=2;}}
    else if(v->stage==2)v->level+=(c->s-v->level)*c->d;
    else if(v->stage==3){v->level*=1.0f-c->r;if(v->level<.00003f){v->level=0;v->stage=0;}}
    return v->level;
}
static float nc_env_gated(nc_env_t *v,const nc_env_cfg_t *c){
    int gate=v->left>0;if(v->left)v->left--;return nc_env_tick(v,c,gate);
}
