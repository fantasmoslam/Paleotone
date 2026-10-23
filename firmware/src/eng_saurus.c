/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 DINOBOX (on GHOULBOX by Jason Persinger and Felucca by Leo Kuroshita, Hügelton Instruments) */
/* SAURUS (DINOBOX): a creature's voice. One note is one call: a buzzing throat source, roughened (GROWL), with breath
 * (BRTH), through three throat formants scaled by the animal's SIZE and opened by its MOUTH, while the CALL shape moves
 * the pitch (SWOOP deep), the mouth and the loudness over LEN after the note-on. WILD makes each call its own.
 *
 *   source  a band-limited saw, darkened by a one-pole tilt (GROWL and velocity open it). Each period draws its own
 *           gain and its next period's length at the wrap: GROWL makes every other period weaker (a subharmonic an
 *           octave down: the fry of a big throat), GROWL and WILD jitter both (shimmer and jitter: a rough voice).
 *           BRTH mixes noise in, louder in each period's first half (aspiration); HISS and SNORT are the noise alone
 *   throat  F1 -> F2 -> F3 in cascade (eng_formant.c's Klatt resonators and pitch ratios), from a closed "oo" through
 *           "oh" to an open "ah"; SIZE moves them from an octave up (a small animal) to two octaves down (a sauropod),
 *           key tracking a quarter; GROWL widens their bands (rougher, less ringing)
 *   call    CALL's shape: five points each of pitch, mouth and loudness over LEN, linear between, the last one held.
 *           SWOOP is the pitch points' depth (16 = two octaves at 127); TRILL flutters the loudness at 18..26 Hz.
 *           WILD: this note's call a little longer or shorter and up to two semitones off (the note's random number)
 *   out     the last formant with its presence (as VOICE), then GROWL's drive: a soft clip blended in
 * The loudness of the call multiplies the ADSR (engine_t.amp), so the ADSR still shapes and ends the note.
 *
 * Voice state: ph[0] the phase; ph[1] the period's gain (bits 0..14, Q15), the subharmonic's toggle (bit 15) and the
 * period's pitch jitter (bits 16..23, signed); ph[2] TRILL's phase (GROWL's flutter, saurus_amp); s[0..5] F1..F3 y1 / y2; s[6] the tilt; s[7] the
 * call's progress (Q24, bits 0..24) and the note's random number (bits 25..31). */

enum { SC_HOLD, SC_BELLOW, SC_ROAR, SC_BARK, SC_HOOT, SC_TRILL, SC_CHIRP, SC_MOAN, SC_HISS, SC_SNORT, SC_COUNT };
static const char *const N_SAUR_CALL[SC_COUNT] = {"HOLD", "BELLOW", "ROAR", "BARK", "HOOT", "TRILL", "CHIRP", "MOAN",
                                                  "HISS", "SNORT"};   /* (stored values: append only) */
#define SCF_UNVOICED 1u
#define SCF_TRILL 2u
/* pitch (in 16ths of SWOOP's depth), mouth (16 = MOUTH), loudness (16 = full), at 0, 1/4 .. 1 of LEN; flags; the
 * output gain (16 = x1: low noise loses most of its energy in the throat) */
