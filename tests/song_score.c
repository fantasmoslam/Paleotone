/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX: a song built in the firmware's own sequencer from a text score: the four tracks' sounds, the four project
 * slots' steps and automation, the song chain (lanes, repeats), then played through mix_block like the FM-1 plays it.
 *   build/host/song_score OUT.wav < SCORE        (tools/riders_song.py writes RIDERS OF THE VALLEY's)
 * SCORE lines ('#' comments):
 *   bpm B | master Q12 | secs S | scape BIOME LEVEL (SCAPE, firmware/src/scape.c)
 *   sound K ENGINE PRESET NAME        track K (0..3): an engine index and a factory preset (sends: DIST only)
 *   level K V | param K NAME V        the sound's own LEVEL or a parameter (E0..E7 ATK DEC SUS REL DST CHO DLY REV PAN RING)
 *   slot S                            start project slot S (0..3): every track's steps and automation cleared
 *   pat K DIV LEN GATE                track K's DIV (N_DIV index: 0 1/4, 1 1/8, 7 1/1), LEN steps, GATE
 *   n K STEP NOTE STEPS VEL [RATCH]   a note (up to 4 on a step), held STEPS steps (ties); RATCH 2 or 3: the step
 *                                     ratcheted (16ths at 1/8)
 *   m K STEP NAME V                   automation of a parameter on a step (held until changed)
 *   mpreset K STEP PRESET NAME        automation to another preset of the track's engine (E0..E7, envelope, DST)
 *   save                              save the slot
 *   row S1 S2 S3 S4 REPEAT            a song section: each track's slot (4: silent)
 * Not a test. */
#define UI_RENDER_NO_MAIN
#include "ui_render.c"

static const struct { const char *name; uint32_t id; } PNAME[] = {
    {"E0", P_E0}, {"E1", P_E0 + 1}, {"E2", P_E0 + 2}, {"E3", P_E0 + 3}, {"E4", P_E0 + 4}, {"E5", P_E0 + 5},
    {"E6", P_E0 + 6}, {"E7", P_E0 + 7}, {"ATK", P_ATK}, {"DEC", P_DEC}, {"SUS", P_SUS}, {"REL", P_REL},
    {"DST", P_DIST}, {"CHO", P_CHOR}, {"DLY", P_DLY}, {"REV", P_REV}, {"PAN", P_PAN}, {"LVL", P_LEVEL}, {"RING", P_RING}};
static uint32_t pid(const char *s)
{
    for (uint32_t i = 0; i < sizeof PNAME / sizeof PNAME[0]; i++)
        if (!strcmp(PNAME[i].name, s))
            return PNAME[i].id;
    fprintf(stderr, "song_score: no parameter %s\n", s);
    exit(1);
}
static uint32_t preset_of(uint32_t e, const char *name)
{
    for (uint32_t i = 0; i < ENGINES[e]->npresets; i++)
        if (!strcmp(ENGINES[e]->presets[i].name, name))
            return i;
    fprintf(stderr, "song_score: no preset %s\n", name);
    exit(1);
}
static void chomp(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' '))
        s[--n] = 0;
}
static void clear_track(track_t *t)
{
    memset(t->step, 0, sizeof t->step);
    for (uint32_t i = 0; i < NSTEP; i++)
        t->step[i].time = ST_REST;
    motion_clear(t);
}
static void automate(track_t *t, uint32_t step, uint32_t id, int v)
{
    if (motion_set_event(t, step, id, (int16_t)v)) {
        fprintf(stderr, "song_score: automation full (track %u step %u)\n", (unsigned)(t - trk), step);
        exit(1);
    }
}

