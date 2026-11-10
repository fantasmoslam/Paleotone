/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS: Paleotone's sounds as the layers' voices (firmware/src/pt/pt.c): every sound of every layer's list is a
 * preset Paleotone has, sounds when played and goes quiet after; its level near the others'; a note's gate lets it go.
 *   pt_test [OUTDIR]   OUTDIR: a WAV of each layer's sounds in turn */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../../firmware/src/pt/pt.h"
static float buf[44100 * 2];
static void wav(const char *path, const float *x, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    uint32_t i, v;
    if (!f)
        return;
    fwrite("RIFF", 1, 4, f);
    v = 36 + n * 2; fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f);
    { uint16_t h[2] = {1, 1}; fwrite(h, 2, 2, f); }
    v = 44100; fwrite(&v, 4, 1, f);
    v = 88200; fwrite(&v, 4, 1, f);
    { uint16_t h[2] = {2, 16}; fwrite(h, 2, 2, f); }
    fwrite("data", 1, 4, f);
    v = n * 2; fwrite(&v, 4, 1, f);
    for (i = 0; i < n; i++) {
        float s = x[i] * 0.5f;
        int16_t q = (int16_t)(s > 1.0f ? 32767 : s < -1.0f ? -32767 : s * 32767.0f);
        fwrite(&q, 2, 1, f);
    }
    fclose(f);
}
int main(int argc, char **argv)
{
    static float all[PT_NLAYER][44100 * 2 * 34];
    static const char *const LN[PT_NLAYER] = {"drone", "seq", "lead", "seq2"};
    int l, k, fails = 0;
    pt_init();
    for (l = 0; l < PT_NLAYER; l++) {
        double lo = 1e9, hi = 0;
        for (k = 0; k < PT_N[l]; k++) {
            uint32_t j, n = 44100 * 2, gate = l == PT_SEQ || l == PT_SEQ2 ? 4000u : 0u;
            double e = 0, tail = 0;
            static const int NOTE[4] = {50, 57, 62, 53};
            pt_sound(l, k);
            if (pt_is(l) != k + 1) {
                printf("pt: %s %d %s: not loaded (no such preset)\n", LN[l], k, pt_name(l, k));
                fails++;
                continue;
            }
            memset(buf, 0, sizeof buf);
            for (j = 0; j < n; j += 441) {          /* (10 ms at a time: notes at their times) */
                uint32_t t = j / 441;               /* (in 10 ms) */
                if (l == PT_DRONE && t == 0) {
                    pt_note_on(l, 50, 90, 0);
                    if (!pt_mono(l, k))
                        pt_note_on(l, 57, 90, 0), pt_note_on(l, 62, 90, 0);
                } else if (l == PT_DRONE && t == 120)
                    pt_notes_off(l);
                else if (l == PT_LEAD && t % 30 == 0 && t < 120) {
                    if (t)
                        pt_note_off(l, NOTE[(t / 30 - 1) % 4] + 12);
                    pt_note_on(l, NOTE[(t / 30) % 4] + 12, 100, 0);
                } else if (l == PT_LEAD && t == 120)
                    pt_note_off(l, NOTE[3] + 12);
                else if ((l == PT_SEQ || l == PT_SEQ2) && t % 15 == 0 && t < 120)
                    pt_note_on(l, NOTE[(t / 15) % 4] + (l == PT_SEQ2 && k < 12 ? -12 : 12), 100, gate);
                pt_render(l, buf + j, 441);
            }
            for (j = 0; j < 44100 * 6 / 5; j++)
                e += buf[j] * buf[j];
            for (j = n - 4410; j < n; j++)
                tail += buf[j] * buf[j];
            e = sqrt(e / (44100 * 6 / 5));
            tail = sqrt(tail / 4410);
            if (e < lo) lo = e;
            if (e > hi) hi = e;
            printf("pt: %-5s %2d %-13s rms %.3f (%+5.1f dB)  tail %.4f  voices %d\n", LN[l], k, pt_name(l, k), e,
                   20 * log10(e + 1e-9), tail, pt_voices());
            if (e < 0.003) {
                printf("pt: %s %s: silent\n", LN[l], pt_name(l, k));
                fails++;
            }
            memcpy(all[l] + (size_t)k * n, buf, n * sizeof(float));
            pt_quiet(l);
        }
        pt_sound(l, -1);
        printf("pt: %s: %d sounds, rms %.1f .. %.1f dB\n", LN[l], PT_N[l], 20 * log10(lo + 1e-9), 20 * log10(hi + 1e-9));
        if (argc > 1) {
            char p[512];
            snprintf(p, sizeof p, "%s/pt-%s.wav", argv[1], LN[l]);
            wav(p, all[l], (uint32_t)PT_N[l] * 44100 * 2);
        }
    }
    printf("pt: %s\n", fails ? "FAIL" : "all ok");
    return fails != 0;
}
