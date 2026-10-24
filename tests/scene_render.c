/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX: a power-on scene (engines.c GB_SCENES, ui.c gb_scene_load) as the FM-1 plays it: loaded, PLAY pressed.
 *   build/host/scene_render OUT.wav SCENE [SECONDS]
 * env T1..T4="ENGINE PRESET NAME|PATTERN": that track's sound and pattern instead (an audition; prints the index the
 * scene table wants); LV1..LV4: a track's LEVEL; BPM: the tempo. Not a test. */
#define UI_RENDER_NO_MAIN
#include "ui_render.c"

int main(int argc, char **argv)
{
    uint32_t i, t, frames, s;
    int32_t o[2 * CTL];
    FILE *f;
    char ev[4];
    if (argc < 3)
        return 1;
    rep = fopen("/dev/null", "w");
    s = (uint32_t)atoi(argv[2]);
    ui_power_on();
    gb_scene_load(s);
    for (i = 0; i < NTRK; i++) {
        const char *v;
        snprintf(ev, sizeof ev, "T%u", i + 1u);
        if ((v = getenv(ev))) {                          /* "E NAME|PAT": find the browser index of NAME */
            char name[64];
            uint32_t e = (uint32_t)atoi(v), k, pat = 0;
            const char *sp = strchr(v, ' '), *bar = strchr(v, '|');
            snprintf(name, sizeof name, "%.*s", (int)(bar ? bar - sp - 1 : (long)strlen(sp + 1)), sp + 1);
            pat = bar ? (uint32_t)atoi(bar + 1) : 0u;
            set_engine_of(&trk[i], e);
            for (k = 0; k < ENGINES[e]->npresets; k++) {
                apply_preset_to(&trk[i], k);
                if (!strcmp(ENGINES[e]->presets[trk[i].preset].name, name))
                    break;
            }
            trk[i].engine = trk[i].eng_req;
            eng_mem_claim(&trk[i]);
            track_defaults_steps(&trk[i]);
            if (pat)
                load_pat16(&trk[i], PATTERNS[pat - 1u].note, PATTERNS[pat - 1u].flags);
            trk[i].p[P_SDIV] = 1;
            printf("  T%u: {%u, %u, %u}  %s %s\n", i + 1u, e, k, pat, ENGINES[e]->presets[trk[i].preset].name,
                   pat ? PATTERNS[pat - 1u].name : "-");
        } else
            printf("  T%u: %s %s\n", i + 1u, ENGINES[trk[i].engine]->presets[trk[i].preset].name,
                   GB_SCENES[s].part[i][2] ? PATTERNS[GB_SCENES[s].part[i][2] - 1u].name : "-");
        snprintf(ev, sizeof ev, "LV%u", i + 1u);
        if (getenv(ev))
            trk[i].p[P_LEVEL] = (int16_t)atoi(getenv(ev));
    }
    if (getenv("BPM"))
        song.g[G_BPM] = (int16_t)atoi(getenv("BPM"));
    song.master_q12 = 1600;
    usb.config = 0;
    song.batt_raw = 600;
    ui.home = 1;
    press(B_PLAY);
    frames = (uint32_t)((argc > 3 ? atof(argv[3]) : 20.0) * FS);
    if (!(f = fopen(argv[1], "wb")))
        return 1;
    wav_hdr(f, frames);
    for (t = 0; t < frames; t += CTL) {
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            wav_put(f, o[2 * i], o[2 * i + 1]);
    }
    fclose(f);
    printf("%s: %s, %d BPM, voices given up %u\n", argv[1], GB_SCENES[s].name, song.g[G_BPM], voice_kills);
    return 0;
}
