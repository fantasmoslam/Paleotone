/* SPDX-License-Identifier: GPL-3.0-only
 * PALEOTONE in the browser: the FM-1 firmware's own sources (the sound, the sequencer and the whole UI), run in an
 * AudioWorklet against a simulated FM-1. The design follows the earlier PALEOTONE / GHOULBOX browser build
 * (web/emu/felucca_web.c there, after X0X's web/emu/x0x_web.c by charlesvestal, GPL-3.0); the device doubles
 * follow tests/ui_pages_test.c, the host harness the UI tests run on:
 *   - the clock: fm1_ms advances with the audio the worklet asks for; mix_block (fx.c, the device's own mix) renders
 *     each block, and the UI (ui_input, ui_leds, ui_draw, as main.c's loop) runs every 15 ms of it, with ui_input
 *     every millisecond in between, as main.c's wait loop calls it;
 *   - the panel: buttons by their printed labels (panel.c maps them to the matrix), the keys as fm1_in.notes, the
 *     encoders by role; the LEDs read back from fm1_led / fm1_led_dim through the FM-1's key matrix;
 *   - the screen: lcd_fill / lcd_blit into a 240 x 240 RGB565 frame (the panel's byte order).
 * Not here: the flash (projects and user presets are not kept: SAVE says so), USB (MIDI comes from Web MIDI
 * instead), TRS MIDI, the serial console, the battery (full).
 *   web/emu/build.sh  ->  build/emu/paleotone.wasm (wasm32-wasi, clang; Emscripten works too) */
#include <stdint.h>

#define FELUCCA_ARRANGER 1
#define main hostsim_main
#include "../../tests/hostsim.c"
#undef main

/* PALEOTONE's words on the screens (firmware/src/felucca.c, the FM-1 build) */
#define BRAND_NAME "PALEOTONE"
#define BRAND_SAIL_X 148
#define BRAND_RESCUE "PALEOTONE USB RESCUE"
#define BRAND_URL "GITHUB.COM/FANTASMOSLAM"
#define BRAND_BASED "ON SLOOP + FELUCCA"
#define BRAND_PROMPT 1
#define VIS_LAST_NAMES "PALEO"
#define BRAND_CRASH "FELUCCA CRASH"
#define VIS_STYLES 12u
#define VIS_N_STR "/12"

/* ----------------------------------------------------------- the screen --- */
static uint16_t web_fb[240 * 240];                 /* RGB565 as the panel gets it (high byte first) */
static uint32_t web_draws;
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    uint32_t i, j;
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++)
            web_fb[(y + j) * 240u + x + i] = p[j * w + i];
    web_draws++;
}
#include "../../firmware/src/gfx.c"
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    uint16_t sw = swap16(c);
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++)
            web_fb[(y + j) * 240u + x + i] = sw;
    web_draws++;
}

/* ------------------------------------------------------------ the panel --- */
#define FM1_TICKS_PER_US 24u
static uint32_t fm1_ticks(void) { return fm1_ms * 1000u * FM1_TICKS_PER_US; }
static int32_t encs[8];
static int32_t fm1_enc_take(uint32_t e) { int32_t s = encs[e & 7u]; encs[e & 7u] = 0; return s; }
#define FM1_NCOL 11u
#define FM1_NBTN 14u
#define FM1_NENC 7u
static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {      /* as firmware/hal/fm1_input.h: key id at (column, row bit) */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};
static uint8_t fm1_led[16], fm1_led_dim[16], fm1_led_bg[16];
static volatile uint16_t fm1_led_bg_ns;
static void fm1_led_key(uint32_t id, int on)
{
    uint32_t p, r;
    for (p = 0; p < FM1_NCOL; p++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][p] == (int8_t)id) {
                if (on)
                    fm1_led[p] |= (uint8_t)(1u << r);
                else
                    fm1_led[p] &= (uint8_t)~(1u << r);
            }
}
static uint32_t edges_btn, notes_seen;
static uint32_t fm1_input_edges(int x) { uint32_t e = edges_btn; (void)x; edges_btn = 0; return e; }
static uint32_t fm1_input_note_edges(void) { uint32_t e = fm1_in.notes & ~notes_seen; notes_seen = fm1_in.notes; return e; }
static void fm1_wdt_feed(void) {}
static int32_t fm1_adc_read(int c) { (void)c; return -1; }
static struct { uint32_t magic, stage, page, home, ui_frames; } felucca_dbg;
#define FELUCCA_ICONS 1
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N], scope_bufr[SCOPE_N];
static uint32_t scope_w;
#include "../../firmware/src/panel.c"
#include "../../firmware/src/ui.c"

