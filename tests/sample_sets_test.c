/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX: its own sets JUNGLE, STEPS, STONE, STORM (SET 8..11, GHOULBOX 1.1's places): each zone sounds, the looped
 * ones hold without a click at the seam, the struck ones decay to silence, the four fit their flash budget */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
static void check(const char *what, int ok) { printf("sets: %-70s %s\n", what, ok ? "ok" : "FAIL"); bad += !ok; }

/* SET value v, note n held `hold` s then released, `tail` s more: the output (mono, int32) */
static int32_t *render(uint32_t v, uint32_t n, double hold, double tail, uint32_t *len)
{
    uint32_t t, i, frames = (uint32_t)((hold + tail) * FS), off = (uint32_t)(hold * FS);
    int32_t o[2 * CTL], *x = calloc(frames + CTL, sizeof *x);
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    host_preset(&trk[0], ENGI_SAMPLE, 0);
    trk[0].p[P_E0] = (int16_t)v;
    trk[0].p[P_REV] = 0; trk[0].p[P_DLY] = 0; trk[0].p[P_CHOR] = 0;
    trk_note_on(&trk[0], n, 100);
    for (t = 0; t < frames; t += CTL) {
        if (t >= off && t < off + CTL) trk_note_off(&trk[0], n);
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++) x[t + i] = o[2 * i];
    }
    *len = frames;
    return x;
}
static double rms(const int32_t *x, uint32_t a, uint32_t b)
{
    double s = 0;
    for (uint32_t i = a; i < b; i++) s += (double)x[i] * x[i];
    return sqrt(s / (b - a));
}
/* the largest sample-to-sample step over [a, b) against the typical one: a loop click is an outlier */
static double click_ratio(const int32_t *x, uint32_t a, uint32_t b)
{
    double mx = 0, s = 0;
    for (uint32_t i = a + 1; i < b; i++) {
        double d = fabs((double)x[i] - x[i - 1]);
        if (d > mx) mx = d;
        s += d * d;
    }
    return mx / sqrt(s / (b - a - 1) + 1e-9);
}

int main(void)
{
    /* DINOBOX: its own sets in GHOULBOX 1.1's places: kind 1 looped, 0 one-shot, 2 a kit (one sound per key) */
    static const struct { uint32_t v; const char *name; int kind; uint32_t nz; } SET[] = {
        {8, "JUNGLE", 1, 1}, {9, "STEPS", 2, 13}, {10, "STONE", 0, 2}, {11, "STORM", 1, 1}};
    uint32_t k, z, len, bytes = 0;
    for (k = 0; k < 4u; k++) {
        uint32_t s = SET[k].v < SMP_NALL ? smp_set_at(SET[k].v) : SMP_NONE;   /* (a value past the list: no such set) */
        const smp_set_t *set = &SMP_SETS[s == SMP_NONE ? 0u : s];
        int ok = s != SMP_NONE && str_eq(SMP_ALL_NAMES[SET[k].v], SET[k].name) && set->nz == SET[k].nz;
        if (!ok) { check(SET[k].name, 0); continue; }
        for (z = 0; ok && z < set->nz; z++) {               /* every zone sounds at its root (a kit: its key) */
            const smp_zone_t *zn = &SMP_ZONES[set->z0 + z];
            uint32_t n = SET[k].kind == 2 ? zn->lo : (uint32_t)(zn->root16 / 16);
            int32_t *x = render(SET[k].v, n, 1.0, 0.5, &len);
            double lv = rms(x, 0, FS / 4);
            if (SET[k].kind == 2)
                printf("sets:   %s key %u: rms %.0f\n", SET[k].name, n, lv);
            ok &= lv > 150.0 && (SET[k].kind != 2 || (zn->lo == zn->hi && zn->lo == 53u + z));
            bytes += (zn->n + 1u) / 2u;
            free(x);
        }
        check(SET[k].name, ok);
        if (SET[k].kind == 1) {                              /* held 8 s: past several loop seams, no click */
            int32_t *x = render(SET[k].v, (uint32_t)(SMP_ZONES[set->z0].root16 / 16), 8.0, 0.2, &len);
            double r = click_ratio(x, FS, 8 * FS);
            printf("sets:   %s click ratio %.1f\n", SET[k].name, r);
            check("  held 8 s: no click at the loop seam (ratio < 12), still sounding", r < 12.0 && rms(x, 7 * FS, 8 * FS) > 300.0);
            free(x);
        } else {                                             /* struck: sounds, then silence */
            const smp_zone_t *zn = &SMP_ZONES[set->z0];
            int32_t *x = render(SET[k].v, SET[k].kind == 2 ? zn->lo : 62u, 3.0, 0.5, &len);
            check("  struck: decays to silence", rms(x, 0, FS / 5) > 300.0 && rms(x, len - FS / 5, len) < 30.0);
            free(x);
        }
    }
    printf("sets: the four use %u B of flash\n", bytes);
    check("the four fit their budget (<= 110 KB: GHOULBOX's four were 114 KB)", bytes <= 110u * 1024u);
    check("1.0's values unchanged: PIANO 0, FLUTE 2, SAX 3, USR1 5",
          str_eq(SMP_ALL_NAMES[0], "PIANO") && str_eq(SMP_ALL_NAMES[2], "FLUTE") && str_eq(SMP_ALL_NAMES[3], "SAX") &&
          str_eq(SMP_ALL_NAMES[5], "USR1") && smp_set_at(2) == 2u);
    puts(bad ? "SAMPLE SETS TEST FAILED" : "sample sets passed");
    return bad != 0;
}
