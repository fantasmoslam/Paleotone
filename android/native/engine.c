/* SPDX-License-Identifier: GPL-3.0-only
 * Native platform adapter for the existing portable firmware build.
 * Engine functions have exactly one owner: the audio callback while running,
 * the lifecycle thread only after the stream is closed. No JNI here.
 */
#include "engine.h"
#include <string.h>
#include "../../web/emu/felucca_web.c"

static float cache_l[128], cache_r[128];
static unsigned cache_pos = 128;
static float a[2], b[2];
static double phase;
static unsigned primed;
static void next(float *v) {
    if (cache_pos == 128) {
        web_render(128);
        memcpy(cache_l, web_out_l(), sizeof cache_l);
        memcpy(cache_r, web_out_r(), sizeof cache_r);
        cache_pos = 0;
    }
    v[0] = cache_l[cache_pos]; v[1] = cache_r[cache_pos++];
}
void pt_boot(const uint8_t *saved, size_t size) {
    web_nor_erase();
    size_t nor_size = web_nor_size(), project_size = web_project_size();
    int valid = saved && size == 16 + nor_size + project_size &&
        !memcmp(saved, "PTANDROID001", 12);
    if (valid) memcpy(web_nor(), saved + 16, nor_size);
    web_boot();
    /* Finish the firmware splash before the first output callback. */
    for (unsigned i = 0; i < 180; ++i) web_render(128);
    if (valid && project_size <= web_project_import_capacity()) {
        memcpy(web_project_import_buffer(), saved + 16 + nor_size, project_size);
        web_project_restore((uint32_t)project_size);
    }
    cache_pos = 128; phase = 0; primed = 0;
}
void pt_render(float *out, unsigned frames, unsigned rate) {
    if (!primed) { next(a); next(b); primed = 1; }
    const double step = (double)web_sample_rate() / rate;
    for (unsigned i = 0; i < frames; ++i) {
        out[i*2] = a[0] + (b[0]-a[0]) * (float)phase;
        out[i*2+1] = a[1] + (b[1]-a[1]) * (float)phase;
        phase += step;
        while (phase >= 1.0) {
            a[0]=b[0]; a[1]=b[1]; next(b); phase-=1.0;
        }
    }
}
void pt_keys(uint32_t mask) { web_keys(mask); }
void pt_buttons(uint32_t mask) { web_buttons(mask); }
void pt_encoder(unsigned role, int steps) { web_enc(role, steps); }
void pt_master(unsigned value) { web_master(value); }
void pt_midi(unsigned status, unsigned d1, unsigned d2) { web_midi(status, d1, d2); }
void pt_panic(void) {
    web_keys(0); web_buttons(0);
    /* Direct voice kill also covers a full MIDI queue or detached controller. */
    for (unsigned k = 0; k < NTRK; ++k) {
        trk_all_off(&trk[k]);
    }
}
unsigned pt_playing(void) { return web_playing(); }
void pt_screen(uint32_t *argb) {
    const uint16_t *pixels = web_screen();
    for (unsigned i = 0; i < 240*240; ++i) {
        unsigned c = (pixels[i] >> 8) | ((pixels[i] & 255) << 8);
        unsigned r=(c>>11)&31, g=(c>>5)&63, b=c&31;
        argb[i] = 0xff000000u | ((r<<3 | r>>2)<<16) |
            ((g<<2 | g>>4)<<8) | (b<<3 | b>>2);
    }
}
size_t pt_save(uint8_t *out, size_t cap) {
    size_t n = web_nor_size(), p = web_project_size();
    if (cap < 16+n+p) return 0;
    uint8_t *project = web_project_capture();
    if (!project) return 0;
    memset(out, 0, 16); memcpy(out, "PTANDROID001", 12);
    memcpy(out+16, web_nor(), n); memcpy(out+16+n, project, p);
    return 16+n+p;
}

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
unsigned pt_control_count(void){return sizeof controls/sizeof controls[0];}
const char *pt_control_label(unsigned slot){return slot<pt_control_count()?controls[slot].label:"";}
void pt_control_read(unsigned slot,int *value,int *min,int *max,char *text){
    if(slot>=pt_control_count()){*value=*min=*max=0;text[0]=0;return;}
    const pt_control *c=&controls[slot];
    const param_desc_t *d=c->track<0?&GP[c->id]:&TP[c->id];
    *value=c->track<0?song.g[c->id]:trk[c->track].p[c->id];*min=d->min;*max=d->max;
    uint8_t before=fmt_named;
    char formatted[64];const char *unit=0;param_format(d,*value,formatted,&unit);
    fmt_named=before;
    snprintf(text,64,"%s%s%s",formatted,unit&&*unit?" ":"",unit?unit:"");
}
void pt_control_set(unsigned slot,int value){
    if(slot==pt_control_count()){transport_req=value?1:2;return;}
    if(slot>=pt_control_count())return;
    const pt_control *c=&controls[slot];
    const param_desc_t *d=c->track<0?&GP[c->id]:&TP[c->id];
    value=param_fit(d,value);
    if(c->track<0){
        if(c->id==G_SBIOME&&value!=song.g[c->id])scape_biome((unsigned)value);
        song.g[c->id]=(int16_t)value;
    }else{trk[c->track].p[c->id]=(int16_t)value;motion_capture(&trk[c->track],c->id,(int16_t)value);}
    ui.force=1;
}
