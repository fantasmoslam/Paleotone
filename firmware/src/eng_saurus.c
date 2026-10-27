/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 DINOBOX (on GHOULBOX by Jason Persinger and Felucca by Leo Kuroshita, Hügelton Instruments) */
/* SAURUS (DINOBOX, PALEOTONE 2): a creature's voice, made like a film roar: up to three throats at once (a low rumble, the
 * body, a scream or a whistle), each with its own pitch and loudness over the call, the way an alligator, a lion and
 * an elephant are layered. One note is one call; CALL picks the recipe (SAUR_CALL: which throats, their pitches above
 * or below the note, resonances, roughness), the knobs scale it (64: the recipe as made):
 *   SIZE   the throats' resonances, x2 (0) .. x0.5 (127), key tracking a quarter
 *   SWOOP  how far the recipe's pitch curves move (64: as made, 127 twice)
 *   LEN    the call's length (1 ms .. 10 s, as before; WILD makes each its own)
 *   GROWL  the roughness: the throat's bursts or flutter, the weak every other period (a subharmonic), the drive
 *   BRTH   the breath and the scream's noise
 *   MOUTH  how far the mouth opens (the resonances from their closed to their open place); a harder note opens it
 *   WILD   the wander: each period's length and loudness, the pitch's slow drift, each call's pitch and length
 * A throat, a sample at a time: a band-limited saw whose periods each draw their own gain and length (jitter,
 * shimmer, the subharmonic), darkened by a one-pole tilt, with breath noise in each period's opening; gated by the
 * growl (a flutter, or bursts that open fast and die away); three resonators side by side (peak gain 1, weights 1,
 * 0.6, 0.35) between closed and open; a soft clip (its drive); a two-pole high or low cut. Its loudness and pitch follow
 * the recipe's six points over LEN. The ADSR still shapes and ends the note.
 * State: the throats and their exact coefficient caches in a pool of SAUR_SLOTS (a voice takes one at its note-on, the oldest if none is
 * free: .poly 4, so only when four tracks play SAURUS at once); s[0] the voice's slot; s[7] the call's progress (Q24,
 * bits 0..24) and the note's random number (bits 25..31). */

enum { SC_HOLD, SC_BELLOW, SC_ROAR, SC_BARK, SC_HOOT, SC_TRILL, SC_CHIRP, SC_MOAN, SC_HISS, SC_SNORT, SC_SHRIEK, SC_HONK,
       SC_BOOM, SC_RATTLE, SC_COUNT };
static const char *const N_SAUR_CALL[SC_COUNT] = {"HOLD", "BELLOW", "ROAR", "BARK", "HOOT", "TRILL", "CHIRP", "MOAN",
                                                  "HISS", "SNORT", "SHRIEK", "HONK", "BOOM", "RATTLE"};   /* (stored values:
                                                                                                    * append only) */
#define SAUR_NL 3u
typedef struct {
    int8_t off;                  /* semitones from the note */
    uint8_t lvl;                 /* its share (255 = x1) */
    uint8_t fc[3], fo[3], bw[3]; /* resonances closed and open (Hz / 16), their widths (Hz / 4) */
    uint8_t bright, breath, sub, growl, grate, burst, gfloor;   /* tilt; noise; the weak period; the growl's depth, rate
                                  * (Hz), bursts (their decay x 16; 0: a flutter), the floor between bursts (/255) */
    uint8_t jit, shim, drive, hp, lp, vibr, vibd;   /* jitter (0.1 %), shimmer, drive (x / 16), high cut (Hz / 8), low
                                  * cut (Hz / 32; 0 none), vibrato (Hz x 4, cents / 2) */
    uint8_t amp[6];              /* loudness at the call's six times (255 = full) */
    int8_t pit[6];               /* pitch at them, quarter semitones (x SWOOP / 64) */
} slayer_t;
typedef struct {
    uint8_t t[6];                /* the six times, of LEN (255 = the end) */
    uint8_t mouth[6];            /* the mouth at them (255 open, x MOUTH / 64) */
    uint8_t nl;                  /* throats */
    uint8_t g;                   /* the output's gain (16 = x1: as loud as SAURUS's calls before) */
    slayer_t l[SAUR_NL];
} scall_t;
/* the recipes (tools/saurus_design.py has them as made, by ear against the CC0 recordings the 2026-10-24 work used) */
#define SAUR_NONE {0, 0, {0}, {0}, {0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}, {0}}
#include "saurus_calls.h"
_Static_assert(NELEM(SAUR_CALL) == SC_COUNT, "a recipe for every CALL");

