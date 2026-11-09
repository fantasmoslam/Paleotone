/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK-37 Pro FADER BANK screen (firmware/src/ui_fbank.c) on the UI harness: banks wrap, what each fader sets,
 * closes by itself; renders DIR/fbank-a.ppm, fbank-a-drums.ppm, fbank-b.ppm (RAPTOR: A ENV, no LEVEL bank). */
#ifndef TWRECKS
#define TWRECKS 0      /* (Paleotone's pages as they are: T-WRECKS's screen is tests/twr_ui_test.c) */
#endif
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static uint8_t smk_fbank, smk_fbank_new;           /* (ui_input.c, SMK build) */
static char knob_digit(uint32_t role) { return role == EN_SELECT ? '2' : '5'; }   /* (panel.c, SMK build: KNOB LAYOUT FM-1) */
#include "../firmware/src/ui_fbank.c"

static void fb_shot(const char *name)
{
    ui.frame++;
    fb_draw();
    ppm(name);
}

int main(int argc, char **argv)
{
    uint32_t i;
    int16_t *vp;
    outdir = argc > 1 ? argv[1] : "build/host";
    panel = PANEL_DEFAULT;
    layers_init();
    settings.palette = 4;
    palette_set(4);
    host_tracks_init();
    for (i = 0; i < NPART; i++) { set_engine_of(&trk[i], TRK_DEF[i][0]); apply_preset_to(&trk[i], TRK_DEF[i][1]); trk[i].engine = trk[i].eng_req; }
    go_home(); ui.force = 1; frame();

    check(smk_fbank == 0 && FB_N == 4u, "power-on: bank A, ENV (RAPTOR: four banks, no LEVEL)");
    fb_set(smk_fbank + 1u);
    check(fb_shown() && smk_fbank == 1 && smk_fbank_new, "FADER BANK: the next bank, its screen up");
    fb_set(smk_fbank + FB_N - 1u);
    check(smk_fbank == 0, "SELECT back: bank A");
    fb_set(3u + 1u);
    check(smk_fbank == 0, "D -> A wraps");
    fb_set(0u + FB_N - 1u);
    check(smk_fbank == 3, "A back -> D");
    song.sel = 1;
    check(fb_param(0, 0, &vp) == &TP[P_ATK] && vp == &trk[1].p[P_ATK] && fb_param(0, 3, &vp) == &TP[P_REL] &&
          vp == &trk[1].p[P_REL], "bank A: the selected synth track's ATTACK .. RELEASE");
    {   /* no bank sets a level: every fader, every bank, all the way, on every track selected */
        int16_t lv[NTRK], dl = song.g[G_DRLVL];
        uint32_t b, f, v, k, same = 1;
        for (k = 0; k < NTRK; k++)
            lv[k] = trk[k].p[P_LEVEL];
        for (k = 0; k < NTRK; k++)
            for (b = 0; b < FB_N; b++)
                for (f = 0; f < 4u; f++)
                    for (v = 0; v < 128u; v += 9u) {
                        song.sel = (uint8_t)k;
                        fb_move(b, f, v);
                        if (fb_param(b, f, &vp) && (vp == &trk[k].p[P_LEVEL] || vp == &song.g[G_DRLVL]))
                            same = 0;
                    }
        for (k = 0; k < NTRK; k++)
            same &= trk[k].p[P_LEVEL] == lv[k];
        check(same && song.g[G_DRLVL] == dl, "no fader sets a track's or the drums' level");
        song.sel = 1;
        fb_move(0, 0, 0);
        check(trk[1].p[P_ATK] == TP[P_ATK].min, "fader 1 in bank A: ATTACK");
    }
    ui.force = 1; fb_shot("fbank-a");
    song.sel = TRK_DRUM;
    check(!fb_param(0, 0, &vp) && !fb_param(0, 3, &vp), "bank A, the drum track selected: nothing");
    ui.force = 1; fb_shot("fbank-a-drums");
    song.sel = 0;
    check(fb_param(1, TRK_DRUM, &vp) == &TP[P_TFLT] && vp == &TDRUM->p[P_TFLT], "bank B, fader 4: the drum track's filter");
    check(fb_param(2, 0, &vp) == &TP[P_REV] && vp == &trk[0].p[P_REV], "bank C, fader 1: track 1's reverb");
    trk[1].p[P_TFLT] = -30; trk[2].p[P_TFLT] = 20;
    fb_set(1); ui.force = 1; fb_shot("fbank-b");
    ui.layer = LY_STEP;
    check(!fb_shown(), "hidden while a layer is held");
    ui.layer = LY_PLAY;
    fm1_ms += 60000u;
    check(fb_shown(), "stays up until FADER BANK / PATCH");
    fb_set(3); fb_set(smk_fbank + 1u);
    check(smk_fbank == 0, "K2 turns the bank round (D -> A)");
    fb_close();
    check(!fb_shown(), "closed");
    printf("fbank: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