int main(int argc, char **argv)
{
    char line[256], name[96];
    double secs = 60;
    uint32_t i, t, frames, nrows = 0, cur = 99;
    int k, a, b, c, d, e;
    int32_t o[2 * CTL];
    FILE *f;
    if (argc < 2)
        return 1;
    rep = fopen("/dev/null", "w");
    ui_power_on();
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    song.g[G_TAPE] = 0;
    song.g[G_RTYPE] = 2;
    fx.rtype = 2;
    song.master_q12 = 1600;
    song.batt_raw = 600;
    usb.config = 0;
    rev_clear();
    hall_clear();
    chain_defaults(&chain_config);
    while (fgets(line, sizeof line, stdin)) {
        chomp(line);
        if (!line[0] || line[0] == '#')
            continue;
        if (sscanf(line, "bpm %d", &a) == 1)
            song.g[G_BPM] = (int16_t)a;
        else if (sscanf(line, "master %d", &a) == 1)
            song.master_q12 = (uint32_t)a;
        else if (sscanf(line, "secs %lf", &secs) == 1)
            ;
        else if (sscanf(line, "scape %d %d", &a, &b) == 2) {   /* SCAPE: a biome at a level (scape.c) */
            scape_biome((uint32_t)a);
            song.g[G_SCAPE] = (int16_t)b;
        }
        else if (sscanf(line, "sound %d %d %95[^\n]", &k, &a, name) == 3) {
            track_t *tk = &trk[k & 3];
            host_preset(tk, (uint32_t)a, preset_of((uint32_t)a, name));
            tk->p[P_CHOR] = tk->p[P_DLY] = tk->p[P_REV] = 0;   /* dry, as tests/stem.c renders the parts */
        } else if (sscanf(line, "level %d %d", &k, &a) == 2)
            trk[k & 3].p[P_LEVEL] = (int16_t)a;
        else if (sscanf(line, "param %d %15s %d", &k, name, &a) == 3)
            trk[k & 3].p[pid(name)] = (int16_t)a;
        else if (sscanf(line, "slot %d", &a) == 1) {
            cur = (uint32_t)a & 3u;
            for (i = 0; i < NTRK; i++)
                clear_track(&trk[i]);
        } else if (sscanf(line, "pat %d %d %d %d", &k, &a, &b, &c) == 4) {
            trk[k & 3].p[P_SDIV] = (int16_t)a;
            trk[k & 3].p[P_SLEN] = (int16_t)b;
            trk[k & 3].p[P_SGATE] = (int16_t)c;
        } else if (sscanf(line, "n %d %d %d %d %d", &k, &a, &b, &c, &d) == 5) {
            track_t *tk = &trk[k & 3];
            int r = 1;
            sscanf(line, "n %*d %*d %*d %*d %*d %d", &r);
            step_t *s = &tk->step[a % NSTEP];
            if (s->time == ST_NOTE && s->n < 4u) {                 /* another note on the step: a chord / kit hit */
                s->note[s->n++] = (uint8_t)b;
            } else {
                uint8_t nt = (uint8_t)b;
                put_step(tk, (uint32_t)a % NSTEP, 1, &nt, ST_NOTE, r >= 2 ? (uint32_t)(r - 1) << SF_RATCH_SH : 0u);
                s->vel = (uint8_t)d;
                for (e = 1; e < c && a + e < NSTEP; e++)
                    put_step(tk, (uint32_t)(a + e), 0, &nt, ST_TIE, 0);
            }
        } else if (sscanf(line, "mpreset %d %d %95[^\n]", &k, &a, name) == 3) {
            track_t *tk = &trk[k & 3];
            const preset_t *p = &ENGINES[tk->engine]->presets[preset_of(tk->engine, name)];
            for (i = 0; i < 8u; i++)
                automate(tk, (uint32_t)a, P_E0 + i, p->e[i]);
            for (i = 0; i < 4u; i++)
                automate(tk, (uint32_t)a, P_ATK + i, p->env[i]);
            automate(tk, (uint32_t)a, P_DIST, p->fx[0] ? p->fx[0] - 1 : 0);
        } else if (sscanf(line, "m %d %d %15s %d", &k, &a, name, &b) == 4)
            automate(&trk[k & 3], (uint32_t)a, pid(name), b);
        else if (!strcmp(line, "save")) {
            if (cur > 3u || project_save(cur)) {
                fprintf(stderr, "song_score: slot %u not saved\n", cur);
                return 1;
            }
        } else if (sscanf(line, "row %d %d %d %d %d", &a, &b, &c, &d, &e) == 5 && nrows < CHAIN_ROWS) {
            chain_row_t *r = &chain_config.row[nrows++];
            r->slot[0] = (uint8_t)a;
            r->slot[1] = (uint8_t)b;
            r->slot[2] = (uint8_t)c;
            r->slot[3] = (uint8_t)d;
            r->repeat = (uint8_t)e;
        } else {
            fprintf(stderr, "song_score: ? %s\n", line);
            return 1;
        }
    }
    chain_config.count = (uint8_t)nrows;
    if (chain_prepare()) {
        fprintf(stderr, "song_score: the song did not start\n");
        return 1;
    }
    ui.home = 1;
    frames = (uint32_t)(secs * FS);
    if (!(f = fopen(argv[1], "wb")))
        return 1;
    wav_hdr(f, frames);
    for (t = 0; t < frames; t += CTL) {
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            wav_put(f, o[2 * i], o[2 * i + 1]);
    }
    fclose(f);
    printf("%s: %.1f s, %u sections, voices given up %u\n", argv[1], frames / (double)FS, nrows, voice_kills);
    return 0;
}