#define SAUR_SLOTS 4u
typedef struct {
    uint32_t ph, gs, gph, vph;   /* the saw; the period's gain (0..14), weak toggle (15), jitter (16..23); growl; vibrato */
    int32_t lp, lq, y[6], f1, f2; /* tilt (two poles: a throat's -12 dB an octave); resonators; the cut */
    int32_t gate, wf, wg, am;    /* the growl's gate (Q15) at the block's end; the drifts (pitch, the growl's); the
                                  * loudness at the block's end (Q15; -1 none yet) */
    fres_t res[3];             /* exact coefficients, keyed by their clamped inputs below */
    uint32_t res_f[3], res_bw[3]; /* zero frequency is invalid; real inputs are at least 30 * 16 */
} sthroat_t;
static struct {
    const voice_t *owner;
    uint32_t age;
    sthroat_t l[SAUR_NL];
} saur_slot[SAUR_SLOTS];
static uint32_t saur_age;

static uint32_t saur_shape(const track_t *t) { return (uint32_t)clamp(t->p[P_E0], 0, SC_COUNT - 1); }

/* a recipe curve at progress q (Q24), its times tm (/255); u (0..255) or s (signed); Q8 */
static int32_t saur_at(const uint8_t *tm, const uint8_t *u, const int8_t *s, uint32_t q)
{
    uint32_t i, x = q >> 16;                            /* 0..256 */
    for (i = 0; i < 5u && x >= tm[i + 1u]; i++)
        ;
    {
        int32_t a = u ? u[i] : s[i], b = i < 5u ? (u ? u[i + 1u] : s[i + 1u]) : a;
        int32_t d = i < 5u ? (int32_t)tm[i + 1u] - (int32_t)tm[i] : 0, f = d > 0 ? (((int32_t)x - tm[i]) << 8) / d : 0;
        return (a << 8) + (b - a) * clamp(f, 0, 256);
    }
}

/* a resonator of peak gain 1 at F (Hz * 16), width BW (Hz * 16): Q30 */
static void saur_res(fres_t *r, uint32_t f16, uint32_t bw16)
{
    int32_t s = sine_i(f16 * 3044u), x = (int32_t)(bw16 * 4781u);
    int32_t x2 = (int32_t)(((int64_t)x * x) >> 30), x3 = (int32_t)(((int64_t)x2 * x) >> 30);
    int32_t e = (1 << 30) - x + (x2 >> 1) - (int32_t)(((int64_t)x3 * 178956971) >> 30);
    int32_t cs = (1 << 30) - 2 * s * s;
    r->b = (int32_t)(((int64_t)e * cs) >> 29);
    r->c = -(int32_t)(((int64_t)e * e) >> 30);
    r->a = (int32_t)(((int64_t)((1 << 30) - e) * s) >> 14);   /* (1 - r) 2 sin(w): the peak at 1 */
}

static void saurus_note_on(track_t *t, voice_t *v)
{
    uint32_t k = (uint32_t)v->s[0], i, best = 0;
    (void)t;
    v->s[7] = (int32_t)((noise32(&formant_nz) >> 25) << 25);   /* the call starts; this note's random number */
    if (k < SAUR_SLOTS && saur_slot[k].owner == v && (v->env || v->env_out))
        return;                                         /* a retrigger: its throats go on (no click) */
    for (i = 0; i < SAUR_SLOTS; i++) {                  /* a free slot, else the oldest */
        if (!saur_slot[i].owner || !saur_slot[i].owner->active || saur_slot[i].owner == v) {
            best = i;
            break;
        }
        if (saur_slot[i].age < saur_slot[best].age)
            best = i;
    }
    memset(&saur_slot[best], 0, sizeof saur_slot[best]);
    saur_slot[best].owner = v;
    saur_slot[best].age = ++saur_age;
    for (i = 0; i < SAUR_NL; i++) {
        saur_slot[best].l[i].gs = 32767u;
        saur_slot[best].l[i].gate = 32767;
        saur_slot[best].l[i].am = -1;
    }
    v->s[0] = (int32_t)best;
}