static const struct { int8_t p[5]; uint8_t m[5], a[5], f, g; } SAUR_CALL[SC_COUNT] = {
    {{0, 0, 0, 0, 0}, {16, 16, 16, 16, 16}, {16, 16, 16, 16, 16}, 0, 16},            /* HOLD: as played (drones) */
    {{0, 4, 2, -4, -12}, {2, 10, 16, 14, 8}, {5, 14, 16, 14, 9}, 0, 16},             /* BELLOW: swells, rises a little, sinks */
    {{6, 4, 0, -8, -16}, {16, 16, 14, 10, 5}, {16, 16, 14, 10, 4}, 0, 16},           /* ROAR: wide open at once, falling */
    {{8, 0, -8, -12, -16}, {16, 12, 6, 2, 0}, {16, 9, 3, 0, 0}, 0, 16},              /* BARK: short; it ends by itself */
    {{-2, 0, 0, 0, -2}, {0, 1, 1, 1, 0}, {6, 16, 16, 14, 10}, 0, 16},                /* HOOT: a closed mouth, steady */
    {{0, 1, 0, 1, 0}, {8, 12, 12, 12, 8}, {16, 16, 16, 16, 12}, SCF_TRILL, 16},      /* TRILL: fluttering */
    {{-12, 8, 12, 6, 0}, {4, 16, 16, 12, 8}, {10, 16, 12, 6, 0}, 0, 16},             /* CHIRP: a fast rising squeal */
    {{2, 0, -4, -8, -10}, {6, 8, 8, 6, 4}, {8, 14, 16, 14, 10}, 0, 16},              /* MOAN: a slow falling groan */
    {{0, 0, 0, 0, 0}, {16, 16, 14, 12, 10}, {6, 16, 14, 10, 6}, SCF_UNVOICED, 16},   /* HISS */
    {{0, 0, 0, 0, 0}, {6, 4, 2, 0, 0}, {16, 8, 2, 0, 0}, SCF_UNVOICED, 45},          /* SNORT: a short burst */
};
/* the throat's three formants (Hz) and bandwidths (Hz): a closed "oo", "oh", an open "ah" */
static const uint16_t SAUR_F[3][3] = {{300, 870, 2240}, {500, 900, 2400}, {760, 1250, 2500}};
static const uint8_t SAUR_B[3][3] = {{60, 80, 150}, {70, 80, 150}, {90, 100, 160}};
#define SAUR_TRILL_INC 1753064u                           /* 18 Hz a sample (2^32 * 18 / FS) */
#define SAUR_GROWL_INC 19784u                             /* the flutter: 26 Hz a control tick (2^20 * 26 * CTL / FS) */
#define SAUR_GROWL_UP 10u                                 /* its envelope (4095 full) a tick: in ~0.3 s .. */
#define SAUR_GROWL_DN 6u                                  /* .. out ~0.5 s */

static uint32_t saur_shape(const track_t *t) { return (uint32_t)clamp(t->p[P_E0], 0, SC_COUNT - 1); }

/* a call curve at progress q (Q24): five points (signed s or unsigned u), linear between, the last held; Q8 */
static int32_t saur_pt(const int8_t *s, const uint8_t *u, uint32_t q)
{
    uint32_t i = q >= (1u << 24) ? 4u : q >> 22, fr = q >= (1u << 24) ? 0u : (q >> 14) & 255u;
    int32_t a = s ? s[i] : u[i], b = i < 4u ? (s ? s[i + 1u] : u[i + 1u]) : a;
    return (a << 8) + (b - a) * (int32_t)fr;
}

static void saurus_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->s[7] = (int32_t)((noise32(&formant_nz) >> 25) << 25);   /* the call starts; this note's random number */
    if (!v->env && !v->env_out) {                       /* from rest (not a retrigger: its states go on, no click) */
        uint32_t i;
        v->ph[0] = 0;
        v->ph[1] = 32767u;                              /* the first period at full gain, no jitter */
        v->ph[2] = 0;
        for (i = 0; i < 7u; i++)
            v->s[i] = 0;
    }
}

/* the voice's loudness: the ADSR x the call's loudness (x TRILL's flutter), once per control tick */
static int32_t saurus_amp(track_t *t, voice_t *v, int32_t adsr)
{
    uint32_t sh = saur_shape(t), q = (uint32_t)v->s[7] & FMT_TALK_MASK;
    int32_t a = (int32_t)(((int64_t)adsr * saur_pt(0, SAUR_CALL[sh].a, q)) >> 12);   /* (16 << 8: x1) */
    if (SAUR_CALL[sh].f & SCF_TRILL) {                  /* depth 70 %; WILD's random number sets this note's rate */
        v->ph[2] += (SAUR_TRILL_INC + ((uint32_t)v->s[7] >> 25) * 6000u) * CTL;
        a = (a * (32767 - ((((sine_i(v->ph[2]) + 32768) >> 1) * 22938) >> 15))) >> 15;
    } else if (t->p[P_E4] > 64) {                       /* DINOBOX: GROWL above 64, the throat's flutter (a deep rolled
                                                         * R): ~26 Hz, uneven; in over ~0.3 s while the call is loud and
                                                         * the key held, out over ~0.5 s as it drops (the bellow rings on
                                                         * without it). ph[2]: the flutter's envelope (bits 20..31), its
                                                         * phase (0..19) */
        uint32_t st = v->ph[2], ge = st >> 20, fp = st & 0xFFFFFu;
        int32_t gd = clamp((t->p[P_E4] - 64) * 520, 0, 32767), depth, chop;
        if (v->gate && saur_pt(0, SAUR_CALL[sh].a, q) >= (8 << 8))
            ge = ge + SAUR_GROWL_UP < 4095u ? ge + SAUR_GROWL_UP : 4095u;
        else
            ge = ge > SAUR_GROWL_DN ? ge - SAUR_GROWL_DN : 0u;
        fp = (fp + SAUR_GROWL_INC + (((noise32(&formant_nz) >> 20) - 2048u) << 1)) & 0xFFFFFu;
        v->ph[2] = ge << 20 | fp;
        chop = (sine_i(fp << 12) + 32768) >> 1;
        chop = (chop * chop) >> 15;
        depth = (int32_t)((gd * (int32_t)((ge * ge) >> 12)) >> 12);
        a = (int32_t)(((int64_t)a * (32767 - ((depth * (32767 - chop)) >> 15))) >> 15);
    }
    return a;
}

