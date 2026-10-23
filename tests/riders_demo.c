/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX: the RIDERS scene (engines.c GB_SCENE_RIDERS, firmware/src/riders.c) as the FM-1 runs it: the scene loaded,
 * PLAY pressed on HOME, the song played to its end (the transport stops by itself). Checks the sounds, that the
 * saved projects and the user's SONG chain are untouched, that a second PLAY plays it again and that an edited
 * pattern plays as the user's instead.
 *   build/host/riders_demo [OUT.wav]          exit 0: ALL PASSED */
#define UI_RENDER_NO_MAIN
#include "ui_render.c"

static int bad;
static void ck(const char *what, int ok)
{
    printf("  %-4s %s\n", ok ? "ok" : "FAIL", what);
    bad += !ok;
}
static void press_play(void)
{
    ui.home = 1;
    press(B_PLAY);
}

int main(int argc, char **argv)
{
    static const char *const WANT[NTRK] = {"PRIMEVAL", "STAMPEDE", "TAR PIT BASS", "WAR DRUMS"};
    uint32_t i, t, frames = 0, maxf = (uint32_t)(150.0 * FS), sections = 0, last_row = 0;
    int32_t o[2 * CTL];
    chain_config_t before;
    FILE *f = 0;
    rep = fopen("/dev/null", "w");
    ui_power_on();
    chain_defaults(&chain_config);
    chain_config.count = 1;
    chain_config.row[0].repeat = 3;                       /* a user's song chain: must stay as it is */
    before = chain_config;
    gb_scene_load(GB_SCENE_RIDERS);
    song.batt_raw = 600;
    usb.config = 0;
    song.master_q12 = 1600;                               /* (the MASTER knob: 0 in the host; as tests/song_score.c) */
    if (getenv("SOLO"))                                   /* SOLO=k: track k only (comparisons) */
        for (i = 0; i < NTRK; i++)
            if (i != (uint32_t)atoi(getenv("SOLO")))
                trk[i].p[P_LEVEL] = 0;
    for (i = 0; i < NTRK; i++)
        ck(WANT[i], !strcmp(ENGINES[trk[i].engine]->presets[trk[i].preset].name, WANT[i]));
    ck("84 BPM, lead 12 dB under", song.g[G_BPM] == 84 && trk[0].p[P_LEVEL] == 80 && trk[1].p[P_LEVEL] == 104);
    ck("the menu lists RIDERS", !strcmp(GB_SCENES[GB_SCENE_RIDERS].name, "RIDERS"));
    press_play();
    ck("PLAY starts the song", chain.armed && transport_req == 1u);
    if (argc > 1)
        f = fopen(argv[1], "wb");
    if (f)
        wav_hdr(f, (uint32_t)(141.1 * FS));
    for (t = 0; t < maxf; t += CTL) {
        mix_block(o, CTL);
        if (f && t < (uint32_t)(141.1 * FS))
            for (i = 0; i < CTL; i++)
                wav_put(f, o[2 * i], o[2 * i + 1]);
        if (chain.running && chain.row != last_row) {
            last_row = chain.row;
            sections++;
        }
        if (t > FS && !song.playing && !chain.running && !frames)
            frames = t;
    }
    if (f)
        fclose(f);
    printf("  song ended at %.1f s after %u section changes, voices given up %u\n", frames / (double)FS, sections,
           voice_kills);
    ck("the song runs its 7 sections, 48 bars, then stops", sections == 6u && frames > 136u * FS && frames < 139u * FS);
    ck("the user's SONG chain untouched", !memcmp(&before, &chain_config, sizeof before));
    ck("no saved project slot written", !project_used(0) && !project_used(1) && !project_used(2) && !project_used(3));
    press_play();
    ck("a second PLAY plays it again", chain.armed);
    for (t = 0; t < FS / 2u; t += CTL)
        mix_block(o, CTL);
    press(B_PLAY);                                   /* stop */
    for (t = 0; t < FS / 2u; t += CTL)
        mix_block(o, CTL);
    trk[0].step[3].time = ST_NOTE;                         /* an edit: the theme is the user's now */
    trk[0].step[3].n = 1;
    trk[0].step[3].note[0] = 60;
    press_play();
    ck("an edited pattern plays as the user's, not the song", !chain.armed && transport_req == 1u);
    printf(bad ? "RIDERS DEMO: %d FAILED\n" : "RIDERS DEMO: ALL PASSED\n", bad);
    return bad != 0;
}