static void saurus_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    const scall_t *c = &SAUR_CALL[saur_shape(t)];
    uint32_t k = (uint32_t)v->s[0], q = (uint32_t)v->s[7] & FMT_TALK_MASK, rnd = (uint32_t)v->s[7] >> 25, li, i, j;
    int32_t wild = p[P_E7], acc[CTL], mouth, size, roff;
    int32_t kg = p[P_E4], kb = p[P_E5], ks = p[P_E2];
    int32_t nst = formant_nz;
    if (k >= SAUR_SLOTS || saur_slot[k].owner != v || n > CTL)
        return;                                         /* (its slot taken by a newer call: silent until it ends) */
    if (q < (1u << 24)) {
        q += ENV_LIN[clamp(p[P_E3] + ((((int32_t)rnd - 64) * wild) >> 8), 0, 127)];
        if (q > (1u << 24))
            q = 1u << 24;
        v->s[7] = (int32_t)(((uint32_t)v->s[7] & ~(uint32_t)FMT_TALK_MASK) | q);
    }
    mouth = clamp(((saur_at(c->t, c->mouth, 0, q) >> 8) * p[P_E6]) >> 6, 0, 255);
    mouth = clamp(mouth + (m->cutoff >> 8) - 64 + ((v->vel - 64) >> 1) + 64, 0, 255);   /* (FLT mod; a harder note) */
    size = (int32_t)fmt_ratio((64 - p[P_E1]) * 3 + ((v->pitch_cur - 60 * 16) >> 2));   /* Q16 */
    roff = (((int32_t)rnd - 64) * wild) >> 8;           /* this call's pitch, +-2 semitones at WILD 127 */
    for (i = 0; i < n; i++)
        acc[i] = 0;
    for (li = 0; li < c->nl; li++) {
        const slayer_t *L = &c->l[li];
        sthroat_t *z = &saur_slot[k].l[li];
        int32_t amp0, amp1, p16, g0, g1, gstep, tilt, gn, sub, shim, jp, dr, kc, vib;
        uint32_t inc, incj, ph = z->ph, gs = z->gs;
        int32_t lp = z->lp, lq = z->lq, y0 = z->y[0], y1 = z->y[1], y2 = z->y[2], y3 = z->y[3], y4 = z->y[4], y5 = z->y[5];
        int32_t f1 = z->f1, f2 = z->f2;
        fres_t r[3];
        uint64_t x64;
        amp1 = ((saur_at(c->t, L->amp, 0, q) >> 8) * L->lvl) >> 1;   /* Q15 */
        amp0 = z->am < 0 ? amp1 : z->am;
        z->am = amp1;
        /* drifts: the pitch slow, the growl's quicker (smoothed noise, +-1 Q15) */
        z->wf += ((((int32_t)(noise32(&nst) >> 16) - 32768) - z->wf) >> 7);
        z->wg += ((((int32_t)(noise32(&nst) >> 16) - 32768) - z->wg) >> 4);
        z->vph += (uint32_t)L->vibr * 24348u * n;       /* (Hz x 4: 2^32 / 44100 / 4) */
        vib = (int32_t)(((int64_t)sine_i(z->vph) * L->vibd * 10) >> 20);
        p16 = L->off * 16 + (((saur_at(c->t, 0, L->pit, q) >> 8) * 4 * ks) >> 6) + vib + roff +
              (int32_t)((((int64_t)clamp(z->wf * 16, -32767, 32767)) * L->jit * 277 * (64 + wild)) >> 31);
        x64 = ((uint64_t)m->inc * fmt_ratio(p16)) >> 16;
        inc = x64 > 0x7FFFFFFFu ? 0x7FFFFFFFu : (uint32_t)x64;
        /* the resonances: closed -> open, x SIZE */
        for (j = 0; j < 3u; j++) {
            int32_t fz = ((L->fc[j] << 8) + (((int32_t)L->fo[j] - L->fc[j]) * mouth)) ;   /* Hz * 16 (Hz/16 << 8) */
            int32_t bz = L->bw[j] << 6;                 /* Hz * 16 */
            fz = (int32_t)(((int64_t)fz * size) >> 16);
            bz = (int32_t)(((int64_t)bz * size) >> 16);
            {
                uint32_t f = (uint32_t)clamp(fz, 30 * 16, 9000 * 16);
                uint32_t bw = (uint32_t)clamp(bz, 15 * 16, 1500 * 16);
                /* saur_res depends only on these inputs and immutable tables. New slots
                 * are zeroed by note_on; retriggers may reuse an exact match. */
                if (z->res_f[j] != f || z->res_bw[j] != bw) {
                    saur_res(&z->res[j], f, bw);
                    z->res_f[j] = f;
                    z->res_bw[j] = bw;
                }
                r[j] = z->res[j]; /* keep the sample loop's existing local coefficients */
            }
        }
        /* the growl's gate at the block's end */
        {
            int32_t gd = clamp((L->growl * kg) >> 6, 0, 127) * 258, rate = L->grate, sq;
            uint32_t gi = (uint32_t)(rate * 97391) + (uint32_t)(((int64_t)rate * 97391 * z->wg) >> 17);
            z->gph += gi * n;
            if (L->burst) {                             /* a burst: opens over 1/60 of its cycle, dies away */
                uint32_t fr = z->gph >> 16;             /* 0..65535 */
                int32_t e = (int32_t)fmt_ratio(-(int32_t)(((uint64_t)fr * L->burst * 17) >> 16)) >> 1;   /* Q15 */
                int32_t at = (int32_t)(fr * 16u > 65535u ? 32767u : fr * 16u / 2u);   /* (over 1/16: no click) */
                at = (at * at) >> 15;
                int32_t fl = L->gfloor * 128;
                sq = fl + (((((32767 - fl) * e) >> 15) * at) >> 15);
                g1 = 32767 - ((gd * (32767 - sq)) >> 15);
            } else {                                    /* a flutter: on and off, now and then skipped or doubled */
                sq = sine_i(z->gph) + ((z->wg * 3) >> 1) > 0 ? 32767 : 0;
                g1 = 32767 - ((gd * sq) >> 15);
            }
        }
        g0 = z->gate;
        gstep = (g1 - g0) / (int32_t)(n ? n : 1u);
        tilt = clamp(1600 + L->bright * 110 + (v->vel - 64) * 16, 900, 16000);
        gn = clamp((L->breath * kb) >> 6, 0, 160) * 200;
        sub = clamp((L->sub * kg) >> 6, 0, 127) * 200;
        shim = L->shim * 200;
        jp = L->jit * (32 + wild) >> 3;                 /* a period's length: +-(jit 0.1 %) */
        dr = 16 + ((L->drive * kg) >> 6);
        kc = L->lp ? clamp(L->lp * 149, 0, 30000) : clamp(L->hp * 37, 0, 30000);   /* (one-pole: 2 pi f / FS, Q15) */
        incj = inc + (uint32_t)(((int64_t)(inc >> 11) * (int8_t)(gs >> 16)) >> 0);
        for (i = 0; i < n; i++) {
            int32_t e, x, a, s, g = g0 + gstep * (int32_t)i, am = amp0 + (((amp1 - amp0) * (int32_t)i) >> CTL_LOG2);
            uint32_t nx;
            e = (osc_saw(ph, incj) * (int32_t)(gs & 0x7FFFu)) >> 15;
            lp += ((e - lp) * tilt) >> 14;
            lq += ((lp - lq) * tilt) >> 14;
            x = lq;
            if (gn) {
                int32_t nz = (((int32_t)(noise32(&nst) >> 16) - 32768) * gn) >> 15;
                x += ph < 0x66666666u ? nz : nz >> 2;   /* breath: in each period's opening */
            }
            x = (x * g) >> 15;
            a = (int32_t)(((int64_t)r[0].a * (x << 6) + (int64_t)r[0].b * y0 + (int64_t)r[0].c * y1) >> 30);
            y1 = y0;
            y0 = a;
            a = (int32_t)(((int64_t)r[1].a * (x << 6) + (int64_t)r[1].b * y2 + (int64_t)r[1].c * y3) >> 30);
            y3 = y2;
            y2 = a;
            a = (int32_t)(((int64_t)r[2].a * (x << 6) + (int64_t)r[2].b * y4 + (int64_t)r[2].c * y5) >> 30);
            y5 = y4;
            y4 = a;
            s = (y0 + ((y2 * 19661) >> 15) + ((y4 * 11469) >> 15)) >> 6;   /* Q15 */
            s = softclip_inline(clamp((s * dr) >> 4, -2000000, 2000000));
            f1 += ((s - f1) * kc) >> 15;
            f2 += ((f1 - f2) * kc) >> 15;
            s = L->lp ? f2 : L->hp ? s - f2 : s;
            acc[i] += (s * am) >> 15;
            nx = ph + incj;
            if (nx < ph) {                              /* a new period: its gain and length */
                uint32_t rr = noise32(&nst), tog = ((gs >> 15) & 1u) ^ 1u;
                int32_t gg = 32767 - (tog ? sub : 0) - (int32_t)(((rr & 0x7FFFu) * (uint32_t)shim) >> 15);
                int32_t pj = (((int32_t)((rr >> 16) & 255u) - 128) * jp) >> 10;
                pj = clamp(pj, -127, 127);
                gs = (uint32_t)clamp(gg, 0, 32767) | tog << 15 | ((uint32_t)pj & 255u) << 16;
                incj = inc + (uint32_t)(((int64_t)(inc >> 11) * pj));
            }
            ph = nx;
        }
        z->ph = ph;
        z->gs = gs;
        z->lp = lp;
        z->lq = lq;
        z->y[0] = y0; z->y[1] = y1; z->y[2] = y2; z->y[3] = y3; z->y[4] = y4; z->y[5] = y5;
        z->f1 = f1;
        z->f2 = f2;
        z->gate = g1;
    }
    formant_nz = nst;
    for (i = 0; i < n; i++)
        out[i] += voice_amp(soft_knee((acc[i] * c->g) >> 4, 26000), m, i) << 1;
}

