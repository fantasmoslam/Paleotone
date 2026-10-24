/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX SCAPE (firmware/src/scape.c): its pages and knobs as the FM-1 has them (HOME > .. > EPOCH > SCAPE), a BIOME
 * turned setting the layers, the sound on and off (off: nothing runs), and its settings kept beside a project (the
 * settings' sidecar, settings_persist.c / project.c scape_side_*): saved, loaded, an old project or settings without
 * them (SCAPE off), the autosave's.
 *   build/host/scape_test          exit 0: ALL PASSED */
#define UI_RENDER_NO_MAIN
#include "ui_render.c"

static int bad;
static void ck(const char *what, int ok)
{
    printf("  %-4s %s\n", ok ? "ok" : "FAIL", what);
    bad += !ok;
}
static void run(double secs, int32_t *peak)
{
    int32_t o[2 * CTL];
    uint32_t t, i;
    for (t = 0; t < (uint32_t)(secs * FS); t += CTL) {
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++)
            if (peak && (o[i] > *peak || -o[i] > *peak))
                *peak = o[i] < 0 ? -o[i] : o[i];
    }
}
static int biome_is(uint32_t b)
{
    int ok = song.g[G_SBIOME] == (int16_t)b && song.g[G_SDIST] == SC_BIOME[b].dist && song.g[G_STIME] == SC_BIOME[b].time;
    for (uint32_t k = 0; k < SC_NL; k++)
        ok &= song.g[G_SL0 + k] == SC_BIOME[b].level[k] && song.g[G_SD0 + k] == SC_BIOME[b].density[k];
    return ok;
}

int main(void)
{
    int16_t keep[G_COUNT];
    int32_t peak = 0;
    uint32_t sig;
    persist_t p, q;
    rep = fopen("/dev/null", "w");
    ui_power_on();
    song.master_q12 = 1600;
    usb.config = 0;

    ck("power-on: SCAPE off, JUNGLE's settings", song.g[G_SCAPE] == 0 && biome_is(1));
    run(1.0, 0);
    ck("off: nothing runs", !sc.on);

    go_title("EPOCH");
    press(B_HOME);
    ck("EPOCH, HOME tapped: the SCAPE page", !ui.home && str_eq(cur_page()->title, "SCAPE"));
    turn(EN_K1, 20);
    ck("KNOB 1: LEVEL", song.g[G_SCAPE] == 20);
    turn(EN_K2, 1);
    ck("KNOB 2: BIOME, which sets the layers, DIST and TIME (TUNDRA)", biome_is(2) && song.g[G_SCAPE] == 20);
    turn(EN_K2, 4);
    ck("BIOME again (CANYONS)", biome_is(6));
    turn(EN_K3, -5);
    turn(EN_K4, 7);
    ck("KNOB 3 DIST, KNOB 4 TIME", song.g[G_SDIST] == SC_BIOME[6].dist - 5 && song.g[G_STIME] == SC_BIOME[6].time + 7);
    press(B_HOME);
    ck("then SCAPE LEVELS", str_eq(cur_page()->title, "SCAPE LEVELS"));
    turn(EN_K1, -10);
    turn(EN_K4, 3);
    ck("its knobs: WIND .. BUGS", song.g[G_SL0] == SC_BIOME[6].level[0] - 10 && song.g[G_SL3] == SC_BIOME[6].level[3] + 3);
    press(B_HOME);
    turn(EN_K2, 9);
    ck("SCAPE LEVELS 2: FROGS .. RUMBL", str_eq(cur_page()->title, "SCAPE LEVELS 2") && song.g[G_SL5] == SC_BIOME[6].level[5] + 9);
    press(B_HOME);
    turn(EN_K2, 11);
    ck("SCAPE DENSITY: WIND .. BUGS", str_eq(cur_page()->title, "SCAPE DENSITY") && song.g[G_SD1] == SC_BIOME[6].density[1] + 11);
    press(B_HOME);
    turn(EN_K3, -4);
    ck("SCAPE DENSITY 2: FROGS .. RUMBL", str_eq(cur_page()->title, "SCAPE DENSITY 2") && song.g[G_SD6] == SC_BIOME[6].density[6] - 4);
    press(B_HOME);
    ck("then HOME", ui.home);

    song.g[G_SCAPE] = 100;
    peak = 0;
    run(4.0, &peak);
    ck("on: it sounds, well under full scale", sc.on && peak > 300 && peak < 16000);

    /* a project keeps it */
    memcpy(keep, song.g, sizeof keep);
    stop_transport();
    ck("saved in slot A", project_save(0) == 0 && scape_side[0][0] == 1u);
    scape_biome(4);
    song.g[G_SCAPE] = 7;
    project_load(0);
    ck("loaded with slot A: every SCAPE value back", !memcmp(&song.g[G_SCAPE], &keep[G_SCAPE], (G_SD7 - G_SCAPE + 1) * 2));
    memset(scape_side[1], 0, sizeof scape_side[1]);       /* slot B: a project from before SCAPE */
    ck("saved in slot B", project_save(1) == 0);
    memset(scape_side[1], 0, sizeof scape_side[1]);
    project_load(1);
    ck("a project with none stored: SCAPE off, its defaults", song.g[G_SCAPE] == 0 && biome_is(1));
    project_load(0);

    /* the autosave's */
    sig = autosave_sig();
    song.g[G_SL7]++;
    ck("a SCAPE change is an autosave's change", autosave_sig() != sig);
    song.g[G_SL7]--;
    scape_side_put(4);
    song.g[G_SCAPE] = 0;
    scape_side_get(4);
    ck("the autosave's SCAPE back", song.g[G_SCAPE] == keep[G_SCAPE] && song.g[G_SD6] == keep[G_SD6]);

    /* the settings carry it; older settings (no SCAPE) still load */
    memset(&p, 0, sizeof p);
    settings_export(&p);
    ck("in the settings", !memcmp(p.scape, scape_side, sizeof p.scape));
    q = p;
    memset(scape_side, 0, sizeof scape_side);
    ck("settings read back with it", settings_import(&q, (int)sizeof q) == 1 && scape_side[0][0] == 1u &&
       !memcmp(scape_side, p.scape, sizeof scape_side));
    {   /* the same settings at 572 bytes: as a full import would leave them, SCAPE none */
        persist_t full = p, old = p;
        int r1 = settings_import(&full, (int)sizeof full), r2;
        memset(old.scape, 0xAB, sizeof old.scape);
        r2 = settings_import(&old, (int)(sizeof old - sizeof old.scape));
        ck("settings of before (572 bytes): accepted, everything else as a full import, SCAPE none",
           r1 == 1 && r2 == 1 && !scape_side[0][0] && !scape_side[4][0] && old.palette == full.palette &&
           old.lowcut == full.lowcut && !memcmp(&old.favorites, &full.favorites, sizeof old.favorites) &&
           !memcmp(&old.panel, &full.panel, sizeof old.panel));
    }
    ck("settings 572 + 120 bytes", sizeof(persist_t) == 572u + 120u);

    printf(bad ? "SCAPE TEST: %d FAILED\n" : "SCAPE TEST: ALL PASSED\n", bad);
    return bad != 0;
}
