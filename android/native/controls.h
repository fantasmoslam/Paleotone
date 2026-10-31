/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* The tablet's control table. Included after the firmware by both the engine
 * (which sets controls) and the display copy (which reads them for the UI). */
typedef struct {const char *label;int track;unsigned id;} pt_control;
/* Track >=0 is explicit: dragging a mixer fader cannot edit another selected track. */
static const pt_control controls[]={
 {"Track 1 level",0,P_LEVEL},{"Track 2 level",1,P_LEVEL},{"Track 3 level",2,P_LEVEL},{"Track 4 level",3,P_LEVEL},
 {"Track 1 mute",0,P_MUTE},{"Track 2 mute",1,P_MUTE},{"Track 3 mute",2,P_MUTE},{"Track 4 mute",3,P_MUTE},
 {"Track 1 pan",0,P_PAN},{"Track 2 pan",1,P_PAN},{"Track 3 pan",2,P_PAN},{"Track 4 pan",3,P_PAN},
 {"Tempo",-1,G_BPM},{"Habitat level",-1,G_SCAPE},{"Drone level",-1,G_SDRN},
 {"Day / night",-1,G_STIME},{"Distance",-1,G_SDIST},{"Music activity",-1,G_DDENS},
 {"Evolution",-1,G_DDRFT},{"Motion",-1,G_SMOTION},{"Creature call rate",-1,G_SCRATE},
 {"Danger",-1,G_SDNGR},{"Creature size",-1,G_SDSIZ},{"Evolving music",-1,G_DREAM},
 {"Hold harmony",-1,G_SCHOLD},{"Next harmony",-1,G_SCNEXT},{"New phrases",-1,G_SCNEW},
 {"Wind",-1,G_SL0},{"Water",-1,G_SL1},{"Rain",-1,G_SL2},{"Insects",-1,G_SL3},
 {"Frogs",-1,G_SL4},{"Birds",-1,G_SL5},{"Drips",-1,G_SL6},{"Rumble",-1,G_SL7},
 {"Track 1 role",-1,G_SROLE0},{"Track 2 role",-1,G_SROLE1},{"Track 3 role",-1,G_SROLE2},{"Track 4 role",-1,G_SROLE3},
 {"Biome",-1,G_SBIOME},{"Music mood",-1,G_DMOOD},{"Call track",-1,G_SCTRK},
 {"Pitch spread",-1,G_SCSPRD},{"Bed",-1,G_SBED},{"Melody",-1,G_SMELO},{"Bass",-1,G_SBASS},{"Pulse",-1,G_SPULSE}
};
#define PT_CONTROL_COUNT (sizeof controls/sizeof controls[0])
static void control_read(unsigned slot,int *value,int *min,int *max,char *text){
    if(slot>=PT_CONTROL_COUNT){*value=*min=*max=0;text[0]=0;return;}
    const pt_control *c=&controls[slot];
    const param_desc_t *d=c->track<0?&GP[c->id]:&TP[c->id];
    *value=c->track<0?song.g[c->id]:trk[c->track].p[c->id];*min=d->min;*max=d->max;
    uint8_t before=fmt_named;
    char formatted[64];const char *unit=0;param_format(d,*value,formatted,&unit);
    fmt_named=before;
    snprintf(text,64,"%s%s%s",formatted,unit&&*unit?" ":"",unit?unit:"");
}