/* ---------------------------------------------------- what is not kept --- */
/* The browser keeps no flash: no project or user preset is listed, and SAVE says where the music can be kept. */
static int project_used(uint32_t i) { (void)i; return 0; }
static void project_save(uint32_t i) { (void)i; ui_message("NOT KEPT IN BROWSER"); }
static void project_load(uint32_t i) { (void)i; }
static void project_erase(uint32_t i) { (void)i; }
static void arrangement_save(void) {}
static uint32_t web_sec_bars[4];
static uint32_t arrangement_ready(void) { return 0; }
static void arrangement_apply(uint32_t s) { (void)s; }
static uint32_t section_bars(uint32_t s) { return web_sec_bars[s & 3u]; }
static void song_backup(void) {}
static void song_restore(void) {}
static void section_store(uint32_t s) { live_sec = (int8_t)s; }
static void section_load(uint32_t s) { live_sec = (int8_t)s; }
static int up_used(uint32_t k) { (void)k; return 0; }
static int up_load(uint32_t k) { (void)k; return -1; }
static int up_load_to(track_t *t, uint32_t k) { (void)t; (void)k; return -1; }
static int up_arp_of(uint32_t k, int16_t *a) { (void)k; (void)a; return 0; }
static uint32_t up_count(void) { return 0; }
static uint32_t up_nth(uint32_t n) { return n; }
static uint32_t up_rank(uint32_t s) { return s; }
static void up_name(uint32_t k, char *b) { (void)k; b[0] = 0; }
static void up_slot_label(char *b, uint32_t k) { fmt_int(b, (int32_t)k + 1); }
static void up_ui(uint32_t op, uint32_t k) { (void)op; (void)k; ui_message("NOT KEPT IN BROWSER"); }
static void settings_save(void) {}
#include "../../firmware/src/ui_song.c"
#include "../../firmware/src/ui_studio.c"
#include "../../firmware/src/icons.c"
#include "../../firmware/src/ui_draw.c"
#include "../../firmware/src/ui_vis.c"
#include "../../firmware/src/ui_layers.c"
#include "../../firmware/src/ui_menu.c"
#include "../../firmware/src/ui_input.c"
#include "../../firmware/src/splash.c"
static int st_read(uint32_t off, void *dst, uint32_t n) { (void)off, (void)dst, (void)n; return -1; }
static int st_erase(uint32_t off) { (void)off; return -1; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { (void)off, (void)src, (void)n; return -1; }
#include "../../firmware/src/storage.c"

/* --------------------------------------------------------- the device --- */
static uint32_t web_booted, web_boot_ms, web_last_frame, web_master_k10 = 700;

static void web_master_apply(void) { song.master_q12 = (web_master_k10 * web_master_k10) >> 8; }   /* main.c's law */

/* main.c felucca_init, without the flash: the power-on tracks, then (a first power-on) the demo song */
static void web_power_on(void)
{
    uint32_t i;
    panel = PANEL_DEFAULT;
    settings.palette = PAL_TERMINAL;
    palette_set(PAL_TERMINAL);
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    fm6_init();
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults(t);
        if (i < NPART) {
            set_engine_of(t, TRK_DEF[i][0]);
            apply_preset_to(t, TRK_DEF[i][1]);
            t->engine = t->eng_req;
        }
        track_defaults_steps(t);
    }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    song.sel = 0;
    web_master_apply();
    demo_fill(1, 3);
    layers_init();
    go_home();
    ui.force = 1;
    sloop_splash();
    web_boot_ms = fm1_ms + 1200u;                    /* the splash, as long as the FM-1 shows it */
    web_booted = 1;
}

/* one pass of main.c's loop */
static void web_frame(void)
{
    felucca_dbg.ui_frames++;
    ui_input();
    ui_leds();
    ui_draw();
}

/* output ring, stereo float, filled a block at a time, drained by web_render */
#define RING 4096u
static float ring[RING * 2];
static uint32_t ring_w, ring_r, web_ms_acc;

static void web_block(void)
{
    static int32_t o[CTL * 2];
    uint32_t i;
    mix_block(o, CTL);
    for (i = 0; i < CTL; i++) {
        uint32_t k = (ring_w++ % RING) * 2u;
        ring[k] = (float)o[2u * i] / 32768.0f;
        ring[k + 1] = (float)o[2u * i + 1u] / 32768.0f;
        if (i & 1u) {                                /* the scope, as audio.c fills it (the visualiser) */
            scope_bufr[scope_w & (SCOPE_N - 1u)] = vis_tap[2u * i + 1u];
            scope_buf[scope_w++ & (SCOPE_N - 1u)] = vis_tap[2u * i];
        }
    }
    web_ms_acc += CTL * 1000u;
    while (web_ms_acc >= FS) {                       /* a millisecond of the device */
        web_ms_acc -= FS;
        fm1_ms++;
        if (!web_booted || (int32_t)(fm1_ms - web_boot_ms) < 0)
            continue;
        if (web_boot_ms) {                           /* the splash goes, the loop starts */
            web_boot_ms = 0;
            ui.force = 1;
            web_last_frame = fm1_ms - 15u;
        }
        if (fm1_ms - web_last_frame >= 15u) {
            web_last_frame = fm1_ms;
            web_frame();
        } else {
            ui_input();
        }
    }
}