static const preset_t SAURUS_PRESETS[] = {
    /* name, {CALL, SIZE, SWOOP, LEN | GROWL, BRTH, MOUTH, WILD}, {A D S R}, fenv, mono (tools/ghoulbox_presets.py) */
    /* GHOULBOX presets (tools/ghoulbox_presets.py) */
    {"BRACHIO CALL", {1, 104, 60, 110, 50, 60, 70, 25}, {30, 90, 127, 90}, 0, 0, FX(0, 20, 15, 110), PAT(17)},
    {"TYRANT ROAR", {2, 64, 64, 112, 64, 64, 64, 30}, {2, 90, 127, 80}, 0, 1, FX(15, 0, 10, 100), PAT(17)},
    {"RAPTOR BARK", {3, 64, 64, 92, 64, 64, 64, 40}, {0, 70, 127, 40}, 0, 0, FX(5, 0, 25, 85), PAT(4)},
    {"RAPTOR CHAT", {5, 64, 64, 98, 64, 64, 64, 60}, {1, 80, 110, 40}, 0, 0, FX(0, 0, 20, 85), PAT(4)},
    {"CREST HOOT", {4, 70, 50, 104, 40, 50, 60, 15}, {40, 90, 127, 100}, 0, 0, FX(0, 30, 20, 115), PAT(17)},
    {"PTERO SHRIEK", {10, 30, 80, 98, 50, 70, 80, 50}, {0, 80, 110, 50}, 0, 0, FX(10, 0, 30, 95), PAT(4)},
    {"HERD MOAN", {7, 76, 64, 110, 64, 64, 64, 40}, {30, 90, 120, 100}, 0, 0, FX(0, 40, 10, 110), PAT(5)},
    {"FRILL HISS", {8, 64, 0, 96, 64, 64, 64, 30}, {4, 80, 110, 60}, 0, 0, FX(0, 0, 10, 80), PAT(17)},
    {"SAURO SNORT", {9, 90, 0, 88, 64, 64, 64, 30}, {0, 80, 127, 50}, 0, 0, FX(0, 0, 0, 90), PAT(4)},
    {"DEEP RUMBLE", {0, 110, 0, 0, 80, 40, 40, 20}, {80, 90, 127, 110}, 0, 0, FX(10, 30, 0, 110), PAT(17)},
    {"TITAN APE", {2, 80, 50, 104, 90, 70, 80, 30}, {4, 90, 127, 80}, 0, 1, FX(20, 0, 10, 95), PAT(17)},
    {"NEST CHIRPS", {6, 30, 64, 84, 40, 50, 64, 90}, {0, 70, 110, 40}, 0, 0, FX(0, 20, 30, 90), PAT(4)},
    {"SWAMP CROAK", {13, 50, 40, 100, 70, 40, 50, 40}, {6, 80, 120, 60}, 0, 0, FX(0, 0, 10, 90), PAT(8)},
    {"MONSTER ROAR", {2, 90, 70, 112, 100, 90, 80, 45}, {2, 90, 127, 70}, 0, 1, FX(40, 0, 20, 90), PAT(17)},
    {"DISTANT HERD", {7, 90, 50, 112, 50, 80, 50, 50}, {80, 90, 120, 120}, 0, 0, FX(0, 40, 25, 127), PAT(5)},
    {"LOW GROWL", {13, 80, 30, 116, 90, 60, 64, 40}, {20, 90, 127, 80}, 0, 1, FX(10, 0, 0, 90), PAT(17)},
    {"BABY REX", {2, 30, 64, 100, 50, 50, 70, 40}, {2, 90, 127, 60}, 0, 1, FX(5, 0, 20, 90), PAT(4)},
    {"SEA MONSTER", {1, 120, 50, 116, 100, 60, 50, 30}, {50, 90, 127, 120}, 0, 0, FX(10, 30, 20, 127), PAT(17)},
    {"HADRO HORN", {11, 64, 64, 110, 64, 64, 64, 20}, {30, 90, 127, 110}, 0, 0, FX(0, 20, 30, 120), PAT(17)},
    {"COMPY PACK", {3, 20, 64, 82, 50, 60, 64, 110}, {0, 64, 100, 30}, 0, 0, FX(0, 20, 30, 85), PAT(13)},
    {"RAPTOR CRY", {10, 64, 64, 100, 64, 64, 64, 40}, {0, 80, 120, 50}, 0, 0, FX(5, 0, 25, 90), PAT(4)},
    {"TITAN BOOM", {12, 64, 64, 114, 64, 64, 64, 20}, {60, 90, 127, 110}, 0, 0, FX(0, 20, 10, 120), PAT(17)},
    /* GHOULBOX end */
};

