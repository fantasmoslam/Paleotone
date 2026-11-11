/* SPDX-License-Identifier: GPL-3.0-only */
/* New selectable voices: useful registers, finite sustained audio, key release and panic. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/pt/pt.h"
static float b[256];
static int render(int layer, int blocks, double *energy, double *peak, FILE *raw)
{
    int i, j;
    for (i = 0; i < blocks; i++) {
        memset(b, 0, sizeof b);
        pt_render(layer, b, 256);
        for (j = 0; j < 256; j++) {
            double x = b[j];
            if (!isfinite(x)) return 1;
            *energy += x*x;
            if (fabs(x) > *peak) *peak = fabs(x);
        }
        if (raw) fwrite(b, sizeof(float), 256, raw);
    }
    return 0;
}
int main(int argc, char **argv)
{
    static const int first[4] = {26, 22, 18, 34};
    int l, k, n, fails = 0;
    pt_init();
    for (l = 0; l < 4; l++) for (k = 0; k < 4; k++) {
        for (n = k < 2 ? 36 : 24; n <= 72; n += 12) {
            double e = 0, pk = 0, tail = 0, tailpk = 0;
            FILE *raw = NULL;
            char path[1024];
            pt_quiet(l); pt_sound(l, first[l]+k);
            if (pt_is(l) != first[l]+k+1) return 2;
            if (argc > 1 && l == PT_LEAD && n == (k < 2 ? 36 : 24)) {
                snprintf(path, sizeof path, "%s/instrument-%d.f32", argv[1], k);
                raw = fopen(path,"wb"); if (!raw) return 3;
            }
            pt_note_on(l, n, 100, 0);
            fails += render(l, 690, &e, &pk, raw);
            pt_note_off(l, n);
            fails += render(l, 1723, &tail, &tailpk, NULL);
            if (raw) fclose(raw);
            if (e < .01 || pk > 1.5 || pt_voices()) {
                printf("FAIL layer %d %s note %d energy %.3f peak %.3f held %d\n",
                       l, pt_name(l,first[l]+k), n, e, pk, pt_voices());
                fails++;
            }
        }
        pt_note_on(l, 48, 127, 0);
        pt_quiet(l);
        { double e = 0, p = 0; fails += render(l, 2, &e, &p, NULL); if (e || pt_voices()) fails++; }
        printf("instrument layer %d %s: checked\n", l, pt_name(l,first[l]+k));
    }
    printf("instruments: %s\n", fails ? "FAIL" : "all passed");
    return fails != 0;
}
