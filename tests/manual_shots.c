/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX: the screens of the user manual, drawn by the firmware's own UI code (tests/ui_test.c's stubs), in the
 * TERMINAL palette, from the HERD scene playing: HOME, the pages, SCAPE, the engines' EDIT pages, the menu.
 *   build/host/manual_shots OUTDIR        -> OUTDIR/<name>.ppm (240 x 240)
 * Not a test. */
#define UI_RENDER_NO_MAIN
#include "ui_render.c"

static const char *outdir;
static void shot(const char *name)
{
    char path[512];
    uint32_t i;
    FILE *f;
    ui.force = 1;
    frame();
    snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    if (!(f = fopen(path, "wb")))
        return;
    fprintf(f, "P6\n240 240\n255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = swap16(host_screen[i]);
        uint8_t b[3] = {(uint8_t)((c >> 11) * 255u / 31u), (uint8_t)(((c >> 5) & 63u) * 255u / 63u),
                        (uint8_t)((c & 31u) * 255u / 31u)};
        fwrite(b, 1, 3, f);
    }
    fclose(f);
    printf("  %s\n", name);
}
/* ms of music and UI: the audio as the device plays it, the scope fed, a frame every 16 ms */
static void live(uint32_t ms)
{
    int32_t o[2 * CTL];
    uint32_t t, i, n = ms / 16u;
    while (n--) {
        for (t = 0; t < FS * 16u / 1000u; t += CTL) {
            mix_block(o, CTL);
            for (i = 0; i < CTL; i++)
                scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)clamp(o[2 * i], -32768, 32767);
        }
        fm1_ms += 16u;
        host_ticks += 16000u;
        ui_input();
        ui_draw();
    }
}
static void sel(uint32_t k)                         /* the selected track (ALGORITHM) */
{
    song.sel = (uint8_t)k;
    ui.force = 1;
}
static void page(const char *title)
{
    go_title(title);
    frame();
}

int main(int argc, char **argv)
{
    uint32_t i;
    if (argc < 2)
        return 1;
    outdir = argv[1];
    rep = fopen("/dev/null", "w");
    ui_power_on();
    settings.palette = 0;
    for (i = 0; i < NPALETTES; i++)
        if (!strcmp(UI_PALETTES[i].name, "TERMINAL"))
            settings.palette = i;
    palette_set(settings.palette);
    gb_scene_load(GB_SCENE_HERD);
    song.master_q12 = 1600;
    usb.config = 0;
    song.batt_raw = 600;
    ui.home = 1;
    press(B_PLAY);
    live(2400);
    shot("home_herd");                              /* HOME, track 1 (ROAR STAMPEDE) */
    sel(1);
    live(800);
    shot("home_saurus");                            /* track 2: SAURUS BRACHIO CALL */
    sel(0);
    press(B_HOME);
    live(400);
    shot("mixer");
    press(B_HOME);
    shot("clock");
    press(B_HOME);
    shot("keys");
    press(B_HOME);
    shot("scape_music");
    press(B_HOME);                                  /* SCAPE: on, JUNGLE */
    turn(EN_K1, 100);
    live(1600);
    shot("scape");
    press(B_HOME);
    turn(EN_K1, 90);
    shot("scape_drone");
    press(B_HOME);
    turn(EN_K1, 2);
    turn(EN_K2, 40);
    shot("scape_life");
    press(B_HOME);
    shot("scape_levels");
    press(B_HOME);
    shot("scape_levels2");
    press(B_HOME);                                  /* HOME again */
    song.g[G_SCAPE] = 0;
    song.g[G_SCTRK] = 0;
    sel(0);
    page("EDIT 1");
    shot("edit_roar");
    page("EDIT 2");
    shot("edit_roar2");
    sel(1);
    page("EDIT 1");
    shot("edit_saurus");
    page("EDIT 2");
    shot("edit_saurus2");
    sel(2);
    page("EDIT 1");
    shot("edit_fm6_mammoth");
    sel(3);
    page("EDIT 1");
    shot("edit_voice_chant");
    sel(2);
    page("STEP");
    live(600);
    shot("step_mammoth");
    sel(1);
    page("ENV");
    shot("env");
    page("LFO");
    shot("lfo");
    page("FX");
    shot("fx");
    page("REVERB");
    shot("reverb");
    page("TAPE");
    shot("tape");
    page("SCL");
    shot("scl");
    page("ARP");
    shot("arp");
    page("VOICE");
    shot("voice");
    page("SONG");
    shot("song");
    page("PROJECT");
    shot("project");
    page("PRESETS");
    shot("presets");
    page("PHRASES");
    shot("phrases");
    page("AUTOMATION");
    shot("automation");
    ui.home = 1;
    frame();
    hold(B_HOME);                                   /* the MENU */
    shot("menu");
    printf("done\n");
    return 0;
}