static const engine_t ENG_SAURUS = {
    .name = "SAURUS",
    .page_title = {"CALL", "THROAT"},
    .edit = {
        {"CALL", F_ENUM, 0, SC_COUNT - 1, SC_BELLOW, N_SAUR_CALL, 0},
        {"SIZE", F_INT, 0, 127, 80, 0, 0},
        {"SWOOP", F_INT, 0, 127, 40, 0, 0},
        {"LEN", F_TIME, 0, 127, 100, 0, 0},
        {"GROWL", F_PCT, 0, 127, 40, 0, 0},
        {"BRTH", F_PCT, 0, 127, 30, 0, 0},
        {"MOUTH", F_INT, 0, 127, 100, 0, 0},
        {"WILD", F_PCT, 0, 127, 30, 0, 0},
    },
    .presets = SAURUS_PRESETS,
    .npresets = NELEM(SAURUS_PRESETS),
    .note_on = saurus_note_on,
    .render = saurus_render,
    .knob = {P_E1, P_E4, P_E2, P_E3},
    .poly = 4,
};
/* its favourites are bytes 0..9 of favorites.factory[15], below the settings there (settings_persist.c): 80 sounds */
_Static_assert(NELEM(SAURUS_PRESETS) <= 80u, "SAURUS's favourites fit bytes 0..9 of favorites.factory[15]");
