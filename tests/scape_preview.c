/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX: SCAPE (firmware/src/scape.c) rendered through the firmware's whole mix (the reverb, MASTER at its power-on
 * 2048, the limiter), the creatures' calls by SAURUS on a track. Not a test: previews for the owner, and its numbers.
 *   build/host/scape_preview OUT.wav SECS BIOME TIME WEATHER DANGER [MUSIC] [SEED] [ENC_AT]
 * BIOME 0..7 (WATER JUNGLE TUNDRA FOREST PLAINS MOUNTAINS CANYONS CALDERA), TIME 0..3 (DAWN DAY DUSK NIGHT), WEATHER
 * 0..3 (CLEAR WIND RAIN STORM), DANGER 0..127; MUSIC 1: the JUNGLE scene playing under it (tracks 1..3; the calls on
 * track 4); ENC_AT: the first encounter's second. Prints the RMS and peak (dBFS) and the cost (instructions/sample). */
#define DINOBOX_SCAPE 1
#define SCAPE_AUDITION 1
static unsigned trace_k[32];
#define SCAPE_TRACE(kind, k, d, lvl) (trace_k[(kind) & 31u]++)
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

static const char *const SAURUS_NAMES[20] = {
    "BRACHIO CALL", "TYRANT ROAR", "RAPTOR BARK", "RAPTOR CHAT", "CREST HOOT", "PTERO SHRIEK", "HERD MOAN", "FRILL HISS",
    "SAURO SNORT", "DEEP RUMBLE", "TITAN APE", "NEST CHIRPS", "SWAMP CROAK", "MONSTER ROAR", "DISTANT HERD", "LOW GROWL",
    "BABY REX", "SEA MONSTER", "HADRO HORN", "COMPY PACK"};
static uint32_t now_s, off_at[NTRK];
static uint8_t music, call_tr;
static uint32_t ncalls;

static void scape_call(uint32_t c, int32_t lvl, int32_t dist, int32_t pan, uint32_t ms)
{
    uint32_t k = music ? 3u : 2u + (call_tr ^= 1u), pi;
    track_t *t = &trk[k];
    const engine_t *e = ENGINES[ENGI_SAURUS];
    for (pi = 0; pi < e->npresets && strcmp(e->presets[pi].name, SAURUS_NAMES[c % 20u]); pi++)
        ;
    if (pi == e->npresets)
        return;
    if (getenv("SCAPE_NOCALLS"))
        return;
    trk_all_off(t);
    host_preset(t, ENGI_SAURUS, pi);
    /* the level: a track's default (104) with lvl (SCAPE in it) under it, the distance's -18 dB at most (LVL: 112 is 0 dB, 2 a dB) */
    t->p[P_LEVEL] = (int16_t)clamp(104 + (int32_t)(40.0 * log10(lvl / 32767.0 + 1e-9)) - dist * 36 / 127, 0, 112);
    t->p[P_PAN] = (int16_t)(pan * 50 / 32768);
    t->p[P_REV] = (int16_t)(30 + dist * 70 / 127);
    t->p[P_DLY] = 0;
    t->p[P_CHOR] = 0;
    t->p[P_MUTE] = 0;
    if (getenv("SCAPE_VERBOSE"))
        printf("call %s lvl %d dist %d -> LVL %d at %.1f s\n", SAURUS_NAMES[c % 20u], lvl, dist, t->p[P_LEVEL], now_s / (double)FS);
    trk_note_on(t, 50, 100);
    off_at[k] = now_s + (ms ? ms : 3000u) * (FS / 1000u);
    ncalls++;
}

static uint64_t instr_now(void)
{
#ifdef __linux__
    static int fd = -2;
    uint64_t v;
    if (fd == -2) {
        struct perf_event_attr a;
        memset(&a, 0, sizeof a);
        a.type = PERF_TYPE_HARDWARE;
        a.size = sizeof a;
        a.config = PERF_COUNT_HW_INSTRUCTIONS;
        a.exclude_kernel = a.exclude_hv = 1;
        fd = (int)syscall(SYS_perf_event_open, &a, 0, -1, -1, 0);
    }
    if (fd >= 0 && read(fd, &v, sizeof v) == (ssize_t)sizeof v)
        return v;
#endif
    return 0;
}

