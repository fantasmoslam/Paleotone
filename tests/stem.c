/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX: one sound's part rendered by the firmware's own audio code, dry (its sends off), for mixing elsewhere.
 *   build/host/stem OUT.wav < SCORE
 * SCORE: a line "ENGINE_INDEX PRESET NAME|BPM|SECONDS" (the name may hold spaces; '|' separates), then lines
 *   "e K V"        EDIT value K (0..7) set to V          "p ID V"   track parameter ID set to V
 *   "n BEAT NOTE LEN VEL"  a note at BEAT (quarter notes from 0), LEN beats long
 * Not a test. */
#define main hostsim_main
#include "hostsim.c"
#undef main

typedef struct { double on, off; uint8_t note, vel; uint8_t started, ended; } ev_t;
static ev_t ev[4096];

int main(int argc, char **argv)
{
    char line[256], name[64];
    unsigned eng;
    double bpm = 96, secs = 10;
    uint32_t nev = 0, i, t, frames, pi;
    int32_t o[2 * CTL];
    FILE *f;
    if (argc < 2 || !fgets(line, sizeof line, stdin))
        return 1;
    {
        char *a = strchr(line, ' '), *b, *c;
        if (!a) return 1;
        *a = 0;
        eng = (unsigned)atoi(line);
        b = strchr(a + 1, '|');
        c = b ? strchr(b + 1, '|') : 0;
        if (!b || !c) return 1;
        *b = 0;
        snprintf(name, sizeof name, "%s", a + 1);
        bpm = atof(b + 1);
        secs = atof(c + 1);
    }
    for (pi = 0; pi < ENGINES[eng]->npresets && strcmp(ENGINES[eng]->presets[pi].name, name); pi++)
        ;
    if (pi == ENGINES[eng]->npresets) {
        fprintf(stderr, "stem: no preset %s\n", name);
        return 1;
    }
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    song.g[G_BPM] = (int16_t)(bpm + 0.5);
    song.g[G_RTYPE] = 2;
    fx.rtype = 2;
    song.g[G_TAPE] = 0;
    song.master_q12 = 1600;                       /* (under the limiter) */
    host_preset(&trk[0], eng, pi);
    trk[0].p[P_REV] = 0;
    trk[0].p[P_DLY] = 0;
    trk[0].p[P_CHOR] = 0;
    for (i = 1; i < NTRK; i++)
        trk[i].p[P_LEVEL] = 0;
    while (fgets(line, sizeof line, stdin)) {
        double b, l;
        int k, v, nt, vel;
        if (sscanf(line, "e %d %d", &k, &v) == 2)
            trk[0].p[P_E0 + k] = (int16_t)v;
        else if (sscanf(line, "p %d %d", &k, &v) == 2)
            trk[0].p[k] = (int16_t)v;
        else if (sscanf(line, "n %lf %d %lf %d", &b, &nt, &l, &vel) == 4 && nev < 4096u) {
            ev[nev].on = b * 60.0 / bpm;
            ev[nev].off = (b + l) * 60.0 / bpm;
            ev[nev].note = (uint8_t)nt;
            ev[nev].vel = (uint8_t)vel;
            nev++;
        }
    }
    frames = (uint32_t)(secs * FS);
    if (!(f = fopen(argv[1], "wb")))
        return 1;
    wav_hdr(f, frames);
    for (t = 0; t < frames; t += CTL) {
        double now = t / (double)FS;
        for (i = 0; i < nev; i++) {               /* offs first (a repeated note), then ons */
            if (ev[i].started && !ev[i].ended && now >= ev[i].off) {
                trk_note_off(&trk[0], ev[i].note);
                ev[i].ended = 1;
            }
        }
        for (i = 0; i < nev; i++)
            if (!ev[i].started && now >= ev[i].on) {
                trk_note_on(&trk[0], ev[i].note, ev[i].vel);
                ev[i].started = 1;
            }
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            wav_put(f, o[2 * i], o[2 * i + 1]);
    }
    fclose(f);
    return 0;
}
