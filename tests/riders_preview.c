/* SPDX-License-Identifier: GPL-3.0-only */
/* Audition only: four fixed tracks through the real firmware mixer and FX.
 * Not a built-in demo or a sequencer/project serialization test. */
#define main hostsim_main
#include "hostsim.c"
#undef main

typedef struct { double on, off; uint8_t track, note, vel, started, ended; } rider_event;
static rider_event events[2048];

int main(int argc, char **argv)
{
    static const unsigned engine[4] = {14, 12, 14, 0};
    static const char *preset[4] = {"STAMPEDE", "TAR PIT BASS", "PRIMEVAL", "ICE AGE PAD"};
    static const int level[4] = {100, 100, 102, 85};
    unsigned k, p, i, count = 0, frame, frames;
    int32_t out[2 * CTL];
    double beat, length;
    unsigned track, pitch, velocity;
    FILE *f;
    if (argc != 2) return 1;
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    song.g[G_BPM] = 84;
    song.g[G_RTYPE] = fx.rtype = 2;
    song.g[G_RSIZE] = 100;
    song.g[G_RDAMP] = 65;
    song.master_q12 = 1300;
    for (k = 0; k < NTRK; k++) {
        for (p = 0; p < ENGINES[engine[k]]->npresets; p++)
            if (!strcmp(ENGINES[engine[k]]->presets[p].name, preset[k])) break;
        if (p == ENGINES[engine[k]]->npresets) return 2;
        host_preset(&trk[k], engine[k], p);
        trk[k].p[P_LEVEL] = level[k];
        trk[k].p[P_REV] = k == 1 ? 5 : 30;
        trk[k].p[P_DLY] = k == 2 ? 15 : 0;
    }
    while (scanf("%u %lf %u %lf %u", &track, &beat, &pitch, &length, &velocity) == 5) {
        if (count == 2048 || track >= NTRK || pitch > 127 || velocity > 127 || beat < 0 || length <= 0) return 3;
        events[count++] = (rider_event){beat * 60.0 / 84.0, (beat + length) * 60.0 / 84.0,
                                      track, pitch, velocity, 0, 0};
    }
    frames = (uint32_t)((48 * 4 * 60.0 / 84.0 + 7) * FS);
    frames = (frames + CTL - 1) / CTL * CTL;
    f = fopen(argv[1], "wb");
    if (!f) return 4;
    wav_hdr(f, frames);
    for (frame = 0; frame < frames; frame += CTL) {
        double now = frame / (double)FS;
        for (i = 0; i < count; i++)
            if (events[i].started && !events[i].ended && now >= events[i].off) {
                trk_note_off(&trk[events[i].track], events[i].note);
                events[i].ended = 1;
            }
        for (i = 0; i < count; i++)
            if (!events[i].started && now >= events[i].on) {
                trk_note_on(&trk[events[i].track], events[i].note, events[i].vel);
                events[i].started = 1;
            }
        mix_block(out, CTL);
        for (i = 0; i < CTL; i++) wav_put(f, out[2 * i], out[2 * i + 1]);
    }
    fclose(f);
    fprintf(stderr, "%u events, %.2f seconds, voice kills %u\n", count, frames / (double)FS, voice_kills);
    return 0;
}
