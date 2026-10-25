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

    go_title("SCAPE MUSIC");
    press(B_HOME); press(B_HOME);
    ck("SCAPE overview, HOME twice: the habitat page", !ui.home && str_eq(cur_page()->title, "SCAPE"));
    go_title("SCAPE SETUP"); turn(EN_K3, 20); go_title("SCAPE");
    ck("KNOB 1: LEVEL", song.g[G_SCAPE] == 20);
    turn(EN_K1, 1);
    ck("KNOB 2: BIOME, which sets the layers, DIST and TIME (TUNDRA)", biome_is(2) && song.g[G_SCAPE] == 20);
    turn(EN_K1, 4);
    ck("BIOME again (CANYONS)", biome_is(6));
    turn(EN_K3, -5);
    turn(EN_K2, 7);
    ck("KNOB 3 DIST, KNOB 4 TIME", song.g[G_SDIST] == SC_BIOME[6].dist - 5 && song.g[G_STIME] == SC_BIOME[6].time + 7);
    go_title("SCAPE DRONE");
    ck("GLO: SCAPE DRONE", str_eq(cur_page()->title, "SCAPE DRONE") && song.g[G_SDRN] == 0);
    turn(EN_K1, 90); turn(EN_K2, 4); turn(EN_K3, -10); turn(EN_K4, -7);
    ck("shared harmonic drone: LEVEL KEY MOVE; creature PTCH", song.g[G_SDRN] == 90 &&
       song.g[G_SDKEY] == 4 && song.g[G_SDMOV] == 30 && song.g[G_SCNOTE] == -7);
    go_title("SCAPE LIFE");
    ck("GLO: SCAPE LIFE: both calls and passing beasts", str_eq(cur_page()->title, "SCAPE LIFE"));
    turn(EN_K1, 2); turn(EN_K2, 100); turn(EN_K3, 1); song.g[G_SCRATE]=100;
    ck("one creature track, call rate, danger rate and size", song.g[G_SCTRK] == 2 &&
       song.g[G_SCRATE] == 100 && song.g[G_SDNGR] == 100 && song.g[G_SDSIZ] == 2);
    go_title("SCAPE LEVELS");
    ck("GLO: SCAPE LEVELS", str_eq(cur_page()->title, "SCAPE LEVELS"));
    turn(EN_K1, -10); turn(EN_K4, 3);
    ck("its knobs: WIND .. BUGS", song.g[G_SL0] == SC_BIOME[6].level[0] - 10 && song.g[G_SL3] == SC_BIOME[6].level[3] + 3);
    go_title("SCAPE LEVELS 2"); turn(EN_K2, 9);
    ck("SCAPE LEVELS 2: FROGS .. RUMBL", str_eq(cur_page()->title, "SCAPE LEVELS 2") && song.g[G_SL5] == SC_BIOME[6].level[5] + 9);
    press(B_HOME); ck("then HOME: no separate DANGER or density menu", ui.home);
    /* Fine density and legacy route values still round-trip from older projects. */
    song.g[G_SD1] += 11; song.g[G_SD6] -= 4; song.g[G_SDSTP] = 80;

    song.g[G_SCAPE] = 100;
    song.g[G_SCTRK] = 0;
    song.g[G_SDNGR] = 0;
    peak = 0;
    run(4.0, &peak);
    ck("on: it sounds (with the drone), well under full scale", sc.on && peak > 300 && peak < 16000);
    {   /* the calls: T2 calls (a SAURUS creature), a note at D3 + PTCH +- SPRD, let go when it ends and when SCAPE stops */
        uint32_t calls = 0, was = 0, t;
        int32_t o[2 * CTL];
        host_preset(&trk[1], ENGI_SAURUS, 0);
        song.g[G_SCTRK] = 2;
        song.g[G_SCRATE] = 127;
        for (t = 0; t < 40u * FS; t += CTL) {
            mix_block(o, CTL);
            if (sc.cnote && !was)
                calls += sc.ctrk == 1u && sc.cnote >= 50 - 7 - 8 && sc.cnote <= 50 - 7 + 8;
            was = sc.cnote;
        }
        ck("calls: T2 played now and then, the notes round D3 + PTCH", calls >= 3u && calls <= 14u);
        while (!sc.cnote)
            mix_block(o, CTL);
        song.g[G_SCAPE] = 0;
        mix_block(o, CTL);
        ck("SCAPE off: a call sounding let go", !sc.on);
        song.g[G_SCAPE] = 100;
        song.g[G_SCTRK] = 0;
    }
    {   /* DANGER: a beast comes by: steps, the hush, the roar on its track (T2 here) at the nearest, no calls meanwhile */
        uint32_t t, beasts = 0, steps = 0, roars = 0, calls = 0, was = 0, wasf = 0, wasr = 0;
        int32_t o[2 * CTL], hush = 32767;
        song.g[G_SDNGR] = 127;
        song.g[G_SDSIZ] = 1;
        song.g[G_SDROR] = 3;                              /* T2 */
        song.g[G_SCTRK] = 2;
        song.g[G_SCRATE] = 127;
        for (t = 0; t < 90u * FS; t += CTL) {
            mix_block(o, CTL);
            beasts += sc.nlen && sc.nt == 1u;
            steps += sc.fn && !wasf;
            wasf = sc.fn;
            roars += sc.rnote && !wasr && sc.rtrk == 1u && sc.rnote == 50 - 7 - 7;
            wasr = sc.rnote;
            calls += sc.cnote && !was && sc.nlen && sc.nt > 2u;
            was = sc.cnote;
            if (sc.nlen && sc.hush < hush)
                hush = sc.hush;
        }
        ck("DANGER 127: beasts come by, each with steps and a roar on T2 (PTCH -7, BIG: a fifth down)",
           beasts >= 1u && beasts <= 4u && steps >= beasts * 10u && roars == beasts);
        ck("DANGER: the insects, frogs and birds hush; no calls start while it passes", hush < 4000 && calls == 0u);
        while (!sc.rnote)
            mix_block(o, CTL);
        song.g[G_SCAPE] = 0;
        mix_block(o, CTL);
        ck("SCAPE off: a roar sounding let go", !sc.on && !sc.rnote);
        song.g[G_SCAPE] = 100;
        song.g[G_SDNGR] = 0;
        song.g[G_SDSIZ] = 2;
        song.g[G_SDROR] = 2;
        song.g[G_SCTRK] = 0;
        mix_block(o, CTL);
        ck("DANGER 0: none", !sc.nlen && !sc.narm);
        song.g[G_SDNGR] = 100;
    }

    /* a project keeps it */
    memcpy(keep, song.g, sizeof keep);
    stop_transport();
    ck("saved in slot A", project_save(0) == 0 && scape_side[0][0] == 1u);
    scape_biome(4);
    song.g[G_SCAPE] = 7;
    project_load(0);
    ck("loaded with slot A: every SCAPE value back (the drone, the calls: PTCH -7)",
       !memcmp(&song.g[G_SCAPE], &keep[G_SCAPE], (G_SDROR - G_SCAPE + 1) * 2) && song.g[G_SCNOTE] == -7 &&
       song.g[G_SDNGR] == 100 && song.g[G_SDSIZ] == 2 && song.g[G_SDSTP] == 80 && song.g[G_SDROR] == 2);
    memset(scape_side[1], 0, sizeof scape_side[1]);       /* slot B: a project from before SCAPE */
    ck("saved in slot B", project_save(1) == 0);
    memset(scape_side[1], 0, sizeof scape_side[1]);
    project_load(1);
    ck("a project with none stored: SCAPE off, its defaults", song.g[G_SCAPE] == 0 && biome_is(1) && song.g[G_SDRN] == 0 &&
       song.g[G_SCTRK] == 0 && song.g[G_SCNOTE] == 0);
    project_load(0);
    scape_side[0][1u + 20u] = 0;                        /* slot A as the first SCAPE firmware wrote it: no drone, no */
    memset(&scape_side[0][1u + 20u], 0, 12);            /* calls, no DANGER stored */
    project_load(0);
    ck("an entry of the first SCAPE firmware: its twenty, the drone and calls at their defaults",
       song.g[G_SCAPE] == keep[G_SCAPE] && song.g[G_SL0] == keep[G_SL0] && song.g[G_SDRN] == 0 && song.g[G_SDTYP] == 0 &&
       song.g[G_SDKEY] == 0 && song.g[G_SDMOV] == 40 && song.g[G_SCTRK] == 0 && song.g[G_SCRATE] == 40 &&
       song.g[G_SCNOTE] == 0 && song.g[G_SCSPRD] == 5 && song.g[G_SDNGR] == 0 && song.g[G_SDSIZ] == 1 &&
       song.g[G_SDSTP] == 100 && song.g[G_SDROR] == 1);
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
    ck("settings 572 + 240 bytes", sizeof(persist_t) == 572u + 240u);

    printf(bad ? "SCAPE TEST: %d FAILED\n" : "SCAPE TEST: ALL PASSED\n", bad);
    return bad != 0;
}
