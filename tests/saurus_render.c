/* SPDX-License-Identifier: GPL-3.0-only */
/* SAURUS (eng_saurus.c) on the Mac / Linux: one call, through the firmware's voice, mixer and reverb.
 *   build/host/saurus_render OUT.wav NOTE SECS HOLD [PRESET_NAME]
 * Without PRESET_NAME: env CALL SIZE SWOOP LEN GROWL BRTH MOUTH WILD (the knobs, 64-ish defaults), DRY=1 sends off.
 * Not a test (tools/saurus_design.py's tuning, the previews). */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int ev(const char *k, int d) { const char *v = getenv(k); return v ? atoi(v) : d; }

int main(int argc, char **argv)
{
    uint32_t i, t, frames, hold, note, pi = 0;
    int32_t o[2 * CTL];
    FILE *f;
    track_t *tr = &trk[0];
    if (argc < 5)
        return 1;
    note = (uint32_t)atoi(argv[2]);
    frames = (uint32_t)(atof(argv[3]) * FS);
    hold = (uint32_t)(atof(argv[4]) * FS);
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    for (i = 0; i < NTRK; i++)
        trk[i].p[P_LEVEL] = 0;
    if (argc > 5) {
        for (pi = 0; pi < ENG_SAURUS.npresets && strcmp(ENG_SAURUS.presets[pi].name, argv[5]); pi++)
            ;
        if (pi == ENG_SAURUS.npresets)
            return 2;
    }
    host_preset(tr, ENGI_SAURUS, pi);
    if (argc <= 5) {
        static const char *const K[8] = {"CALL", "SIZE", "SWOOP", "LEN", "GROWL", "BRTH", "MOUTH", "WILD"};
        static const int D[8] = {2, 64, 64, 100, 64, 64, 64, 0};
        for (i = 0; i < 8u; i++)
            tr->p[P_E0 + i] = (int16_t)ev(K[i], D[i]);
        tr->p[P_ATK] = 0; tr->p[P_DEC] = 127; tr->p[P_SUS] = 127; tr->p[P_REL] = (int16_t)ev("REL", 60);
    }
    tr->p[P_LEVEL] = 110;
    if (ev("DRY", 0)) {
        tr->p[P_REV] = 0; tr->p[P_DLY] = 0; tr->p[P_CHOR] = 0; tr->p[P_DIST] = 0;
    }
    song.g[G_RTYPE] = 2; fx.rtype = 2; song.g[G_TAPE] = 0; song.master_q12 = 1600;
    if (!(f = fopen(argv[1], "wb")))
        return 3;
    wav_hdr(f, frames);
    trk_note_on(tr, note, (uint32_t)ev("VEL", 110));
    for (t = 0; t < frames; t += CTL) {
        if (t < hold && t + CTL >= hold)
            trk_note_off(tr, note);
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            wav_put(f, o[2 * i], o[2 * i + 1]);
    }
    fclose(f);
    return 0;
}