static void saurus_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t sh = saur_shape(t), fl = SAUR_CALL[sh].f, q = (uint32_t)v->s[7] & FMT_TALK_MASK;
    uint32_t rnd = (uint32_t)v->s[7] >> 25, i, j, ph = v->ph[0], gs = v->ph[1], inc, incj;
    int32_t wild = p[P_E7], growl = p[P_E4], pitch16, open, ratio, bws, f0, fq[3], bq[3];
    int32_t og, ogk, tilt, gv, gn, dr, sub, jit, jp;
    int32_t y1 = v->s[0], y2 = v->s[1], y3 = v->s[2], y4 = v->s[3], y5 = v->s[4], y6 = v->s[5], lp = v->s[6];
    int32_t nst = formant_nz;
    fres_t r1, r2, r3;

    if (q < (1u << 24)) {                               /* the call goes on over LEN (WILD: this note's own length) */
        q += ENV_LIN[clamp(p[P_E3] + ((((int32_t)rnd - 64) * wild) >> 8), 0, 127)];
        if (q > (1u << 24))
            q = 1u << 24;
        v->s[7] = (int32_t)(((uint32_t)v->s[7] & ~(uint32_t)FMT_TALK_MASK) | q);
    }
    /* pitch: the shape's points x SWOOP, and WILD's offset for this note (up to +-2 semitones) */
    pitch16 = ((saur_pt(SAUR_CALL[sh].p, 0, q) * p[P_E2] * 3) >> 12) + ((((int32_t)rnd - 64) * wild) >> 8);
    {
        uint64_t x = pitch16 ? ((uint64_t)m->inc * fmt_ratio(pitch16)) >> 16 : m->inc;
        inc = x > 0x7FFFFFFFu ? 0x7FFFFFFFu : (uint32_t)x;
    }
    /* the mouth: MOUTH x the shape's opening, ENV / LFO -> FLT, a harder note opens it a little */
    open = clamp(((p[P_E6] * saur_pt(0, SAUR_CALL[sh].m, q)) >> 11) + (m->cutoff >> 7) + ((v->vel - 64) >> 1), 0, 256);
    /* SIZE: +1 octave (0) .. -2 octaves (127), key tracking 1/4; HISS's noise a twelfth up (a fricative) */
    ratio = (int32_t)fmt_ratio(192 - ((p[P_E1] * 4645) >> 10) + ((v->pitch_cur - 60 * 16) >> 2) + (sh == SC_HISS ? 304 : 0));
    bws = (int32_t)fmt_ratio((growl - 32) * 2);         /* GROWL widens the bands */
    f0 = (int32_t)(((uint64_t)inc * 705600u) >> 32);    /* F0 in Hz * 16 */
    {
        int32_t a = open >= 128 ? 1 : 0, w = open - (a << 7);   /* oo -> oh (0..128), oh -> ah (128..256) */
        for (j = 0; j < 3u; j++) {
            int32_t fz = ((SAUR_F[a][j] << 7) + (SAUR_F[a + 1][j] - SAUR_F[a][j]) * w) >> 3;   /* Hz * 16 */
            int32_t bz = ((SAUR_B[a][j] << 7) + (SAUR_B[a + 1][j] - SAUR_B[a][j]) * w) >> 3;
            fz = (int32_t)(((int64_t)fz * ratio) >> 16);
            bz = (int32_t)(((((int64_t)bz * ratio) >> 16) * bws) >> 16);
            if (j == 0 && fz < f0 + (f0 >> 4) && fz > 0) {   /* a high call: F1 follows F0 (no boom on it) */
                bz = (int32_t)(((int64_t)bz * (f0 + (f0 >> 4))) / fz);
                fz = f0 + (f0 >> 4);
            }
            fq[j] = clamp(fz, 40 * 16, 7000 * 16);
            bq[j] = clamp(bz, 20 * 16, 1500 * 16);
        }
    }
    fres_coef(&r1, (uint32_t)fq[0], (uint32_t)bq[0]);
    fres_coef(&r2, (uint32_t)fq[1], (uint32_t)bq[1]);
    fres_coef(&r3, (uint32_t)fq[2], (uint32_t)bq[2]);

    tilt = clamp(2600 + growl * 70 + (v->vel - 64) * 24, 1500, 16000);   /* the source: dark .. open (Q14) */
    gv = 32767 - p[P_E5] * p[P_E5];                     /* voicing: a little less with more breath */
    gn = p[P_E5] * 200;                                 /* breath: Q15 */
    if (fl & SCF_UNVOICED) {
        gv = 0;
        gn = 22000 + p[P_E5] * 80;
    }
    sub = (growl * growl * 19) >> 4;                    /* the weak period's dip: up to 0.58 (Q15) */
    jit = (growl + wild) * 40;                          /* a period's random gain: up to 0.31 */
    jp = growl * 40 + wild * 120;                       /* a period's random length: up to +-3.8 % */
    og = (int32_t)(((int64_t)(750 * SAUR_CALL[sh].g) * fmt_ratio((60 * 16 - v->pitch_cur) / 4)) >> 16);   /* low notes: +1.5 dB
                                                                                         * an octave */
    ogk = (og >> 2) * (2 + (growl >> 5));               /* presence */
    dr = 64 + growl * 3;                                /* GROWL's drive: x1 .. x7 */
    incj = inc + (uint32_t)((int32_t)(inc >> 11) * (int32_t)(int8_t)(gs >> 16));

    for (i = 0; i < n; i++) {
        int32_t e, x, a, s;
        uint32_t nx;
        e = (osc_saw(ph, incj) * (int32_t)(gs & 0x7FFFu)) >> 15;
        lp += ((e - lp) * tilt) >> 14;
        x = (lp * gv) >> 15;
        if (gn) {
            int32_t nz = (((int32_t)(noise32(&nst) >> 16) - 32768) * gn) >> 15;
            x += (fl & SCF_UNVOICED) || ph < 0x80000000u ? nz : nz >> 1;   /* aspiration: with the period's opening */
        }
        a = (int32_t)(((int64_t)r1.a * (x << 6) + (int64_t)r1.b * y1 + (int64_t)r1.c * y2 + (1 << 29)) >> 30);
        y2 = y1;
        y1 = a;
        a = (int32_t)(((int64_t)r2.a * a + (int64_t)r2.b * y3 + (int64_t)r2.c * y4 + (1 << 29)) >> 30);
        y4 = y3;
        y3 = a;
        a = (int32_t)(((int64_t)r3.a * a + (int64_t)r3.b * y5 + (int64_t)r3.c * y6 + (1 << 29)) >> 30);
        y6 = y5;
        y5 = clamp(a, -(1 << 28), 1 << 28);
        s = clamp((int32_t)(((int64_t)y5 * og + (int64_t)(y5 - y6) * ogk) >> 21), -200000, 200000);
        if (growl)                                      /* the drive, blended in by GROWL */
            s += ((softclip((s * dr) >> 6) - s) * growl) >> 7;
        s = soft_knee(s, 24000);
        out[i] += voice_amp(s, m, i) << 1;
        nx = ph + incj;
        if (nx < ph) {                                  /* a new period: its gain and its length */
            uint32_t r = noise32(&nst), tog = ((gs >> 15) & 1u) ^ 1u;
            int32_t g = 32767 - (tog ? sub : 0) - (int32_t)(((r & 0x7FFFu) * (uint32_t)jit) >> 15);
            int32_t pj = (((int32_t)((r >> 16) & 255u) - 128) * jp) >> 15;
            gs = (uint32_t)clamp(g, 0, 32767) | tog << 15 | ((uint32_t)pj & 255u) << 16;
            incj = inc + (uint32_t)((int32_t)(inc >> 11) * pj);
        }
        ph = nx;
    }
    v->ph[0] = ph;
    v->ph[1] = gs;
    formant_nz = nst;
    v->s[0] = y1;
    v->s[1] = y2;
    v->s[2] = y3;
    v->s[3] = y4;
    v->s[4] = y5;
    v->s[5] = y6;
    v->s[6] = lp;
}

