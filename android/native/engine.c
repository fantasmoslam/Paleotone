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
