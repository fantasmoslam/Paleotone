/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX SCAPE (firmware/src/scape.c) heard through the firmware's mixer and reverb, the tracks silent.
 *   build/host/scape_render OUT.wav BIOME [SECONDS] [SCAPE_LEVEL]
 * env LAYER=k: that layer only (its level and density from the biome); TIME=v, DIST=v override the biome's.
 * Not a test. */
#define main hostsim_main
#include "hostsim.c"
#undef main

int main(int argc, char **argv)
{
    uint32_t b, i, t, frames, k;
    double secs;
    int32_t o[2 * CTL];
    FILE *f;
    if (argc < 3)
        return 1;
    b = (uint32_t)atoi(argv[2]);
    secs = argc > 3 ? atof(argv[3]) : 30.0;
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    for (i = 0; i < NTRK; i++)
        trk[i].p[P_LEVEL] = 0;
    song.g[G_RTYPE] = 2;                                  /* HALL, as the scenes */
    fx.rtype = 2;
    song.g[G_TAPE] = 0;
    song.master_q12 = 1600;                               /* (as tests/stem.c) */
    scape_biome(b);
    song.g[G_SCAPE] = (int16_t)(argc > 4 ? atoi(argv[4]) : 100);
    if (getenv("LAYER")) {
        for (k = 0; k < SC_NL; k++)
            if (k != (uint32_t)atoi(getenv("LAYER")))
                song.g[G_SL0 + k] = 0;
            else {                                        /* LVL=, DENS=: that layer's (calibration) */
                if (getenv("LVL"))
                    song.g[G_SL0 + k] = (int16_t)atoi(getenv("LVL"));
                if (getenv("DENS"))
                    song.g[G_SD0 + k] = (int16_t)atoi(getenv("DENS"));
            }
    }
    if (getenv("TIME"))
        song.g[G_STIME] = (int16_t)atoi(getenv("TIME"));
    if (getenv("DIST"))
        song.g[G_SDIST] = (int16_t)atoi(getenv("DIST"));
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
    printf("%s: %s, %.0f s\n", argv[1], SC_BIOME[b % SC_NBIOME].name, secs);
    return 0;
}