static const preset_t SAURUS_PRESETS[] = {
    /* name, {CALL, SIZE, SWOOP, LEN | GROWL, BRTH, MOUTH, WILD}, {A D S R}, fenv, mono (tools/ghoulbox_presets.py) */
    /* GHOULBOX presets (tools/ghoulbox_presets.py) */
    {"BRACHIO CALL", {1, 120, 42, 108, 20, 35, 90, 25}, {40, 90, 127, 90}, 0, 0, FX(0, 20, 15, 110), PAT(17)},
    {"TYRANT ROAR", {2, 105, 58, 102, 100, 45, 127, 30}, {4, 90, 127, 80}, 0, 1, FX(30, 0, 10, 100), PAT(17)},
    {"RAPTOR BARK", {3, 35, 55, 82, 80, 60, 120, 50}, {0, 70, 127, 40}, 0, 0, FX(10, 0, 25, 85), PAT(4)},
    {"RAPTOR CHAT", {5, 30, 20, 96, 50, 50, 90, 70}, {2, 80, 110, 40}, 0, 0, FX(5, 0, 20, 85), PAT(4)},
    {"CREST HOOT", {4, 85, 10, 104, 10, 25, 40, 15}, {50, 90, 127, 100}, 0, 0, FX(0, 30, 20, 115), PAT(17)},
    {"PTERO SHRIEK", {6, 15, 70, 92, 70, 70, 127, 60}, {0, 80, 110, 50}, 0, 0, FX(15, 0, 30, 95), PAT(4)},
    {"HERD MOAN", {7, 95, 45, 110, 35, 40, 80, 40}, {40, 90, 120, 100}, 0, 0, FX(0, 40, 10, 110), PAT(5)},
    {"FRILL HISS", {8, 40, 0, 96, 30, 90, 110, 30}, {6, 80, 110, 60}, 0, 0, FX(0, 0, 10, 80), PAT(17)},
    {"SAURO SNORT", {9, 120, 0, 88, 40, 110, 70, 30}, {0, 80, 127, 50}, 0, 0, FX(0, 0, 0, 90), PAT(4)},
    {"DEEP RUMBLE", {0, 127, 0, 0, 60, 30, 40, 20}, {80, 90, 127, 110}, 0, 0, FX(10, 30, 0, 110), PAT(17)},
    {"TITAN APE", {2, 70, 30, 98, 100, 60, 110, 30}, {6, 90, 127, 80}, 0, 1, FX(25, 0, 10, 95), PAT(17)},
    {"NEST CHIRPS", {6, 5, 50, 84, 20, 40, 110, 90}, {0, 70, 110, 40}, 0, 0, FX(0, 20, 30, 90), PAT(4)},
    {"SWAMP CROAK", {5, 75, 8, 100, 60, 30, 20, 40}, {10, 80, 120, 60}, 0, 0, FX(0, 0, 10, 90), PAT(8)},
    {"MONSTER ROAR", {2, 90, 66, 100, 127, 70, 127, 45}, {2, 90, 127, 70}, 0, 1, FX(60, 0, 20, 90), PAT(17)},
    {"DISTANT HERD", {1, 110, 36, 112, 15, 30, 70, 50}, {90, 90, 120, 120}, 0, 0, FX(0, 40, 25, 127), PAT(5)},
    {"LOW GROWL", {0, 95, 0, 0, 127, 80, 50, 40}, {30, 90, 127, 80}, 0, 1, FX(20, 0, 0, 90), PAT(17)},
    {"BABY REX", {2, 55, 60, 95, 70, 50, 110, 40}, {2, 90, 127, 60}, 0, 1, FX(10, 0, 20, 90), PAT(4)},
    {"SEA MONSTER", {7, 127, 30, 118, 70, 30, 60, 30}, {60, 90, 127, 120}, 0, 0, FX(10, 30, 20, 127), PAT(17)},
    {"HADRO HORN", {4, 100, 15, 110, 15, 20, 70, 20}, {40, 90, 127, 110}, 0, 0, FX(0, 20, 30, 120), PAT(17)},
    {"COMPY PACK", {6, 20, 60, 70, 30, 50, 100, 110}, {0, 64, 100, 30}, 0, 0, FX(0, 20, 30, 85), PAT(13)},
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
    .amp = saurus_amp,
    .knob = {P_E1, P_E4, P_E2, P_E3},
    .poly = 4,
};
/* its favourites are bytes 0..9 of favorites.factory[15], below the settings there (settings_persist.c): 80 sounds */
_Static_assert(NELEM(SAURUS_PRESETS) <= 80u, "SAURUS's favourites fit bytes 0..9 of favorites.factory[15]");
