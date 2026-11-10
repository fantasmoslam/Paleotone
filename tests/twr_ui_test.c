/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS's screen and controls (firmware/src/twr_ui.c) on the UI test's harness, the audio running between frames:
 * boot on its screen, PLAY, the arc moving, PRESETS loading every preset (its sounds found, its key on the parts),
 * the knobs, HOME to Paleotone's pages and back, a sound's page from its screen. Writes DIR/twr-*.ppm.
 *   twr_ui_test DIR */
#define TWRECKS 1
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include "../firmware/src/twr_ui.c"
static void turn(uint32_t role, int32_t n) { encs[panel.enc[role]] += n * panel.dir[role]; frames(3); }
int main(int argc, char **argv)
{
    uint32_t i, k, e, pk, d0, ok;
    outdir = argc > 1 ? argv[1] : "build/host";
    panel = PANEL_DEFAULT;
    settings.palette = PAL_TERMINAL;
    palette_set(PAL_TERMINAL);
    for (i = 0; i < G_COUNT; i++) song.g[i] = GP[i].def;
    fm6_init();
    for (i = 0; i < NTRK; i++) {                    /* as felucca_init */
        track_defaults(&trk[i]);
        if (i < NPART) { set_engine_of(&trk[i], TRK_DEF[i][0]); apply_preset_to(&trk[i], TRK_DEF[i][1]); trk[i].engine = trk[i].eng_req; }
        track_defaults_steps(&trk[i]);
    }
    song.master_q12 = 2048;
    layers_init();
    go_home();
    twr_boot();
    frames(4);
    check(twr_scr && twr.p == &TWR_PRESETS[0] && song.sel == TW_LEAD, "boot: T-WRECKS's screen, the first preset, the keys on the LEAD");
    for (k = 0, ok = 1; k < TWR_NPRESETS; k++)      /* every preset's sounds exist (ui.c BANK's names) */
        for (i = 0; i < TW_LAYERS; i++)
            if (twr_find(TWR_PRESETS[k].snd[i], &e, &pk)) {
                printf("ui: %s: no sound %s\n", TWR_PRESETS[k].name, TWR_PRESETS[k].snd[i]);
                ok = 0;
            }
    check(ok, "every preset's five sounds are Paleotone's");
    ppm("twr-stopped");
    {   /* the keys, stopped: the LEAD sounds */
        uint32_t v, on = 0;
        fm1_in.notes |= 1u << 12;
        frames(10);
        for (v = 0; v < NVOICE; v++)
            on += trk[TW_LEAD].v[v].active;
        fm1_in.notes &= ~(1u << 12);
        frames(10);
        check(on > 0, "a white key, stopped: the LEAD sounds");
    }
    tap(B_PLAY);
    frames(300);                                    /* ~5 s */
    check(song.playing && twr_cnt[TW_DRONE] > 0, "PLAY: the conductor plays (the drone)");
    ppm("twr-playing");
    turn(EN_PRESET, 1);
    frames(200);
    check(twr_cur == 1u && twr.p == &TWR_PRESETS[1] && song.g[G_BPM] == TWR_PRESETS[1].bpm, "PRESETS: the next preset, its tempo");
    check(trk[0].p[P_ROOT] == TWR_PRESETS[1].root && trk[TW_LEAD].p[P_SCALE] == TWR_PRESETS[1].scale &&
          trk[TW_LEAD].p[P_QUANT] == 2, "the preset's key and mode on the parts, the LEAD's white keys in it");
    ppm("twr-preset2");
    turn(EN_PRESET, -2);
    frames(4);
    check(twr_cur == TWR_NPRESETS - 1u, "PRESETS back past the first: the last");
    d0 = twr.k[1];
    turn(EN_K1 + 1, -40);
    check(twr.k[1] < d0, "KNOB 2: DRUMS down");
    turn(EN_K1 + 3, 10);
    check(song.g[G_AMB] == twr.k[3], "KNOB 4: the atmosphere (AMB)");
    {   /* ALGORITHM: a layer; PRESETS swaps its sound; REC shuffles */
        uint32_t c0 = twr_cur, b0, kind, ok2 = 1, l;
        turn(EN_ALGO, 2);                           /* PRESET -> DRONE -> OST */
        check(twr_focus == 2u && song.sel == TW_OST, "ALGORITHM: the OST layer, the keys on it");
        b0 = twr_bank_of(&trk[TW_OST]);
        turn(EN_PRESET, 1);
        check(twr_cur == c0 && twr_bank_of(&trk[TW_OST]) == (b0 + 1u) % NBANK && trk[TW_OST].p[P_ROOT] == TWR_PRESETS[c0].root,
              "PRESETS on a layer: its next sound, the preset and its key stay");
        ppm("twr-swap");
        turn(EN_ALGO, 4);
        check(twr_focus == 6u && song.sel == TRK_DRUM, "ALGORITHM to DRUMS");
        b0 = (uint32_t)TDRUM->p[P_E0];
        turn(EN_PRESET, 1);
        check((uint32_t)TDRUM->p[P_E0] == (b0 + 1u) % DRUM_KITS, "PRESETS on DRUMS: the next kit");
        turn(EN_ALGO, -9);
        check(twr_focus == 0u && song.sel == TW_LEAD, "ALGORITHM back to PRESET: the keys on the LEAD");
        {
            uint32_t was[TW_LAYERS];
            for (l = 0; l < TW_LAYERS; l++)
                was[l] = twr_bank_of(&trk[l]);
            tap(B_REC);
            frames(3);
            for (l = 0; l < TW_LAYERS; l++) {
                uint32_t now = twr_bank_of(&trk[l]);
                kind = was[l] < NBANK ? BANK[was[l]].kind : 99u;
                if (now >= NBANK || BANK[now].kind != kind || trk[l].p[P_SCALE] != TWR_PRESETS[twr_cur].scale) {
                    printf("ui: shuffle: layer %u: %s -> %s\n", l, was[l] < NBANK ? BANK[was[l]].name : "?",
                           now < NBANK ? BANK[now].name : "?");
                    ok2 = 0;
                }
            }
            check(ok2, "REC: SHUFFLE, every layer a sound of the same kind, in the preset's mode");
            ppm("twr-shuffled");
        }
    }
    tap(B_HOME);
    frames(30);
    check(!twr_scr, "HOME tapped: Paleotone's pages");
    ppm("twr-paleotone");
    tap(B_HOME);
    frames(30);
    check(twr_scr, "HOME tapped there: back");
    tap(B_ENV);
    frames(30);
    check(!twr_scr && !ui.home, "ENV from its screen: the LEAD's envelope page");
    tap(B_HOME);
    frames(30);
    check(twr_scr, "HOME: back again");
    ppm("twr-back");
    tap(B_PLAY);
    frames(30);
    check(!song.playing, "PLAY again: stopped");
    printf("ui: %s\n", fails ? "FAIL" : "all ok");
    return fails != 0;
}
