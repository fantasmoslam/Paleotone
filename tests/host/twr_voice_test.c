/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS: the layers on Paleotone's sounds (firmware/src/pt, amb.c's hooks), in the whole instrument. Each layer
 * alone (the others at 0, no atmosphere), playing: on its own default voice, then on each Paleotone sound of its list,
 * its level by RMS against its own (printed: the median sets amb.c's PT_G_*), every sound heard, no NaN; switched
 * back and forth while it plays; stopped and quiet, every MIDI note it sent let go.
 *   twr_voice_test [SECS]   SECS a sound (default 12) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "amb.h"
#include "../../firmware/src/pt/pt.h"

static int fails;
#define CHECK(c, ...)                                                                                              \
    do {                                                                                                           \
        if (!(c)) {                                                                                                \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                            \
            printf(__VA_ARGS__);                                                                                   \
            printf("\n");                                                                                          \
            fails++;                                                                                               \
        }                                                                                                          \
    } while (0)

static int held[16][128];                        /* the notes sounding on MIDI out, by channel */
static long ons[16];
void en_midi_out(uint32_t st, uint32_t d1, uint32_t d2)
{
    const uint32_t ch = st & 15u, k = st & 0xF0u;
    if (k == 0x90u && d2 > 0u) {
        held[ch][d1 & 127u]++;
        ons[ch]++;
    } else if (k == 0x80u || k == 0x90u) {
        if (held[ch][d1 & 127u] > 0)
            held[ch][d1 & 127u]--;
    } else if (k == 0xB0u && d1 == 123u)
        memset(held[ch], 0, sizeof held[ch]);
}
static int held_on(int ch)
{
    int i, n = 0;
    for (i = 0; i < 128; i++)
        n += held[ch][i];
    return n;
}

static int32_t buf[2 * 256];
static int bad;
static double run(float secs)                    /* played secs: the output's RMS */
{
    const int blocks = (int)(secs * 44100.0f / 256.0f);
    int b, j;
    double s = 0.0;
    for (b = 0; b < blocks; b++) {
        en_render(buf, 256, 2048);
        for (j = 0; j < 512; j++) {
            const double x = (double)buf[j] / 8388608.0;
            if (x != x)
                bad++;
            s += x * x;
        }
    }
    return sqrt(s / (blocks * 512.0 + 1e-9));
}

/* the layers: its level, its voice parameter, its own voices' count, its MIDI channel */
static const struct { const char *name; int lvl, voice, own, ch; } L[PT_NLAYER] = {
    {"Drone", P_CHORD, P_CVOICE, EN_NCV, 0}, {"Sequence", P_SEQ, P_SVOICE, EN_NSV, 1},
    {"Lead", P_LEAD, P_LVOICE, EN_NBV, 2}, {"Sequence 2", P_SEQ2, P_S2VOICE, EN_NS2V, 3},
};
static double play_lead(float secs)              /* the Lead's phrase, measured */
{
    static const int N[6] = {62, 65, 67, 69, 67, 60};
    int i, n = (int)(secs * 2.0f);
    double s = 0.0;
    for (i = 0; i < n; i++) {
        double a, b;
        en_lead_midi(N[i % 6], 1);
        a = run(0.4f);
        en_lead_midi(N[i % 6], 0);
        b = run(0.1f);
        s += a * a * 0.8 + b * b * 0.2;
    }
    return sqrt(s / (n > 0 ? n : 1));
}

int main(int argc, char **argv)
{
    const float secs = argc > 1 ? (float)atof(argv[1]) : 12.0f;
    int l, k, i;
    for (l = 0; l < PT_NLAYER; l++) {
        double own, d[40], lo = 99.0, hi = -99.0;
        int nd = 0;
        en_init();
        for (i = 0; i < PT_NLAYER; i++)
            en_set(L[i].lvl, i == l ? 70 : 0);
        en_set(P_CRACKLE, 0);
        en_play(1);
        run(4.0f);                               /* (the piece under way: the Drone swelled in) */
        own = l == PT_LEAD ? play_lead(secs) : run(secs);
        CHECK(own > 1e-4, "%s: its own voice silent (%g)", L[l].name, own);
        for (k = 0; k < PT_N[l]; k++) {
            double r, db;
            en_set(L[l].voice, L[l].own + k);
            run(l == PT_DRONE ? 3.0f : 0.5f);     /* (the new sound in: a pad's attack) */
            r = l == PT_LEAD ? play_lead(secs) : run(secs);
            db = 20.0 * log10((r + 1e-12) / (own + 1e-12));
            printf("twr: %-10s %-9s %+6.1f dB  (level %d %d %+.2f)\n", L[l].name, pt_name(l, k), db, l, k, db);
            CHECK(fabs(db) < 12.0, "%s %s: %+.1f dB against its own voice (tools/pt_levels.py levels them)", L[l].name,
                  pt_name(l, k), db);
            if (nd < 40)
                d[nd++] = db;
            lo = db < lo ? db : lo;
            hi = db > hi ? db : hi;
        }
        {   /* the median */
            int a, b;
            for (a = 0; a < nd; a++)
                for (b = a + 1; b < nd; b++)
                    if (d[b] < d[a]) {
                        const double t = d[a];
                        d[a] = d[b], d[b] = t;
                    }
            printf("twr: %s: Paleotone's %d sounds against its own: median %+.1f dB, %+.1f .. %+.1f\n", L[l].name,
                   PT_N[l], nd ? d[nd / 2] : 0.0, lo, hi);
        }
        for (k = 0; k < 6; k++) {                /* back and forth while it plays: own, Paleotone's, own */
            en_set(L[l].voice, k & 1 ? L[l].own + (k * 5) % PT_N[l] : k / 2);
            if (l == PT_LEAD)
                play_lead(1.0f);
            else
                run(1.0f);
        }
        en_play(0);
        run(3.0f);
        for (k = 0; k < 16; k++)                 /* (the Lead's last key let go: the keys hold nothing) */
            en_lead_midi(60 + k, 0);
        run(3.0f);
        for (i = 0; i < 4; i++)
            CHECK(held_on(i) == 0, "%s: stopped, %d notes still on MIDI channel %d", L[l].name, held_on(i), i + 1);
        CHECK(pt_voices() == 0 || l == PT_DRONE, "%s: stopped, %d Paleotone voices still sounding", L[l].name, pt_voices());
        en_panic();
        run(0.5f);
        CHECK(pt_voices() == 0, "%s: after a panic, %d Paleotone voices sounding", L[l].name, pt_voices());
        memset(held, 0, sizeof held);
    }
    CHECK(!bad, "%d samples not a number", bad);
    printf("twr: %s\n", fails ? "FAIL" : "all ok");
    return fails != 0;
}