#define EXPORT __attribute__((used, visibility("default")))
#undef __attribute__

static float out_l[1024], out_r[1024];
EXPORT float *web_out_l(void) { return out_l; }
EXPORT float *web_out_r(void) { return out_r; }
/* n frames (<= 1024) into out_l / out_r: the device runs until they exist */
EXPORT void web_render(uint32_t n)
{
    uint32_t k;
    if (n > 1024u)
        n = 1024u;
    while (ring_w - ring_r < n)
        web_block();
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_r++ % RING) * 2u;
        out_l[k] = ring[i];
        out_r[k] = ring[i + 1];
    }
}
EXPORT void web_boot(void)
{
    if (!web_booted)
        web_power_on();
}

/* input, by the printed labels: buttons bit b = B_FX .. B_OCTUP (panel.c), keys bit k = key k (F3 .. G5),
 * encoders by role (EN_SELECT .. EN_K4), MASTER 0..1023 */
static uint32_t web_key_mask;
EXPORT void web_buttons(uint32_t labels)
{
    uint32_t m = 0, b, old = fm1_in.buttons;
    for (b = 0; b < NB; b++)
        if ((labels >> b) & 1u)
            m |= 1u << panel.btn[b];
    edges_btn |= m & ~old;
    fm1_in.buttons = m;
}
EXPORT void web_keys(uint32_t mask)
{
    mask &= (1u << 27) - 1u;
    web_key_mask = mask;
    fm1_in.notes = mask;
}
EXPORT void web_enc(uint32_t role, int32_t n)
{
    if (role < NE)
        encs[panel.enc[role] & 7u] += n * panel.dir[role];
}
EXPORT void web_master(uint32_t v)
{
    web_master_k10 = v > 1023u ? 1023u : v;
    web_master_apply();
}
/* a channel message from Web MIDI, as a USB-MIDI packet arrives (usb.c midi_in_event): 0 = the ring was full */
EXPORT uint32_t web_midi(uint32_t status, uint32_t d1, uint32_t d2)
{
    if (status < 0x80u || status >= 0xF0u)
        return 1;
    if (mi_w - mi_r >= MQ)
        return 0;
    midi_in_event((status >> 4) | status << 8 | (d1 & 127u) << 16 | (d2 & 127u) << 24);
    return 1;
}

/* the LEDs: bit b = the button labelled b lit, keys bit k; the dim glow alike */
static uint32_t led_on(const uint8_t *l, uint32_t id)
{
    uint32_t c, r;
    for (c = 0; c < FM1_NCOL; c++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][c] == (int8_t)id)
                return (l[c] >> r) & 1u;
    return 0;
}
static uint32_t led_buttons(const uint8_t *l)
{
    uint32_t b, m = 0;
    for (b = 0; b < NB; b++)
        m |= led_on(l, panel.btn[b]) << b;
    return m;
}
static uint32_t led_keys(const uint8_t *l)
{
    uint32_t k, m = 0;
    for (k = 0; k < 27u; k++)
        m |= led_on(l, 14u + k) << k;
    return m;
}
EXPORT uint32_t web_lit_buttons(void) { return led_buttons(fm1_led); }
EXPORT uint32_t web_lit_keys(void) { return led_keys(fm1_led); }
EXPORT uint32_t web_dim_buttons(void) { return led_buttons(fm1_led_dim); }
EXPORT uint32_t web_dim_keys(void) { return led_keys(fm1_led_dim); }
EXPORT uint32_t web_dim_level(void) { return 0; }
EXPORT uint32_t web_breath_buttons(void) { return 0; }
EXPORT uint32_t web_breath_keys(void) { return 0; }
EXPORT uint32_t web_mid_keys(void) { return 0; }

EXPORT uint16_t *web_screen(void) { return web_fb; }
EXPORT uint32_t web_screen_draws(void) { return web_draws; }
EXPORT uint32_t web_now_ms(void) { return fm1_ms; }
EXPORT uint32_t web_playing(void) { return song.playing; }
EXPORT uint32_t web_sample_rate(void) { return FS; }