int main(int argc, char **argv)
{
    static int32_t out[2 * CTL], l[CTL], r[CTL], v[CTL];
    uint32_t secs, i, k, frames;
    double acc = 0, peak = 0;
    uint64_t i0, cost = 0;
    FILE *f;
    if (argc < 7)
        return 1;
    secs = (uint32_t)atoi(argv[2]);
    music = argc > 7 && atoi(argv[7]);
    ui_power_on();
    song.master_q12 = 2048;                             /* (main.c: the power-on MASTER) */
    if (music) {
        gb_scene_load(0);                               /* JUNGLE */
        trk[3].p[P_MUTE] = 1;
        transport_req = 1;
    } else
        for (i = 0; i < NTRK; i++)
            trk[i].p[P_MUTE] = 1;
    scape_reset();
    if (argc > 8)
        sc.rng = atoi(argv[8]) * 7919 + 1;
    scape_biome((uint32_t)atoi(argv[3]));
    if (getenv("SCAPE_LEVEL"))
        sc.level = (uint8_t)atoi(getenv("SCAPE_LEVEL"));
    sc.tod = (uint8_t)atoi(argv[4]);
    sc.weather = (uint8_t)atoi(argv[5]);
    sc.danger = (uint8_t)atoi(argv[6]);
    if (argc > 9)
        sc.enc_wait = (uint32_t)(atof(argv[9]) * SC_BPS);
    if (getenv("SCAPE_SOLO")) {                         /* one layer alone (no drone, no encounter) */
        uint32_t s = (uint32_t)atoi(getenv("SCAPE_SOLO"));
        for (i = 0; i < SC_NL; i++)
            if (i != s)
                sc.l[i].level = 0;
        sc.dlevel = 0;
        sc.danger = 0;
    }
    if (getenv("SCAPE_DRONE")) {                        /* the drone alone */
        for (i = 0; i < SC_NL; i++)
            sc.l[i].level = 0;
        sc.danger = 0;
    }
    if (getenv("SCAPE_COST")) {                         /* the generator alone (no mix): its cost */
        for (now_s = 0; now_s < secs * FS; now_s += CTL) {
            memset(l, 0, sizeof l); memset(r, 0, sizeof r); memset(v, 0, sizeof v);
            i0 = instr_now();
            scape_block(l, r, v, CTL);
            cost += instr_now() - i0;
            for (i = 0; i < CTL; i++)
                peak = fabs((double)l[i]) > peak ? fabs((double)l[i]) : peak;
        }
        {
            struct timespec a, b;
            double ns_scape, ns_voice;
            uint32_t j;
            scape_reset(); scape_biome((uint32_t)atoi(argv[3])); sc.tod = (uint8_t)atoi(argv[4]); sc.weather = (uint8_t)atoi(argv[5]);
            sc.danger = (uint8_t)atoi(argv[6]); sc.enc_wait = SC_BPS * 2u;
            clock_gettime(CLOCK_MONOTONIC, &a);
            for (j = 0; j < secs * FS / CTL; j++) { memset(l, 0, sizeof l); memset(r, 0, sizeof r); memset(v, 0, sizeof v); scape_block(l, r, v, CTL); }
            clock_gettime(CLOCK_MONOTONIC, &b);
            ns_scape = ((b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec)) / ((double)secs * FS);
            host_preset(&trk[0], ENGI_SAURUS, 0);       /* against one SAURUS voice calling */
            trk[0].p[P_MUTE] = 0;
            clock_gettime(CLOCK_MONOTONIC, &a);
            for (j = 0; j < secs * FS / CTL; j++) {
                if (!(j % 2000u)) trk_note_on(&trk[0], 50, 100);
                if (j % 2000u == 1500u) trk_all_off(&trk[0]);
                memset(l, 0, sizeof l); track_render(&trk[0], l, CTL);
            }
            clock_gettime(CLOCK_MONOTONIC, &b);
            ns_voice = ((b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec)) / ((double)secs * FS);
            printf("time: scape %.1f ns/sample, a SAURUS track calling %.1f ns/sample (scape = %.2f of it)\n", ns_scape, ns_voice, ns_scape / ns_voice);
        }
        return 0;
    }
    if (!(f = fopen(argv[1], "wb")))
        return 1;
    frames = secs * FS / CTL * CTL;
    wav_hdr(f, frames);
    for (now_s = 0; now_s < frames; now_s += CTL) {
        fm1_ms = (uint32_t)((uint64_t)now_s * 1000u / FS);
        for (k = 0; k < NTRK; k++)
            if (off_at[k] && now_s >= off_at[k]) {
                trk_all_off(&trk[k]);
                off_at[k] = 0;
            }
        mix_block(out, CTL);
        for (i = 0; i < CTL; i++) {
            double a = fabs((double)out[2 * i]) > fabs((double)out[2 * i + 1]) ? fabs((double)out[2 * i]) : fabs((double)out[2 * i + 1]);
            acc += (double)out[2 * i] * out[2 * i] + (double)out[2 * i + 1] * out[2 * i + 1];
            peak = a > peak ? a : peak;
            wav_put(f, out[2 * i], out[2 * i + 1]);
        }
    }
    fclose(f);
    if (getenv("SCAPE_KINDS")) {
        static const char *const KN[] = {"off", "wind", "bubble", "waves", "drop", "cicada", "lava", "rumble", "song", "whistle",
            "hoot", "knock", "wader", "raptor", "cricket", "frog", "toad", "drip", "rocks", "snap", "creak", "steam", "call",
            "herd", "thunder"};
        for (i = 1; i < 25u; i++)
            if (trace_k[i])
                printf("%s %u, ", KN[i], trace_k[i]);
        printf("\n");
    }
    printf("rms %.1f dBFS  peak %.1f dBFS  calls %u  events %u\n", 20 * log10(sqrt(acc / (2.0 * frames)) / 32768 + 1e-12),
           20 * log10(peak / 32768 + 1e-12), ncalls, sc.nspawn);
    return 0;
}
