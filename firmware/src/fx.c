/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Effects: per-track DIST insert, then sends into three shared buses (chorus, tempo delay, reverb).
 * Stereo dry mix; the chorus and the reverb come back in stereo, the delay in the middle. */
#define DLY_LEN 32768u           /* RAPTOR: 0.74 s (SLOOP's 1.49 s: the RAM went to tracks 5 and 6): 1/4 down to 81 BPM,
                                  * 1/8 dotted down to 61, 1/4 dotted down to 121 (clamped below) */
#define CHO_LEN 2048u
static int16_t dly_buf[DLY_LEN] __attribute__((section(".pool")));
static int16_t cho_buf[CHO_LEN] __attribute__((section(".pool")));
/* RAPTOR: Paleotone's reverb bus (its fx.c): ROOM, SPRING and HALL (G_RTYPE), mono, as Paleotone's chorus and delay */
static const uint16_t REV_COMB[4] = {1116, 1188, 1277, 1356};
static const uint16_t REV_AP[2] = {556, 441};
static int16_t rev_comb[1116 + 1188 + 1277 + 1356] __attribute__((section(".lowram")));   /* (the pool is full: .lowram,
                                                                      * the SRAM below the RAM region, app.ld) */
static union {                          /* ROOM's allpasses; SPRING's allpass chain (int32: no clamps) */
    int16_t ap[556 + 441];
    int32_t sp[(556 + 441) / 2];
} rev_u __attribute__((section(".pool")));
#define rev_ap (rev_u.ap)
static struct {
    uint32_t dly_w, cho_w, cho_ph;
    int32_t dly_lp;
    uint16_t comb_i[4], ap_i[2];
    int32_t comb_lp[4];
    uint8_t rtype;                       /* the reverb model running (G_RTYPE: 0 ROOM, 1 SPRING, 2 HALL) */
    uint16_t sp_w;                       /* SPRING: the loop's write index (SP_MASK) */
    int32_t sp_lp, sp_hp, sp_he, sp_size;   /* .. its loop low-pass, low cut (and its remainder), the loop
                                             * length (Q8, glides) */
    uint32_t sp_ph;                      /* .. the output tap's wobble */
    uint16_t hl_p;                       /* HALL: the ring's pointer (counts down: hl_at) */
    int32_t hl_lpl, hl_lpr;              /* .. the two halves' damping low-passes */
    uint32_t hl_ph;                      /* .. the tank allpasses' slow modulation */
    int32_t hl_dc[4];                    /* .. FREEZE (RAPTOR): a DC blocker in each half instead of the losses (a loop
                                          * gain of 1 adds up the roundings that lean one way) */
} fx = {.rtype = 2};

/* HALL (G_RTYPE 2): Paleotone's (GHOULBOX) J. Dattorro figure-eight tank ("Effect Design, Part 1", JAES 1997): a
 * pre-delay (longer with SIZE), four input allpasses (diffusion), then two halves that feed each other: a modulated
 * allpass, a delay, the damping low-pass (DAMP), the decay gain (SIZE), a second allpass and delay. Seven taps, his
 * left output: the bus is mono. Paleotone's ring is 32768 samples, the back half of its 1.49 s delay line; RAPTOR's
 * delay line is half that (the RAM went to tracks 5 and 6), so HALL here is a smaller room: the pre-delay and the
 * input allpasses as Paleotone's, the tank's lines and taps x 0.428, in the back half of dly_buf (16384; the delay
 * keeps the front half while HALL runs, 0.37 s). Its decay gain is Paleotone's to the power 0.428 (the same RT60
 * a second: the loop is shorter) and its damping pole's time constant x sqrt(0.428) (the same high loss a second:
 * HL_KD). tools: the numbers come from Paleotone's by those two rules.
 * All thirteen lines share one ring and one pointer that counts down a sample at a time: line k writes at
 * hl_p + HL_B[k] and reads d samples back at hl_p + HL_B[k] + d, both masked. A line's region is its longest delay + 1 */
enum { H_PRE, H_I1, H_I2, H_I3, H_I4, H_AL, H_D1L, H_APL, H_D2L, H_AR, H_D1R, H_APR, H_D2R, H_N };
static const uint16_t HL_D[H_N] = {2792, 189, 142, 504, 368, 406, 2535, 1025, 2118, 540, 2401, 1512, 1801};
#define HL_B_PRE 0u
#define HL_B_I1 2793u
#define HL_B_I2 2983u
#define HL_B_I3 3126u
#define HL_B_I4 3631u
#define HL_B_AL 4000u
#define HL_B_D1L 4407u
#define HL_B_APL 6943u
#define HL_B_D2L 7969u
#define HL_B_AR 10088u
#define HL_B_D1R 10629u
#define HL_B_APR 13031u
#define HL_B_D2R 14544u
#define HL_MASK (DLY_LEN / 2u - 1u)
#define HL_AL 383u                       /* the modulated allpasses' centre delays, +-HL_EXC (their lines: +23) */
#define HL_AR 517u
#define HL_EXC 21
#define hl_buf (dly_buf + DLY_LEN / 2u)
_Static_assert(HL_B_D2R + 1801u + 1u <= DLY_LEN / 2u, "HALL's lines fit the back half of dly_buf");

/* SPRING (G_RTYPE 1), Paleotone's: one spring of a spring tank, in the ROOM's own buffers: the input and the loop's
 * return -> a low cut (~110 Hz) -> SP_N stretched first-order allpasses (the chirp) -> the loop's delay line
 * (rev_comb, SP_LEN) -> back through a one-pole low-pass (DAMP) and the decay gain (SIZE). The output: the spring's
 * far end, half way along the loop, plus a second pickup at three quarters */
#define SP_LEN 4096u
#define SP_MASK (SP_LEN - 1u)
#define SP_N 10u
#define SP_A 2867
_Static_assert(sizeof rev_comb / 2u >= SP_LEN && sizeof rev_u.sp / 4u >= 4u * (SP_N + 1u), "SPRING in ROOM's buffers");
static volatile uint8_t rev_frz;         /* punch-in FREEZE (punch.c): the reverb holds, no loss, no damping */
static int32_t rev_l[CTL], rev_r[CTL];   /* the reverb's own return of this block (FREEZE plays it) */

/* DIST: low cut -> drive (1x..8x, exponential) -> asymmetric soft clip
 * (a little bias = even harmonics) -> tone low-pass that closes with drive ->
 * make-up gain. State per part (track_t dist_*). */
static void track_dist(track_t *t, int32_t *b, uint32_t n)
{
    int32_t d = t->p[P_DIST], i, g, k, mk, bias = 2400, b0;
    if (!d) {
        t->dist_on = 0;
        return;
    }
    if (!t->dist_on) {                                  /* coming on: no stale high-pass state (a thump) */
        t->dist_on = 1;
        t->dist_hp = b[0];
        t->dist_lp1 = t->dist_lp2 = 0;
    }
    g = 4096 + d * d * 2;                                /* Q12: 1x .. ~9x, gentle at first */
    k = 32000 - d * 95;                                  /* tone: transparent at low drive .. ~3 kHz, Q15 */
    mk = 30000 - d * 120;                                /* make-up */
    b0 = softclip(bias);
    for (i = 0; i < (int32_t)n; i++) {
        int32_t x = b[i], y;
        t->dist_hp += (x - t->dist_hp + 64) >> 7;           /* ~55 Hz low cut: keep the bass out of the clipper */
        x = clamp(x - t->dist_hp, -230000, 230000);         /* (x >> 2) * g fits 32 bits; the clip is flat out there */
        y = softclip(MULQ12R(x, g, 2) + bias) - b0;         /* (as (x >> 2) * g >> 10 in 32 bits: no overflow for loud
                                                             * poly; #4: rounded, no step of 4 x the drive) */
        t->dist_lp1 += mulq15(y - t->dist_lp1, k);         /* two poles: tames the fizz */
        t->dist_lp2 += mulq15(t->dist_lp1 - t->dist_lp2, k);
        b[i] = mulq15(t->dist_lp2, mk);
    }
}

/* master: peak limiter in front of the soft clipper, a 35 ms hold then ~46 ms release (2.6), threshold where tanh is still nearly linear,
 * so chords get quieter instead of crushed. SLOOP 2.6: it looks one block ahead (lim_block): the output is the mix
 * CTL samples late (0.73 ms) and the gain glides over each block to what the loudest sample of it and of the next one
 * needs, so a hot kick or bass is already turned down when it comes. Before, the gain caught up in ~4 samples and the
 * first ones went through the tanh knee at up to 4x: a crackle on the beats with the MASTER up (in headphones most) */
#define LIM_T 18000
#ifndef SMK_MIX
#define SMK_MIX BOARD_SMK37                     /* (tests/smk_mix_test.c: on the host) */
#endif
#if SMK_MIX
/* SMK-37: HOME menu > AUDIO > LIMITER. SOFT (the default): the threshold at full scale (LIM_T_SOFT, 6 dB above LIM_T;
 * the knee rounds what passes KNEE and keeps it under full scale) and the MASTER knob gives the mix less level below
 * its top (smk_master_q12: 9 dB less at its power-on 724), so a held chord fits under it instead of turning the whole
 * mix down at every attack ("one note loud, a chord quieter"); the knob's top is as before, and louder peaks pass.
 * CLASSIC: SLOOP's, LIM_T and the knob as the FM-1's pot. tests/smk_mix_test.c */
#define LIM_T_SOFT 36000
static uint8_t lim_classic;                     /* 0 SOFT, 1 CLASSIC (settings: project.c midi_part_fill) */
#define LIM_TH (lim_classic ? LIM_T : LIM_T_SOFT)
static uint32_t smk_master_q12(uint32_t k10, uint32_t classic)   /* the MASTER knob 0..1023 -> song.master_q12 */
{
    uint32_t q = (k10 * k10) >> 8;              /* (the FM-1's law: 0 .. ~4096) */
    return classic ? q : (((q * q) >> 12) * k10) >> 10;   /* SOFT: (k / 1023)^5, the same at the top, -9 dB at 724 */
}
#else
#define LIM_TH LIM_T
#endif
typedef struct {
    int32_t dl[CTL], dr[CTL];           /* the mix of the last block (the delay) */
    uint32_t w;                         /* its oldest sample (the next out) */
    int32_t env;                        /* the peak the gain fits under LIM_T (LIM_T: none) */
    int32_t g;                          /* the gain at the end of the last block, Q15 (32768: 1) */
    uint32_t hold;                      /* blocks before the release starts (LIM_HOLD after a peak near env) */
} lim_t;
/* (2.6, a whistle on loud basses) the release waits LIM_HOLD blocks after the last peak near the envelope: a held bass
 * (down to ~29 Hz, a period under 35 ms) keeps one gain, instead of the gain dipping and creeping back at every cycle
 * (a sawtooth at the bass's pitch with a corner at every block: tones around 1.4 kHz and up) */
#define LIM_HOLD 48u
#define LIM_REL 11                      /* the release after the hold: ~46 ms (12 before the hold: ~93 ms; the same loudness) */
static lim_t lim_dac = {.env = LIM_T, .g = 32768}, lim_usb = {.env = LIM_T, .g = 32768};
/* l, r: n (<= CTL) samples in; out: the n samples of CTL before, limited (before the knee) */
static void lim_block(lim_t *s, int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i, w = s->w;
    int32_t pk = 0, a, g0 = s->g, g1, dg, e = s->env;
    for (i = 0; i < CTL; i++) {                         /* the samples going out now and the ones after them (the
                                                         * rest of the delay, then this block): CTL ahead */
        a = s->dl[i] < 0 ? -s->dl[i] : s->dl[i], pk = a > pk ? a : pk;
        a = s->dr[i] < 0 ? -s->dr[i] : s->dr[i], pk = a > pk ? a : pk;
    }
    for (i = 0; i < n; i++) {
        a = l[i] < 0 ? -l[i] : l[i], pk = a > pk ? a : pk;
        a = r[i] < 0 ? -r[i] : r[i], pk = a > pk ? a : pk;
    }
    if ((int64_t)pk * 17 >= (int64_t)e * 16)           /* a peak within ~0.5 dB of the envelope: the hold again */
        s->hold = LIM_HOLD;
    if (s->hold)
        s->hold--;
    else if (e > LIM_TH)                                /* the release (after the hold), a block at a time */
        e -= (int32_t)((((int64_t)(e - LIM_TH) * (int32_t)n) >> LIM_REL) + 1);
    if (e < LIM_TH)
        e = LIM_TH;
    if (pk > e)
        e = pk, s->hold = LIM_HOLD;
    s->env = e;
    g1 = e > LIM_TH ? (int32_t)(((uint32_t)LIM_TH << 15) / (uint32_t)e) : 32768;
    s->g = g1;
    dg = (g1 - g0) / (int32_t)(n ? n : 1u);             /* (towards 0: the gain never above both ends' bound) */
    for (i = 0; i < n; i++) {
        int32_t xl = s->dl[w], xr = s->dr[w], g = g0 + dg * (int32_t)(i + 1u);
        s->dl[w] = l[i], s->dr[w] = r[i];
        w = (w + 1u) & (CTL - 1u);
        if (i + 1u == n)
            g = g1;
        if (g < 32768) {                                /* (64-bit: |x| may be far above Q15. 2.6 fix: x >> 4 first cut
                                                         * the limited signal to 12 bits, a whistle on loud basses) */
            xl = (int32_t)(((int64_t)xl * g) >> 15);
            xr = (int32_t)(((int64_t)xr * g) >> 15);
        }
        l[i] = xl, r[i] = xr;
    }
    s->w = w;
}
static volatile uint8_t fx_lowcut;     /* settings: 12 dB/oct ~110 Hz for the small speaker */
static uint8_t flt_reso;               /* (2.7.2) menu AUDIO > FILTER RESO: 0..10, the track filter's resonance (fx.c
                                        * tflt_reso); 0 = the clean filter as before. One setting for every track's filter */
static int32_t lc_l1, lc_l2, lc_r1, lc_r2, dc_l, dc_r, dce_l, dce_r;

/* DC blocker (~2 Hz), always on: a leaky integrator of the input (Q6 state) subtracted from it.
 * The >> 12 step keeps its remainder (error feedback, 0..4095) and adds it to the next one, so no
 * part of the step is lost: the state follows the input exactly, down to 0 after the sound stops.
 * (It was rounded, and a rounded step of (x - dc) / 4096 stops moving at |x - dc| < 2048: a
 * constant offset of up to +-31 stayed at the output after silence.) */
static inline int32_t dc_block(int32_t x, int32_t *dc, int32_t *err)
{
    int32_t e = (x << 6) - *dc + *err, d = e >> 12;
    *err = e - (d << 12);
    *dc += d;
    return x - ((*dc + 32) >> 6);
}

static int32_t lce[4];
static inline int32_t lowcut1(int32_t x, int32_t *lc, int32_t *err)   /* x minus its one-pole low-pass */
{
    int32_t e = x - *lc + *err, d = e >> 6;
    *err = e - (d << 6);
    *lc += d;
    return x - *lc;
}

/* the output stage: linear up to KNEE (a clean low end: no tanh harmonics on a loud sine), above it
 * a tanh knee with the same slope at the joint, to full scale */
#define KNEE 16384
static inline int32_t knee(int32_t x)
{
    int32_t a = x < 0 ? -x : x;
    if (a <= KNEE)
        return x;
    a = KNEE + (softclip((a - KNEE) * 2) >> 1);
    return x < 0 ? -a : a;
}

/* the USB audio input at full level (menu USB AUDIO = FULL): the same output stage with its own state,
 * fed as if MASTER were all the way up, so the recording level does not follow the knob. The DAC path
 * (master_pre, lim_dac) is untouched. audio.c sets usb_full_now for a render while the computer records. */
static uint8_t usb_full;                 /* menu USB AUDIO: 0 MASTER (follows the knob), 1 FULL (panel.c lights_word) */
static uint8_t usb_full_now;             /* this render fills usb_out (audio.c) */
static int32_t usb_out[2u * CTL];
/* the visualiser's input (audio.c fills the scope with it): the mix as if MASTER were all the way up, so the
 * picture does not follow the volume knob, even at 0 (2.4). The same soft knee as the output, no limiter state */
static int16_t vis_tap[2u * CTL];
static struct { int32_t dc[2], dce[2], lc[4], lce[4]; } uo;
static void master_pre_usb(int32_t *l, int32_t *r)      /* (USB AUDIO = FULL: its own state) */
{
    *l = dc_block(*l, &uo.dc[0], &uo.dce[0]);
    *r = dc_block(*r, &uo.dc[1], &uo.dce[1]);
    if (fx_lowcut) {
        *l = lowcut1(*l, &uo.lc[0], &uo.lce[0]);
        *l = lowcut1(*l, &uo.lc[1], &uo.lce[1]);
        *r = lowcut1(*r, &uo.lc[2], &uo.lce[2]);
        *r = lowcut1(*r, &uo.lc[3], &uo.lce[3]);
    }
}

/* the output stage before the limiter: the DC blocker, the low cut */
static inline void master_pre(int32_t *l, int32_t *r)
{
    *l = dc_block(*l, &dc_l, &dce_l);
    *r = dc_block(*r, &dc_r, &dce_r);
    if (fx_lowcut) {                  /* two one-pole high-passes, error feedback as dc_block (the */
        *l = lowcut1(*l, &lc_l1, &lce[0]);          /* rounded step stopped at |x - lc| < 32: an offset) */
        *l = lowcut1(*l, &lc_l2, &lce[1]);
        *r = lowcut1(*r, &lc_r1, &lce[2]);
        *r = lowcut1(*r, &lc_r2, &lce[3]);
    }
}



static uint32_t delay_samples(void)
{
    uint32_t s = dly_samples((uint32_t)song.g[G_DTIME]), len = fx.rtype == 2u ? DLY_LEN / 2u : DLY_LEN;   /* HALL: the front half */
    return s < 16u ? 16u : s >= len ? len - 1u : s;
}

#define HALF0(v) ((v) >= 0 ? (v) >> 1 : -(-(v) >> 1))   /* v / 2 towards zero */
/* ROOM (G_RTYPE 0): 4 damped combs + 2 allpasses (Freeverb-like, mono), added to out */
static __attribute__((noinline)) void rev_room(const int32_t *rev_in, int32_t *out, uint32_t n)
{
    uint32_t i, k;
    int32_t size = 25000 + song.g[G_RSIZE] * 50, damp = 32767 - song.g[G_RDAMP] * 200;
    if (rev_frz)                                        /* FREEZE (punch.c): no damping, a loop gain of 1 */
        damp = 32767;
    for (i = 0; i < n; i++) {
        int32_t a = 0;
        int16_t *c = rev_comb;
        int32_t in = mulq15(rev_in[i], 2580);           /* 1/8 at -4 dB: level as before the allpass fix */
        for (k = 0; k < 4u; k++) {
            int32_t o = c[fx.comb_i[k]];
            fx.comb_lp[k] = o + mulq15(fx.comb_lp[k] - o, 32767 - damp);
            c[fx.comb_i[k]] = (int16_t)clamp(in + (rev_frz ? fx.comb_lp[k] : mulq15(fx.comb_lp[k], size)), -32768, 32767);
            if (++fx.comb_i[k] >= REV_COMB[k])
                fx.comb_i[k] = 0;
            a += o;
            c += REV_COMB[k];
        }
        c = rev_ap;
        for (k = 0; k < 2u; k++) {
            int32_t o = c[fx.ap_i[k]];
            int32_t v = a + (o >> 1);
            c[fx.ap_i[k]] = (int16_t)clamp(v, -32768, 32767);
            a = o - a;                                  /* Freeverb: out = buf - in (o - v is a notch comb) */
            if (++fx.ap_i[k] >= REV_AP[k])
                fx.ap_i[k] = 0;
            c += REV_AP[k];
        }
        out[i] += a;
    }
}

/* SPRING (see the top), added to out */
static __attribute__((noinline)) void rev_spring(const int32_t *rev_in, int32_t *out, uint32_t n)
{
    uint32_t i, k, s = (uint32_t)song.g[G_RSIZE];
    int32_t g = rev_frz ? 32767 : 19661 + (int32_t)s * 85;   /* the loop's gain: 0.6 .. 0.93 (FREEZE: 1) */
    int32_t kl = rev_frz ? 32767 : 26000 - song.g[G_RDAMP] * 160;   /* its low-pass: ~9 kHz .. ~1.3 kHz */
    int32_t len = (int32_t)(1323u + ((s * 1323u) >> 7)) << 8, L, L2, L3, f, w;
    int16_t *ln = rev_comb;
    int32_t *ap = rev_u.sp;
    if (!fx.sp_size)
        fx.sp_size = len;
    fx.sp_size += clamp(len - fx.sp_size, -256, 256);   /* SIZE glides (a sample a block at most) */
    L = fx.sp_size >> 8;
    fx.sp_ph += 2u * LFO_INC[24];                       /* the wobble: a slow sine, 1.5 samples deep */
    w = (fx.sp_size >> 1) + ((osc_sine(fx.sp_ph) * 3) >> 8);   /* the far end, Q8 */
    L2 = w >> 8;
    f = w & 255;
    L3 = (L * 3) >> 2;
    for (i = 0; i < n; i++) {
        uint32_t wp = fx.sp_w, j = (wp & 3u) * (SP_N + 1u);
        int32_t x = mulq15(rev_in[i], 2580), r = ln[(wp - (uint32_t)L) & SP_MASK], p, o;
        int32_t t0 = ln[(wp - (uint32_t)L2) & SP_MASK], t1 = ln[(wp - (uint32_t)L2 - 1u) & SP_MASK];
        fx.sp_lp += mulq15(r - fx.sp_lp, kl);
        o = fx.sp_lp * g;
        x += (o + ((o >> 31) & 32767)) >> 15;           /* towards 0: a loop of floors would hold an offset */
        o = x - fx.sp_hp + fx.sp_he;                    /* the low cut, its step's remainder kept (as */
        fx.sp_he = o & 63;                              /* dc_block): no dead band to hold an offset in the loop */
        fx.sp_hp += o >> 6;
        x -= fx.sp_hp;
        p = ap[j];                                      /* the chain: ap[j + k], stage k's output 4 samples ago */
        ap[j] = x;
        for (k = 1; k <= SP_N; k++) {                   /* (lossless: bounded by the loop's input, no clamp) */
            int32_t v = (x - ap[j + k]) * SP_A;         /* towards 0, as the loop's gain: floors would feed */
            o = ap[j + k];                              /* the loop a little offset and noise for ever */
            x = ((v + ((v >> 31) & 4095)) >> 12) + p;
            p = o;
            ap[j + k] = x;
        }
        ln[wp & SP_MASK] = (int16_t)clamp(x, -32768, 32767);
        fx.sp_w = (uint16_t)(wp + 1u);
        out[i] += (t0 + (((t1 - t0) * f) >> 8)) * 4 + ln[(wp - (uint32_t)L3) & SP_MASK] * 2;
    }
}

/* HALL (see the top) */
static inline int32_t hl_mul(int32_t a, int32_t g)      /* a * g / 32768 towards 0 (|a| <= 32768) */
{
    int32_t v = a * g;
    return (v + ((v >> 31) & 32767)) >> 15;
}
#define hl_at(base, d) hl_buf[(p + (base) + (d)) & HL_MASK]        /* d samples back (1 .. HL_D) */
#define hl_put(base, v) (hl_buf[(p + (base)) & HL_MASK] = (int16_t)clamp((v), -32768, 32767))
static inline int32_t hl_ap(uint32_t p, uint32_t base, uint32_t len, int32_t x, int32_t g)   /* allpass, g Q15 */
{
    int32_t o = hl_at(base, len), w = clamp(x - hl_mul(o, g), -32768, 32767);
    hl_buf[(p + base) & HL_MASK] = (int16_t)w;
    return o + hl_mul(w, g);
}
static inline int32_t hl_apmod(uint32_t p, uint32_t base, int32_t x, int32_t g, int32_t dq8)   /* .. at a moving delay (Q8) */
{
    uint32_t d = (uint32_t)dq8 >> 8;
    int32_t o0 = hl_at(base, d), o1 = hl_at(base, d + 1u), o = o0 + (((o1 - o0) * (dq8 & 255)) >> 8);
    int32_t w = clamp(x - hl_mul(o, g), -32768, 32767);
    hl_buf[(p + base) & HL_MASK] = (int16_t)w;
    return o + hl_mul(w, g);
}

/* HALL's decay gain (Q15) for SIZE 0..127, applied twice in each half of the tank (a half's loop: ~0.32 s), so
 * the low notes' RT60 (where the damping does not reach) = 0.48 s / -log10(g). SIZE sets that RT60 on an
 * exponential curve, 1.5 s at 0 to 12 s at 127 (x 2 every 42 steps: even to the ear); DAMP shortens the highs.
 * HL_G: g at SIZE 0, 8 .. 120, 127 (g = 10^(-0.4815 / RT60)), linear between. */
static const uint16_t HL_G[17] = {23882, 24827, 25688, 26467, 27170, 27802, 28369, 28875, 29327, 29730, 30087,
                                  30404, 30685, 30933, 31153, 31347, 31497};   /* (Paleotone's ^ 0.428) */
/* DAMP -> the damping low-pass's coefficient (Paleotone's 32767 - DAMP * 220, its pole's time constant x sqrt(0.428)) */
static const uint16_t HL_KD[128] = {
    32767, 32623, 32478, 32333, 32186, 32039, 31892, 31743, 31594, 31444, 31294, 31143, 30991, 30838, 30685, 30530,
    30376, 30220, 30064, 29906, 29749, 29590, 29430, 29270, 29109, 28947, 28785, 28621, 28457, 28292, 28126, 27959,
    27792, 27624, 27454, 27284, 27113, 26941, 26769, 26595, 26421, 26245, 26069, 25892, 25714, 25535, 25355, 25174,
    24992, 24809, 24626, 24441, 24255, 24069, 23881, 23692, 23503, 23312, 23120, 22928, 22734, 22539, 22343, 22146,
    21948, 21749, 21549, 21348, 21145, 20942, 20737, 20531, 20324, 20116, 19907, 19697, 19485, 19272, 19058, 18843,
    18627, 18409, 18190, 17970, 17748, 17526, 17301, 17076, 16849, 16621, 16392, 16161, 15929, 15696, 15461, 15225,
    14987, 14748, 14508, 14266, 14023, 13778, 13531, 13283, 13034, 12783, 12531, 12277, 12021, 11764, 11505, 11245,
    10983, 10719, 10454, 10187, 9918, 9648, 9375, 9102, 8826, 8548, 8269, 7988, 7705, 7420, 7134, 6845};
static int32_t hall_decay(int32_t size)
{
    uint32_t k = (uint32_t)size >> 3, f = (uint32_t)size & 7u;
    return HL_G[k] + (((int32_t)HL_G[k + 1u] - (int32_t)HL_G[k]) * (int32_t)f >> 3);
}

static __attribute__((noinline)) void rev_hall(const int32_t *rev_in, int32_t *out, uint32_t n)
{
    uint32_t i, s = (uint32_t)song.g[G_RSIZE], p = fx.hl_p;
    uint32_t pd = 1u + ((s * (HL_D[H_PRE] - 1u)) >> 7);   /* pre-delay: 0 .. 63 ms */
    int32_t g = hall_decay((int32_t)s), kd = HL_KD[song.g[G_RDAMP] & 127];   /* damping: open .. ~1 kHz (FREEZE,
                                                                                * punch.c: neither, a loop gain of 1) */
    int32_t m = (osc_sine(fx.hl_ph) * HL_EXC) >> 7, m2 = (osc_sine(fx.hl_ph + 0x40000000u) * HL_EXC) >> 7;
    int32_t dl = (int32_t)(HL_AL << 8) + m, dr = (int32_t)(HL_AR << 8) + m2, lpl = fx.hl_lpl, lpr = fx.hl_lpr;
    fx.hl_ph += LFO_INC[20];                            /* ~0.5 Hz, stepped per block */
    for (i = 0; i < n; i++, p--) {
        int32_t x = clamp(mulq15(rev_in[i], 2580), -32768, 32767), el, er, a, o;
        a = hl_at(HL_B_PRE, pd);
        hl_put(HL_B_PRE, x);
        a = hl_ap(p, HL_B_I1, 189u, a, 24576);          /* input diffusion 0.75, 0.75, 0.625, 0.625 */
        a = hl_ap(p, HL_B_I2, 142u, a, 24576);
        a = hl_ap(p, HL_B_I3, 504u, a, 20480);
        a = hl_ap(p, HL_B_I4, 368u, a, 20480);
        el = hl_at(HL_B_D2L, 2118u);                    /* the halves' ends, before either writes */
        er = hl_at(HL_B_D2R, 1801u);
        /* left half, fed by the right: decay diffusion 1 (-0.7, moving), delay, damping, decay, diffusion 2 (0.5) */
        o = hl_at(HL_B_D1L, 2535u);
        hl_put(HL_B_D1L, hl_apmod(p, HL_B_AL, clamp(a + (rev_frz ? er : hl_mul(er, g)), -32768, 32767), -22938, dl));
        lpl = rev_frz ? dc_block(o, &fx.hl_dc[0], &fx.hl_dc[1]) : hl_mul(o, kd) + hl_mul(lpl, 32768 - kd);   /* (each term towards 0: no dead band to stick in) */
        hl_put(HL_B_D2L, hl_ap(p, HL_B_APL, 1025u, rev_frz ? lpl : hl_mul(lpl, g), 16384));
        /* right half, fed by the left */
        o = hl_at(HL_B_D1R, 2401u);
        hl_put(HL_B_D1R, hl_apmod(p, HL_B_AR, clamp(a + (rev_frz ? el : hl_mul(el, g)), -32768, 32767), -22938, dr));
        lpr = rev_frz ? dc_block(o, &fx.hl_dc[2], &fx.hl_dc[3]) : hl_mul(o, kd) + hl_mul(lpr, 32768 - kd);   /* (each term towards 0: no dead band to stick in) */
        hl_put(HL_B_D2R, hl_ap(p, HL_B_APR, 1512u, rev_frz ? lpr : hl_mul(lpr, g), 16384));
        /* Dattorro's left output taps (x 1.33 x 0.428): the bus is mono; x 4: ROOM's level */
        o = hl_at(HL_B_D1R, 152u) + hl_at(HL_B_D1R, 1693u) - hl_at(HL_B_APR, 1089u) + hl_at(HL_B_D2R, 1136u)
          - hl_at(HL_B_D1L, 1133u) - hl_at(HL_B_APL, 107u) - hl_at(HL_B_D2L, 607u);
        out[i] += o << 2;
    }
    fx.hl_p = (uint16_t)p;
    fx.hl_lpl = lpl;
    fx.hl_lpr = lpr;
}

/* HALL's half of the delay line and its states to silence */
static void hall_clear(void)
{
    uint32_t i, *p = (uint32_t *)(void *)hl_buf;
    for (i = 0; i < DLY_LEN / 4u; i++)
        p[i] = 0;
    fx.hl_lpl = fx.hl_lpr = 0;
}

/* the reverb's buffers and states to silence (the model changed) */
static void rev_clear(void)
{
    uint32_t i;
    for (i = 0; i < sizeof rev_comb / 2u; i++)
        rev_comb[i] = 0;
    for (i = 0; i < sizeof rev_u.ap / 2u; i++)          /* (int16: 997 of them, an odd count the int32 view misses one of) */
        rev_u.ap[i] = 0;
    for (i = 0; i < 4u; i++)
        fx.comb_lp[i] = 0;
    fx.sp_lp = fx.sp_hp = fx.sp_he = 0;
}

static void rev_run(uint32_t model, const int32_t *rev_in, int32_t *out, uint32_t n)   /* G_RTYPE's model */
{
    if (model == 2u)
        rev_hall(rev_in, out, n);
    else if (model)
        rev_spring(rev_in, out, n);
    else
        rev_room(rev_in, out, n);
}

#include "tape.c"                                    /* RAPTOR: Paleotone's TAPE / CRSH on the mix, before MASTER */
#include "ambience.c"                                /* RAPTOR: Paleotone's ambience (AMB, CRKL) into the mix */

/* the reverb's buffers and states halved, a FREEZE's start (punch.c: its gain doubled, no step; room for the held
 * loop's peaks on the int16 lines) */
static void rev_halve(void)
{
    uint32_t i;
    for (i = 0; i < sizeof rev_comb / 2u; i++)
        rev_comb[i] = (int16_t)HALF0(rev_comb[i]);
    if (fx.rtype == 1u)
        for (i = 0; i < sizeof rev_u.sp / 4u; i++)
            rev_u.sp[i] = HALF0(rev_u.sp[i]);
    else
        for (i = 0; i < sizeof rev_u.ap / 2u; i++)
            rev_u.ap[i] = (int16_t)HALF0(rev_u.ap[i]);
    if (fx.rtype == 2u)
        for (i = 0; i < DLY_LEN / 2u; i++)
            hl_buf[i] = (int16_t)HALF0(hl_buf[i]);
    for (i = 0; i < 4u; i++)
        fx.comb_lp[i] = HALF0(fx.comb_lp[i]);
    fx.sp_lp = HALF0(fx.sp_lp), fx.hl_lpl = HALF0(fx.hl_lpl), fx.hl_lpr = HALF0(fx.hl_lpr);
}

/* process the three buses for one block (Paleotone's: mono, the chorus and the delay, then the reverb model); sends
 * in, wet out (the same on both sides). rev_l / rev_r: the reverb's own return (FREEZE plays it) */
static void fx_buses(const int32_t *cho_in, const int32_t *dly_in, const int32_t *rev_in, int32_t *wet_l,
                     int32_t *wet_r, uint32_t n)
{
    uint32_t i, dl = delay_samples(), dm = fx.rtype == 2u ? DLY_LEN / 2u - 1u : DLY_LEN - 1u;   /* HALL: front half */
    int32_t fb = song.g[G_DFDBK] * 230, col = 2000 + song.g[G_DCOLOR] * 240;
    if (thr_dly && fb < 89 * 230)                       /* ECHO THROW: FDBK at least 70 %, gliding with it (SLOOP 2.6) */
        fb += ((89 * 230 - fb) * thr_dly) >> 15;
    int32_t dmix = song.g[G_DMIX] * 258;
    int32_t cdepth = song.g[G_CDEPTH] * 6, rt;
    uint32_t cinc = LFO_INC[song.g[G_CRATE] & 127] / CTL;
    for (i = 0; i < n; i++) {
        int32_t y = 0, x, r;
        /* chorus: modulated short delay, 5..15 ms */
        cho_buf[fx.cho_w & (CHO_LEN - 1u)] = (int16_t)clamp(cho_in[i] >> 1, -32768, 32767);
        fx.cho_ph += cinc;
        r = (400 << 8) + ((osc_sine(fx.cho_ph) + 32768) * cdepth >> 8);   /* Q8 delay: read between samples */
        {
            uint32_t ri = (uint32_t)r >> 8;
            int32_t f = r & 255, c0 = cho_buf[(fx.cho_w - ri) & (CHO_LEN - 1u)];
            int32_t c1 = cho_buf[(fx.cho_w - ri - 1u) & (CHO_LEN - 1u)];
            y += (c0 + (((c1 - c0) * f) >> 8)) << 1;
        }
        fx.cho_w++;
        /* delay with a low-passed feedback */
        x = dly_buf[(fx.dly_w - dl) & dm];
        fx.dly_lp += mulq15(x - fx.dly_lp, col);
        dly_buf[fx.dly_w & dm] =
            (int16_t)clamp((dly_in[i] >> 1) + mulq15(fx.dly_lp, fb), -32768, 32767);
        fx.dly_w++;
        y += mulq15(x << 1, dmix);
        wet_l[i] = y;
        rev_l[i] = 0;
    }
    rt = song.g[G_RTYPE] >= 0 && song.g[G_RTYPE] <= 2 ? song.g[G_RTYPE] : 0;
    if (rt != fx.rtype) {                               /* the model changed: the old one's block fades out, */
        int32_t g = 65536, d = 65536 / (int32_t)n;      /* its buffers are cleared, the new one starts from silence */
        rev_run(fx.rtype, rev_in, rev_l, n);
        for (i = 0; i < n; i++, g -= d)
            rev_l[i] = mulq16(rev_l[i], (uint32_t)g);
        rev_clear();
        if (fx.rtype == 2u || rt == 2)                  /* HALL's half: the delay's own again, or HALL's from */
            hall_clear();                               /* silence (no old tail or old echoes replayed) */
        fx.rtype = (uint8_t)rt;
    } else
        rev_run((uint32_t)rt, rev_in, rev_l, n);
    for (i = 0; i < n; i++) {
        wet_l[i] += rev_l[i];
        wet_r[i] = wet_l[i];
        rev_r[i] = rev_l[i];
    }
}
/* one block of the whole mix (shared with tests/hostsim.c): events -> each part
 * -> dist -> SLICER -> level / pan / sends -> drums (-> SLICER) -> buses -> master; out: stereo Q15.
 * The parts and the drums are render units (mix_units below), each adding into an accumulator set: the dry
 * mix and the three sends. Core 0 renders its units into acc[0] and core 1 (cpu1.h) its share into acc[1],
 * added before the buses: integer sums commute, so the output is the same, bit for bit, whatever the split
 * (tests/regress.c gives the golden hashes under every mode of the host double: tests/cpu1_modes.sh). */
static void events_block(uint32_t n);                    /* seq.c */
typedef struct { int32_t l[CTL], r[CTL], sc[CTL], sd[CTL], sr[CTL]; } mixacc_t;   /* dry L, R; chorus, delay, reverb sends */
static mixacc_t acc[2] __attribute__((section(".lowram")));          /* core 0's, core 1's (.lowram: app.ld, main.c) */
static int32_t wet_l[CTL], wet_r[CTL];
static int32_t part_buf[2][CTL] __attribute__((section(".lowram")));   /* a part's scratch, one per core */
static int32_t drum_buf[4][CTL] __attribute__((section(".lowram")));   /* the drums through the drum track's FILTER: L, R,
                                                                        * reverb, delay */

/* ---- mute / solo: a track that goes silent fades out over ~6 ms (and back in) */
#define MUTE_STEP 4096                                  /* Q15 per block: 8 blocks */
static int32_t gain_next(track_t *t)                    /* the track's mute gain at the end of this block */
{
    int32_t to = trk_silent(t) ? 32767 : 0, a = t->att;
    t->att = a < to ? (to - a > MUTE_STEP ? a + MUTE_STEP : to) : (a - to > MUTE_STEP ? a - MUTE_STEP : to);
    return 32767 - t->att;
}

/* ---- DUCK: every kick (the drum track's KICK / KICK 2) dips the synth parts, which come back over an
 * eighth note: depth G_DUCK, the curve (1 - t / T)^2. drum_on sets drums.kick. */
static struct {
    uint32_t t;                                         /* units since the kick */
    int32_t g0, g1;                                     /* the parts' gain at the block start, end (Q15) */
} duck = {0xFFFFFFFFu, 32767, 32767};

static void duck_block(uint32_t adv)
{
    int32_t depth = song.g[G_DUCK] * 258, x;
    uint32_t len = BEAT_U / 2u;
    duck.g0 = duck.g1;
    if (drums.kick) {
        drums.kick = 0;
        duck.t = 0;
    }
    if (!depth || duck.t >= len) {
        duck.g1 = 32767;
        return;
    }
    x = 32767 - (int32_t)((duck.t << 10) / (len >> 5));      /* 1 - t / T, Q15 (t < len < 2^21) */
    if (x < 0)
        x = 0;
    duck.g1 = 32767 - mulq15(depth, mulq15(x, x));
    duck.t = duck.t + adv < duck.t ? 0xFFFFFFFFu : duck.t + adv;
}

/* ---- the DJ filter: v < 0 a low-pass closing, > 0 a high-pass opening, 0 off. The cutoff glides
 * to the knob (no zipper); at 0 it opens fully, then the filter is bypassed. On the master (G_FILT)
 * and, SLOOP 2.4, on each track (P_TFLT: a synth part's mono signal, the drum track's left, right,
 * reverb send and, 2.5, delay send) */
typedef struct {
    int32_t cut;                                        /* now, 0..127 << 8 (CUTOFF_HZ index) */
    int8_t mode;                                        /* -1 LP, 1 HP, 0 off */
    uint8_t seed;                                       /* (2.6) bit per channel: the LP's state from its next sample */
    int32_t z[4][2];                                    /* the SVF states, per channel */
} djf_t;
static djf_t djf, tflt[NTRK];

/* (2.7.2) the track filter's SVF resonance (tsvf_coef's reso) for the FILTER RESO setting: 40 (clean, as the filter
 * always was) at 0, up to 110 at 10 (a strong but still stable peak; tflt_run leaves 4x headroom for it). The master
 * DJ filter (G_FILT) and the tape TONE keep the fixed 40. */
static int32_t tflt_reso(void) { return 40 + (int32_t)(flt_reso > 10u ? 10u : flt_reso) * 7; }
/* the knob v -> this block's coefficients in *c; 0 = bypassed (nothing to do). reso: tsvf_coef's resonance */
static int djf_block(djf_t *f, int32_t v, tsvf_t *c, int32_t reso)
{
    int32_t to;
    if (v < 0 && f->mode >= 0) {                        /* (switching side: from open) */
        f->mode = -1;
        f->cut = 127 << 8;
        memset(f->z, 0, sizeof f->z);
        f->seed = 15;                                   /* (2.6) the low-pass starts where the signal is: from rest
                                                         * its output began at 0 under a sounding note (a click) */
    } else if (v > 0 && f->mode <= 0) {
        f->mode = 1;
        f->cut = 0;
        memset(f->z, 0, sizeof f->z);
    }
    if (!f->mode)
        return 0;
    to = f->mode < 0 ? (v < 0 ? (127 << 8) + v * 90 * 4 : 127 << 8) : (v > 0 ? v * 90 * 4 : 0);
    f->cut += clamp(to - f->cut, -384, 384);            /* ~1.5 index a block */
    if (!v && f->cut == to) {
        f->mode = 0;                                    /* fully open again: off */
        return 0;
    }
    tsvf_coef(c, f->cut, reso);
    return 1;
}
/* n samples of channel ch through the filter (x within +-140000) */
static void djf_run(djf_t *f, const tsvf_t *c, int32_t *b, uint32_t n, uint32_t ch)
{
    uint32_t i;
    int32_t *z = f->z[ch];
    if ((f->seed >> ch) & 1u) {
        f->seed &= (uint8_t)~(1u << ch);
        if (f->mode < 0 && n) {
            z[0] = 0;                                   /* at rest on this input: the output is it (DC: exact) */
            z[1] = clamp(b[0], -140000, 140000);
        }
    }
    for (i = 0; i < n; i++) {
        int32_t x = clamp(b[i], -140000, 140000), y = tsvf_lp(c, x, &z[0], &z[1]);
        b[i] = f->mode < 0 ? y : x - y;
    }
}
/* a track's filter on a signal at 4x its level (b >> 2 into the filter's range, back << 2) */
static void tflt_run(djf_t *f, const tsvf_t *c, int32_t *b, uint32_t n, uint32_t ch)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        b[i] >>= 2;
    djf_run(f, c, b, n, ch);
    for (i = 0; i < n; i++)
        b[i] <<= 2;
}

/* INSERT (SLOOP 2.6, after Felucca 1.5 by Leo Kuroshita, Discussions #78 and #177): one effect per track, after DIST
 * and before the SLICER, with a dry / wet MIX. TYPE (P_ITYPE, N_ITYPE; stored, append-only) and its three values A B C
 * (P_IA .. P_IC, what they mean: params.c ins_desc):
 *   SOFT HARD FOLD FUZZ  DRIVE (1x .. ~13x, squared), TONE (two one-pole low-passes at F_CUTOFF), LEVEL (F_DB, as the
 *                        track's LEVEL: 112 is 0 dB). A slow low cut in front (~27 Hz), a DC blocker after (FUZZ is
 *                        lopsided). SOFT tanh, HARD a flat clip at a voice's level, FOLD a triangle folder (the wave
 *                        reflected at +-16384: every fold more harmonics), FUZZ a hard top and a soft, halved bottom;
 *   CRUSH                BITS (1 .. 16, of a voice's full scale), RATE (a held sample, 689 Hz .. 44.1 kHz, INS_HOLD:
 *                        aliasing as the old samplers), LPF (two one-pole low-passes at F_CUTOFF after them);
 *   PHASR                RATE (F_LFOHZ), DEPTH (the sweep around ~850 Hz: 0 still .. the whole 150 Hz .. 4.8 kHz),
 *                        FDBK (0 .. 0.7): four all-pass stages, mono, per track;
 *   FLANG CHOR           RATE, DEPTH, FDBK: a 256-sample line per track (5.8 ms); FLANG swept 0.25 .. 5 ms (a squared
 *                        triangle), feedback 0 .. 0.85; CHOR a sine around 3.2 ms (+-2.3 ms at DEPTH 127), feedback
 *                        0 .. 0.4. A DC blocker on the swept copy (a clipped loud input is lopsided).
 * The swept ones are the pedals': their wet is (dry + swept) / 2, the notches need both, so MIX 100 % is the whole
 * effect and less a lighter one. MIX (P_IMIX) blends dry and wet linearly; MIX 0 or TYPE OFF: the track bit for bit
 * as before (nothing runs). MIX glides (2.9 ms, SL_SLOPE); a TYPE change fades the old one out, clears the state,
 * fades the new one in; so does a start from MIX 0 (no stale line or filter).
 * The drum track (SLOOP): its left, right, reverb send and delay send each through their own state, the same settings
 * (fx.c mix_block). State in RAM (.bss): 580 B a channel; noinline, its own loops */
enum { IT_OFF, IT_SOFT, IT_HARD, IT_FOLD, IT_FUZZ, IT_CRUSH, IT_PHASER, IT_FLANGER, IT_CHORUS, IT_N };
_Static_assert(IT_N == sizeof N_ITYPE / sizeof N_ITYPE[0], "the INSERT's types as params.c names them");
#define IL_LEN 256u                     /* FLANG / CHOR: the line, samples (a power of 2: masked) */
#define PH_ST 4u                        /* PHASR: all-pass stages (two notches, the classic pedal's) */
static const uint8_t INS_HOLD[16] = {64, 48, 40, 32, 24, 20, 16, 12, 10, 8, 6, 5, 4, 3, 2, 1};   /* CRUSH RATE */
/* PHASR: the stages' coefficient over the sweep (Q14), 33 points */
static const int16_t PH_A[33] = {
    -16038, -15998, -15955, -15906, -15853, -15793, -15727, -15653, -15572, -15481, -15381, -15270, -15147, -15012,
    -14862, -14696, -14514, -14313, -14092, -13849, -13582, -13290, -12970, -12621, -12240, -11825, -11375, -10886,
    -10356, -9784, -9166, -8500, -7783,
};
typedef struct {
    int32_t w;                          /* the wet share now, Q15 */
    uint8_t type;                       /* the type running */
    uint8_t cnt;                        /* CRUSH: samples left of the held one */
    uint16_t wp;                        /* FLANG / CHOR: the line's next write */
    uint32_t ph;                        /* the sweep's phase */
    int32_t hp, lp1, lp2, dc, hold;     /* low cut (Q8), the two low-passes, DC blocker (Q8), CRUSH's held sample */
    int32_t px[PH_ST], py[PH_ST], fb;   /* PHASR: each stage's last input and output, the fed back */
    int16_t line[IL_LEN];               /* FLANG / CHOR, at a quarter (headroom) */
} ins_t;
/* (in .bss, not the pool: tools/build.py keeps 8 KiB of the pool spare; RAM has room) */
static ins_t ins[NPART + 4u];                    /* the parts; the drum track's L, R, REV, DLY */

static inline int32_t ins_mixq(int32_t m) { return m >= 127 ? 32768 : m <= 0 ? 0 : m * 258; }   /* MIX -> Q15 */
static inline int32_t ins_mix(int32_t x, int32_t y, int32_t w) { return w == 32768 ? y : x + mulq16(y - x, (uint32_t)w << 1); }
static inline uint32_t ins_tri(uint32_t q) { uint32_t t = q >> 16; return t < 32768u ? t << 1 : (65535u - t) << 1; }
/* the one-pole low-pass (topology-preserving) coefficient of cutoff index c, Q16: G = g / (1 + g), g = tan(pi fc / FS) */
static int32_t ins_lpk(int32_t c)
{
    uint32_t g = SVF_G[clamp(c, 0, 127)];
    return (int32_t)((g << 16) / (4096u + g));
}
/* the drives' and CRUSH's shape of v (a voice's full scale 32768; v already through DRIVE) */
static inline int32_t ins_shape(uint32_t ty, int32_t v, uint32_t sh)
{
    switch (ty) {
    case IT_SOFT:
        return softclip(v);
    case IT_HARD:
        return clamp(v, -24000, 24000);
    case IT_FOLD: {
        int32_t u = (int32_t)((uint32_t)(v + 16384) & 0xFFFFu) - 32768;   /* a triangle of period 65536: +-16384 */
        u = 16384 - (u < 0 ? -u : u);
        return u + (u >> 1);                            /* x 1.5 (no divide) */
    }
    case IT_FUZZ:
        return v > 0 ? (v > 20000 ? 20000 : v) : softclip(v) >> 1;
    case IT_CRUSH:                                      /* sh: 16 - BITS; to the nearest step */
        return sh ? ((v + (1 << (sh - 1u))) >> sh) << sh : v;
    default:
        return v;
    }
}
static inline uint32_t ins_bits_sh(int32_t a) { return 15u - (uint32_t)(clamp(a, 0, 127) >> 3); }   /* BITS 1..16 */

/* SOFT .. FUZZ: low cut -> DRIVE -> shape -> TONE -> LEVEL -> DC blocker; CRUSH: held sample -> bits -> LPF. The state
 * in locals for the block (b may alias it as far as the compiler knows: no load / store a sample), written back */
static __attribute__((noinline)) void ins_drive(ins_t *s, const int16_t *p, int32_t *b, uint32_t n, int32_t tw)
{
    uint32_t i, ty = s->type, sh = ins_bits_sh(p[P_IA]);
    uint32_t hold = INS_HOLD[clamp(p[P_IB], 0, 127) >> 3];
    int32_t a = clamp(p[P_IA], 0, 127), g = 4096 + a * a * 3;   /* DRIVE, Q12: 1x .. 12.8x, gentle at first */
    int32_t w = s->w, lp1 = s->lp1, lp2 = s->lp2, x, y, v;
    if (ty == IT_CRUSH) {
        uint32_t k = (uint32_t)ins_lpk(p[P_IC]), cnt = s->cnt;
        int32_t hd = s->hold;
        for (i = 0; i < n; i++) {
            x = b[i];
            w += clamp(tw - w, -SL_SLOPE, SL_SLOPE);
            if (!cnt) {
                hd = ins_shape(IT_CRUSH, clamp(x, -262144, 262143), sh);
                cnt = hold;
            }
            cnt--;
            y = hd << 4;                                /* (x16: the low cutoffs' rounding leaves no offset) */
            v = mulq16(y - lp1, k);                     /* LPF: two one-pole low-passes */
            y = v + lp1;
            lp1 = y + v;
            v = mulq16(y - lp2, k);
            y = v + lp2;
            lp2 = y + v;
            b[i] = ins_mix(x, (y + 8) >> 4, w);
        }
        s->cnt = (uint8_t)cnt;
        s->hold = hd;
    } else {
        uint32_t k = (uint32_t)ins_lpk(p[P_IB]);
        int32_t lvl = LEVEL_Q12[p[P_IC] & 127], hp = s->hp, dc = s->dc;
        for (i = 0; i < n; i++) {
            x = b[i];
            w += clamp(tw - w, -SL_SLOPE, SL_SLOPE);
            hp += ((x << 8) - hp) >> 8;                 /* ~27 Hz low cut (Q8: no offset left by the rounding) */
            v = clamp(x - (hp >> 8), -230000, 230000);
            y = ins_shape(ty, ((v >> 3) * g) >> 9, 0) << 4;   /* (>> 3 first: 32 bits; x16 as CRUSH) */
            v = mulq16(y - lp1, k);                     /* TONE: two one-pole low-passes */
            y = v + lp1;
            lp1 = y + v;
            v = mulq16(y - lp2, k);
            y = v + lp2;
            lp2 = y + v;
            y = (((y + 8) >> 4) * lvl) >> 12;           /* LEVEL */
            dc += ((y << 8) - dc) >> 9;                 /* DC blocker, ~14 Hz (FUZZ's lopsided halves) */
            b[i] = ins_mix(x, y - (dc >> 8), w);
        }
        s->hp = hp;
        s->dc = dc;
    }
    s->w = w;
    s->lp1 = lp1;
    s->lp2 = lp2;
}

/* PHASR, FLANG, CHOR: the sweep at RATE (one LFO per state), DEPTH, FDBK; the state in locals for the block */
static __attribute__((noinline)) void ins_swept(ins_t *s, const int16_t *p, int32_t *b, uint32_t n, int32_t tw)
{
    uint32_t i, inc = LFO_INC[clamp(p[P_IA], 0, 127)] / CTL, dep = (uint32_t)clamp(p[P_IB], 0, 127);
    int32_t c = clamp(p[P_IC], 0, 127), w = s->w, dc = s->dc;
    if (s->type == IT_PHASER) {                         /* the coefficients a block at a time (at its middle) */
        uint32_t tr = ins_tri(s->ph + (n >> 1) * inc), q = 32768u + (uint32_t)(((int32_t)tr - 32768) * (int32_t)dep / 127);
        int32_t a = PH_A[q >> 11] + (((PH_A[(q >> 11) + 1u] - PH_A[q >> 11]) * (int32_t)(q & 2047u)) >> 11);
        int32_t fbk = c * 180, fb = s->fb;              /* 0 .. 0.7, Q15 */
        int32_t x0 = s->px[0], x1 = s->px[1], x2 = s->px[2], x3 = s->px[3];
        int32_t y0 = s->py[0], y1 = s->py[1], y2 = s->py[2], y3 = s->py[3];
        _Static_assert(PH_ST == 4u, "ins_swept: four all-pass stages");
        for (i = 0; i < n; i++) {
            int32_t x = b[i], u = clamp((x + 4) >> 3, -16383, 16383) + ((fb * fbk + 16384) >> 15), y;
            w += clamp(tw - w, -SL_SLOPE, SL_SLOPE);
            /* y = a (u - y1) + u1, four times */
            y = clamp(x0 + ((a * (u - y0) + 8192) >> 14), -65535, 65535), x0 = u, y0 = y, u = y;
            y = clamp(x1 + ((a * (u - y1) + 8192) >> 14), -65535, 65535), x1 = u, y1 = y, u = y;
            y = clamp(x2 + ((a * (u - y2) + 8192) >> 14), -65535, 65535), x2 = u, y2 = y, u = y;
            y = clamp(x3 + ((a * (u - y3) + 8192) >> 14), -65535, 65535), x3 = u, y3 = y;
            fb = clamp(y, -32767, 32767);
            y <<= 2;                                    /* (y: at an eighth) */
            dc += ((y << 8) - dc) >> 9;                 /* DC blocker, ~14 Hz (a clipped loud input is lopsided) */
            b[i] = ins_mix(x, (x >> 1) + y - (dc >> 8), w);   /* (dry + all-passed) / 2 */
        }
        s->px[0] = x0, s->px[1] = x1, s->px[2] = x2, s->px[3] = x3;
        s->py[0] = y0, s->py[1] = y1, s->py[2] = y2, s->py[3] = y3;
        s->fb = fb;
        s->ph += n * inc;
    } else {
        int32_t chorus = s->type == IT_CHORUS, fbk = c * (chorus ? 103 : 219);   /* 0 .. 0.4 / 0 .. 0.85, Q15 */
        uint32_t dq = dep * 516u, ph = s->ph, wp = s->wp; /* DEPTH, Q16 */
        int16_t *line = s->line;
        for (i = 0; i < n; i++, ph += inc) {
            int32_t x = b[i], r0, r1, v, fr;
            uint32_t d;                                 /* the delay, Q16 samples */
            w += clamp(tw - w, -SL_SLOPE, SL_SLOPE);
            if (chorus) {
                d = (uint32_t)((140 << 16) + ((osc_sine(ph) * (int32_t)dep * 50) >> 5));   /* 3.2 +- 2.3 ms */
            } else {
                uint32_t tr = ins_tri(ph), sw = (tr * tr) >> 16;   /* longer near the short end */
                d = (11u << 16) + 209u * (((sw >> 1) * dq) >> 15);  /* 0.25 .. 5 ms */
            }
            r0 = (int32_t)(wp - (d >> 16)) & (int32_t)(IL_LEN - 1u);
            r1 = (r0 - 1) & (int32_t)(IL_LEN - 1u);
            fr = (int32_t)((d >> 1) & 0x7FFFu);
            v = line[r0] + (((line[r1] - line[r0]) * fr) >> 15);
            line[wp] = (int16_t)clamp((x >> 2) + softclip(mulq15(v, fbk)), -32767, 32767);
            wp = (wp + 1u) & (IL_LEN - 1u);
            v <<= 1;                                    /* (v: at a quarter) */
            dc += ((v << 8) - dc) >> 9;                 /* DC blocker (the line's clip on a loud lopsided wave) */
            b[i] = ins_mix(x, (x >> 1) + v - (dc >> 8), w);   /* (dry + delayed) / 2 */
        }
        s->ph = ph;
        s->wp = (uint16_t)wp;
    }
    s->w = w;
    s->dc = dc;
}

/* the INSERT of the settings p on one channel b through state s: 1 while it runs (or fades out); 0: dry, bit for bit */
static __attribute__((noinline)) int ins_run(ins_t *s, const int16_t *p, int32_t *b, uint32_t n)
{
    int32_t ty = clamp(p[P_ITYPE], 0, IT_N - 1), tw;
    if (s->type != ty && !s->w)                         /* faded out: the new type from rest */
        s->type = (uint8_t)ty;
    tw = s->type == ty && ty ? ins_mixq(p[P_IMIX]) : 0;
    if (!s->w) {
        if (!tw)
            return 0;                                   /* dry: bit for bit */
        memset(s, 0, sizeof *s);                        /* starting: no stale line or filter */
        s->type = (uint8_t)ty;
    }
    if (s->type >= IT_PHASER)
        ins_swept(s, p, b, n, tw);
    else
        ins_drive(s, p, b, n, tw);
    return s->w != 0;
}
/* a part's INSERT on its block b (mix_part): nothing while TYPE is OFF or MIX is 0 and faded */
static void track_insert(track_t *t, int32_t *b, uint32_t n)
{
    t->ins_run = (uint8_t)ins_run(&ins[(uint32_t)(t - trk) % NPART], t->p, b, n);
}
/* the INSERT at rest at once (mix_part: TYPE turned OFF while the part was silent, its fade never ran) */
static __attribute__((noinline)) void ins_rest(track_t *t)
{
    memset(&ins[(uint32_t)(t - trk) % NPART], 0, sizeof(ins_t));
    t->ins_run = 0;
}

/* one synth part into the dry mix and the sends; a part with no voice sounding costs
 * the LFO tick and a cleared buffer only (after the DIST tail has run out) */
static void mix_part_run(track_t *t, uint32_t n, mixacc_t *mx, int32_t *b);
/* (2.6) a preset's fade (voice.c sound_change): the whole part rendered with the sound it had, then the new one back.
 * The sound set aside per core, as part_buf is (a part may fade on each core at once) */
static void mix_part(track_t *t, uint32_t n, mixacc_t *mx, int32_t *b)   /* into the set mx, through the scratch b */
{
    static int16_t pnew[2][P_COUNT] __attribute__((section(".lowram")));
    int16_t *pn = pnew[mx != &acc[0]];
    if (!(t->pold && t->xf_on && t->xf)) {
        mix_part_run(t, n, mx, b);
        return;
    }
    memcpy(pn, t->p, sizeof pnew[0]);
    memcpy(t->p, t->p_old, sizeof t->p);
    mix_part_run(t, n, mx, b);
    memcpy(t->p, pn, sizeof pnew[0]);
}
static void mix_part_run(track_t *t, uint32_t n, mixacc_t *mx, int32_t *b)
{
    uint32_t i;
    int32_t g0 = 32767 - t->att, g1 = gain_next(t);
    if (track_render(t, b, n))
        t->tail = t->p[P_ITYPE] ? 64 : 16, t->ins_xt = 0;              /* blocks of DIST / INSERT state to run out after the last
                                                         * voice (the INSERT's: 46 ms, a flanger's feedback) */
    else if ((!t->tail || !(t->p[P_DIST] | t->p[P_ITYPE]) || !--t->tail) && !slicer_busy(t) &&
             !(t->ins_run && t->ins_res > 16u && t->ins_xt < 255u && ++t->ins_xt)) {   /* (2.6: the INSERT's DC
                                                         * blocker run out first, up to 255 blocks more: no step) */
        slicer_track(t, 0, n);                          /* (the SLICER's step clock runs on) */
        if (t->ins_run && !t->p[P_ITYPE])
            ins_rest(t);                                /* TYPE OFF while silent: no fade-out left for the next note */
        return;
    }
    if (!g0 && !g1) {                                   /* silent (MUTE / SOLO): the voices run, nothing is heard */
        slicer_track(t, 0, n);
        return;
    }
    {
        int32_t lvl = expr_gain(LEVEL_Q12[t->p[P_LEVEL] ? clamp(t->p[P_LEVEL] + t->p[P_ED_FX], 1, 127) : 0], (uint32_t)(t - trk)), pan = t->p[P_PAN];   /* (+ the sound's trim: 1/2 dB steps, as LEVEL's) */
        int32_t gl = 4096 - (pan > 0 ? pan * 64 : 0), gr = 4096 + (pan < 0 ? pan * 64 : 0);
        int32_t c = t->p[P_CHOR] * 258, d = send_thr(t->p[P_DLY] * 258, thr_dly), r = send_thr(t->p[P_REV] * 258, thr_rev);
        int32_t pk = t->peak;
        int32_t xmax = c > d ? c : d;
        int32_t ga = mulq15(g0, duck.g0), gb = mulq15(g1, duck.g1);   /* mute x duck, ramped over the block */
        xmax = 0x7FFFFFFF / ((xmax > r ? xmax : r) | 1);   /* sends: loud chords at a high LEVEL */
        track_dist(t, b, n);
        if (t->p[P_ITYPE] | t->ins_run) {
            track_insert(t, b, n);                      /* the INSERT (2.6): off, nothing */
            t->ins_res = (uint16_t)(b[n - 1u] < 0 ? (-b[n - 1u] > 65535 ? 65535 : -b[n - 1u])
                                                  : (b[n - 1u] > 65535 ? 65535 : b[n - 1u]));   /* (its tail's end) */
        }
        slicer_track(t, b, n);                          /* slicer.c: before the level, pan and sends */
        {
            tsvf_t fc;                                  /* the track's FILTER (P_TFLT), after the SLICER */
            djf_t *f = &tflt[(uint32_t)(t - trk) % NTRK];
            if (djf_block(f, t->p[P_TFLT], &fc, tflt_reso()))
                tflt_run(f, &fc, b, n, 0);
        }
        int32_t lvl0, dl, gl0, dgl, gr0, dgr;          /* (2.7) LEVEL (and a new sound's trim) and PAN glide (drums.c
                                                         * mix_glide), ramped sample by sample inside the block */
        if (!t->mix_on)
            t->lvl = lvl, t->gpl = gl, t->gpr = gr, t->mix_on = 1;
        lvl0 = t->lvl, gl0 = t->gpl, gr0 = t->gpr;
        t->lvl = mix_glide(lvl0, lvl), t->gpl = mix_glide(gl0, gl), t->gpr = mix_glide(gr0, gr);
        dl = (t->lvl - lvl0) >> CTL_LOG2, dgl = t->gpl - gl0, dgr = t->gpr - gr0;
        for (i = 0; i < n; i++) {
            int32_t lv = lvl0 + dl * (int32_t)i, bi = b[i], x = MULQ12R(bi, lv, 2), a;   /* (#4) rounded, all of b: 8 loud
                                                         * voices in 32 bits as ((b >> 2) * lv) >> 10 was */
            int32_t xs, g = ga + (((gb - ga) * (int32_t)i) >> CTL_LOG2);
            if (g < 32767) {                            /* (always, at rest: mulq15(32767, 32767) is 32766) */
                g >>= 3;                                /* (Q12: no 32-bit overflow) */
                x = MULQ12R(x, g, 4);
            }
            a = x < 0 ? -x : x;
            xs = clamp(x, -xmax, xmax);                 /* sends: mulq15 would overflow */
            if (a > pk)
                pk = a;
            if (c)
                mx->sc[i] += mulq15(xs, c);
            if (d)
                mx->sd[i] += mulq15(xs, d);
            if (r)
                mx->sr[i] += mulq15(xs, r);
            gl = gl0 + ((dgl * (int32_t)i) >> CTL_LOG2), gr = gr0 + ((dgr * (int32_t)i) >> CTL_LOG2);   /* (2.7) */
            mx->l[i] += (x * (gl >> 4) + 128) >> 8;     /* (x may pass 2^19: PAN's Q12 to Q8, exact at rest: 64 a step;
                                                         * #4: x >> 4 floored the part to 16 LSB) */
            mx->r[i] += (x * (gr >> 4) + 128) >> 8;
        }
        t->peak = pk;
    }
}

/* ---- DUST: the master through an old sampler and a record. G_DUST 0..127 turns up together: drive
 * into a soft clip, a lower sample rate (held samples, 44.1 -> 11 kHz), fewer bits (15 -> 8), a
 * one-pole low-pass (open -> ~3 kHz), a little hiss and crackle. The hiss and the crackle are the
 * record turning: they fade in with PLAY and out (~0.1 s) at STOP, so a stopped SLOOP is silent.
 * Stereo, ~25 ops a sample. */
static struct {
    int32_t hl, hr, hn;                                 /* held samples, samples left to hold */
    int32_t ll, lr;                                     /* low-pass states */
    int32_t rnd, click;                                 /* noise state, a crackle decaying */
    int32_t bed;                                        /* hiss / crackle level, Q15: 0 stopped, 32767 playing */
} dust = {0, 0, 0, 0, 0, 0x2545F491, 0, 0};

static int32_t crush_bits(int32_t v, int32_t shift)    /* fewer bits, rounded toward 0: no DC from tails */
{
    return v >= 0 ? (v >> shift) << shift : -((-v >> shift) << shift);
}

static void dust_process(int32_t *l, int32_t *r, uint32_t n)
{
    int32_t d = song.g[G_DUST], hold, shift, a, drive, hiss, i, bed0, bed1;
    uint32_t pc;
    if (!d) {
        dust.bed = 0;
        return;
    }
    hold = 1 + d * 3 / 127;
    shift = d / 18;
    a = 32767 - d * 165;                                /* one-pole coefficient, Q15 */
    drive = 4096 + d * 24;                              /* Q12: 1x .. 1.75x */
    hiss = d * 2;
    pc = (uint32_t)d * 7u;                              /* crackle: chance per sample, x 2^-22 */
    bed0 = dust.bed;                                    /* ~0.1 s from 0 to full (238 a block of 32) */
    bed1 = dust.bed = clamp(dust.bed + (song.playing ? 238 : -238), 0, 32767);
    for (i = 0; i < (int32_t)n; i++) {
        int32_t x = l[i], y = r[i];
        int32_t bed = bed0 + (((bed1 - bed0) * i) >> CTL_LOG2);
        uint32_t nz = noise32(&dust.rnd);
        if (--dust.hn <= 0) {                           /* sample and hold, then the bits */
            dust.hn = hold;
            dust.hl = softclip(((x >> 2) * drive) >> 10);
            dust.hr = softclip(((y >> 2) * drive) >> 10);
            if (shift) {
                dust.hl = crush_bits(dust.hl, shift);
                dust.hr = crush_bits(dust.hr, shift);
            }
        }
        dust.ll += mulq15(dust.hl - dust.ll, a);
        dust.lr += mulq15(dust.hr - dust.lr, a);
        if ((nz >> 10) < pc)                            /* a speck of dust */
            dust.click = mulq15(((int32_t)(nz & 0x3FFu) - 512) * d / 8, bed);
        x = dust.ll + dust.click + mulq15(((int32_t)(nz >> 16) - 32768) * hiss >> 15, bed);
        y = dust.lr + dust.click + mulq15(((int32_t)(nz & 0xFFFFu) - 32768) * hiss >> 15, bed);
        dust.click -= dust.click >> 2;
        l[i] = x;
        r[i] = y;
    }
}

static void djf_process(int32_t *l, int32_t *r, uint32_t n)
{
    tsvf_t c;
    if (!djf_block(&djf, song.g[G_FILT], &c, 40))
        return;
    djf_run(&djf, &c, l, n, 0);
    djf_run(&djf, &c, r, n, 1);
}

#include "punch.c"            /* PUNCH-IN FX on the whole mix (FX held + a white key) */
/* ---- the render units and the second core. A unit is a synth part (mix_part) or the drum track with the
 * metronome (mix_drums); unit k is bit k of a mask, the drums bit NPART. What a unit on core 1 may not share
 * with one on core 0: part_buf (one per core); FM6's operator buses (per part, in its arena); the formant
 * engine's breath noise, one xorshift for all its voices (two FORMANT parts stay together, on core 0); and
 * rng() (libc.c): a part whose LFO wraps this block (track_lfo_tick draws) or whose PHYS model changed under
 * a note (phys_render resets with a draw) stays on core 0, where the draws keep their order (units_split).
 * SLOOP 2.6 added two things a part keeps for its block, now per part or per core: the ENV SYNC steps (voice.c
 * esync_k) and the preset fade's sound set aside (mix_part pnew); its INSERT state is per part (ins[]), the drum
 * track's its own; TEXTURE, the THROWs' glide, FREEZE and the look-ahead limiter run on core 0 outside the units. */
#define U_DRUMS (1u << NPART)
#define U_ALL ((2u << NPART) - 1u)

/* the drum track's INSERT (2.6): its left, right and the two sends, each through its own state, the same settings */
static void drum_insert(int32_t *l, int32_t *r, int32_t *v, int32_t *d, uint32_t n)
{
    track_t *t = TDRUM;
    int run;
    if (!(t->p[P_ITYPE] | t->ins_run))
        return;
    run = ins_run(&ins[NPART], t->p, l, n);
    run |= ins_run(&ins[NPART + 1u], t->p, r, n);
    run |= ins_run(&ins[NPART + 2u], t->p, v, n);
    run |= ins_run(&ins[NPART + 3u], t->p, d, n);
    t->ins_run = (uint8_t)run;
}
/* the drum track with the metronome: drums_render, through the SLICER when it is on (slicer.c), then through its
 * INSERT (2.6) and the track's FILTER when they are on (into drum_buf, left, right and the two sends apart, then added) */
static void mix_drums(uint32_t n, mixacc_t *mx)
{
    uint32_t i;
    tsvf_t fc;
    djf_t *f = &tflt[TRK_DRUM];
    int flt;
    drums.a0 = TDRUM->att;                              /* the drum track's mute / solo fade */
    drums.a1 = 32767 - gain_next(TDRUM);
    flt = n <= CTL && djf_block(f, TDRUM->p[P_TFLT], &fc, tflt_reso());   /* the drum track's FILTER */
    if (flt || (n <= CTL && (TDRUM->p[P_ITYPE] | TDRUM->ins_run))) {   /* .. or its INSERT: left, right, the sends */
        int32_t *dl = drum_buf[0], *dr = drum_buf[1], *dv = drum_buf[2], *dd = drum_buf[3];
        for (i = 0; i < n; i++)
            dl[i] = dr[i] = dv[i] = dd[i] = 0;
        slicer_drums(dl, dr, dv, dd, n);
        drum_insert(dl, dr, dv, dd, n);                 /* (2.6: off, nothing) */
        if (flt) {
            tflt_run(f, &fc, dl, n, 0);
            tflt_run(f, &fc, dr, n, 1);
            tflt_run(f, &fc, dv, n, 2);
            tflt_run(f, &fc, dd, n, 3);
        }
        for (i = 0; i < n; i++) {
            mx->l[i] += dl[i];
            mx->r[i] += dr[i];
            mx->sr[i] += dv[i];
            mx->sd[i] += dd[i];
        }
    } else {
        slicer_drums(mx->l, mx->r, mx->sr, mx->sd, n);   /* drums_render, through the SLICER when on */
    }
    click_render(mx->l, mx->r, n);                        /* the metronome: its own voice (drums.c), any kit, any mute */
}

/* the units of mask into the set mx, in their order */
static void mix_units(uint32_t mask, uint32_t n, mixacc_t *mx, int32_t *scratch)
{
    uint32_t k;
    for (k = 0; k < NPART; k++)
        if (mask & (1u << k))
            mix_part(&trk[k], n, mx, scratch);
    if (mask & U_DRUMS)
        mix_drums(n, mx);
}

/* the load figures: core 1's time in its job and core 0's time waiting for it, in ticks over a half (cpu1.h
 * cpu1_ticks; the job's are written by core 1, read and cleared by core 0 while core 1 is idle), then as 1/256
 * of the half, smoothed as song.cpu_q8 is (audio.c calls cpu1_load_half after each half; console.c shows them) */
static uint32_t cpu1_job_ticks, cpu1_wait_ticks, cpu1_busy_q8, cpu1_wait_q8;
static void cpu1_load_half(uint32_t half_us)
{
    uint32_t per = half_us * CPU1_TICKS_PER_US / 256u;  /* ticks per 1/256 of the half */
    if (!per)
        per = 1;
    cpu1_busy_q8 = (cpu1_busy_q8 * 15u + cpu1_job_ticks / per) / 16u;
    cpu1_wait_q8 = (cpu1_wait_q8 * 15u + cpu1_wait_ticks / per) / 16u;
    cpu1_job_ticks = cpu1_wait_ticks = 0;
}

static void cpu1_units_job(uint32_t arg)                /* core 1's job: the units of arg & 0xFF, n = arg >> 8 */
{
    uint32_t t0 = cpu1_ticks();
    mix_units(arg & 0xFFu, arg >> 8, &acc[1], part_buf[1]);
    cpu1_job_ticks += cpu1_ticks() - t0;
}

/* the units core 1 renders this block: the last two parts (SFX, CALLS) and the drums (cpu1_units when set: the menu, the tests),
 * less the parts that must stay here (above) */
static uint32_t units_split(void)
{
    uint32_t mask = cpu1_units < 0 ? (3u << (NPART - 2u)) | U_DRUMS : (uint32_t)cpu1_units & U_ALL, k, nform = 0;   /* (RAPTOR:
                                                         * SFX, CALLS and the drums; SLOOP: its last part and the drums) */
    if (!mask)
        return 0;
    for (k = 0; k < NPART; k++)
        nform += ENGINES[trk[k].engine] == &ENG_FORMANT;
    for (k = 0; k < NPART; k++) {
        const track_t *t = &trk[k];
        const engine_t *e = ENGINES[t->engine];
        uint32_t fade = t->xf_on && t->xf, inc;
        const int16_t *p = fade && t->pold ? t->p_old : t->p;   /* the values this block renders with (mix_part: a
                                                         * preset fading out keeps its old sound; voice.c track_render:
                                                         * an engine fading out its old engine values, pe_old) */
        if (!(mask & (1u << k)))
            continue;
        inc = p[P_LSYNC] ? lfo_sync_inc(p) : LFO_INC[p[P_LRATE] & 127];   /* (2.6: LFO 2 SYNC, as track_lfo_tick) */
        if (t->lfo_ph + inc < t->lfo_ph || (e == &ENG_PHYS && phys_render_draws(t, fade ? t->pe_old[0] : p[P_E0])) ||
            (e == &ENG_FORMANT && nform > 1u))
            mask &= ~(1u << k);
    }
    return mask;
}

static int32_t master_cur = -1;                        /* the volume knob, ramped per sample (no zipper) */
/* ---- TEXTURE (SLOOP 2.6, GLO > TEXTURE): a looping bed under the mix, the glue of lo-fi and ambient tracks, without
 * taking a track. SRC: a generated noise (no memory, nothing to load) or a user sample slot looped (USR1..4, its first
 * zone at its own pitch; a looped zone from its loop, else whole), LEVEL, TONE (the DJ filter: left a low-pass, right a
 * high-pass), MOVE: how much it breathes over RATE (1 .. 16 bars of the tempo; its level and brightness, a sine from the
 * transport's start), WHEN: PLAY (fades in and out with the transport) or ALWAYS, DUCK: the kick pumps it as the
 * parts (GLO > MASTER > DUCK). Into the dry mix (no sends), before DUST, the punch-ins and the master filter.
 *   VINYL  hiss, rumble, crackle and the odd pop, each crackle somewhere left to right
 *   TAPE   hiss tilted dark, its level drifting slowly (flutter)
 *   RAIN   a soft bed and droplets, left and right apart
 *   WIND   noise through two resonant low-passes wandering slowly, gusts
 *   RADIO  a band of static (500 Hz .. 3 kHz), coarse, in bursts that come and go
 *   ROOM   room tone: a low rumble and a little air
 * Its level: x * LEVEL^2 / 16384 into the mix (the sources at about 8000 rms: LEVEL 64 about -30 dBFS rms out, 127
 * about -18). Off (and faded out): nothing runs, the mix bit for bit as without */
enum { TX_OFF, TX_VINYL, TX_TAPE, TX_RAIN, TX_WIND, TX_RADIO, TX_ROOM, TX_USR1, TX_N = TX_USR1 + 4 };
static struct {
    int32_t rnd;                                        /* noise state (never 0) */
    int32_t g;                                          /* gain now (Q15 of the LEVEL target), ramped a block */
    uint8_t src, play;                                  /* the source running; the transport as last seen */
    uint32_t ph;                                        /* MOVE's phase (a turn per RATE) */
    int32_t a[2], b[2], c[2], d[2], e[2];               /* per side: the sources' states (see texture_gen) */
    int32_t lpm[2];                                     /* MOVE's brightness (one-pole) */
    int32_t t0, t1, t2;                                 /* slow random walks / timers */
    tsvf_t wc[2];                                       /* WIND: this block's filters */
    int32_t wz[2][2];
    uint32_t upos, ufrac;                               /* USR: the read position, Q16 fraction */
    int32_t up, ui, us0, us1;                           /* USR: ADPCM predictor, step index, the two samples interpolated */
    uint32_t ugen;                                      /* USR: the slots' scan count (an upload restarts it) */
    int32_t ulast, uoff;                                /* USR: the last value out; the step at a wrap, gliding to 0
                                                         * (a sample that does not end where it starts: no click) */
    uint8_t uwrap;                                      /* USR: tx_usr_next went round */
    djf_t f;                                            /* TONE */
} tx = {.rnd = 0x2545F491};
static inline int32_t tx_n16(void) { return (int32_t)(noise32(&tx.rnd) >> 16) - 32768; }   /* -32768 .. 32767 */
static void texture_reset(uint32_t src)
{
    int32_t rnd = tx.rnd, g = tx.g;
    uint32_t ph = tx.ph;
    uint8_t play = tx.play;
    memset(&tx, 0, sizeof tx);
    tx.rnd = rnd ? rnd : 0x2545F491;
    tx.g = g, tx.ph = ph, tx.play = play;
    tx.src = (uint8_t)src;
    tx.t0 = 50 << 8, tx.t1 = 60 << 8;                  /* (WIND: the two cutoffs, RADIO / TAPE: the level) */
    tx.ugen = smp_user_gen - 1u;                        /* (USR: the zone looked up on the first block) */
}
/* USR k: the next sample of its first zone, looped (sample_next's decoder, its own state) */
static inline int32_t tx_usr_next(const smp_zone_t *z)
{
    uint32_t pos = tx.upos, b = SMP_DATA[z->off + (pos >> 1)];
    uint32_t code = (pos & 1u) ? (b >> 4) : (b & 15u);
    int32_t step = IMA_STEP[(uint32_t)tx.ui <= 88u ? tx.ui : 88], vd = step >> 3;
    if (code & 4u) vd += step;
    if (code & 2u) vd += step >> 1;
    if (code & 1u) vd += step >> 2;
    tx.up = clamp(tx.up + ((code & 8u) ? -vd : vd), -32768, 32767);
    tx.ui = clamp(tx.ui + IMA_IDX[code & 7u], 0, 88);
    if (++pos > (z->looped ? z->le : z->n - 1u)) {      /* round again: from the loop (its state), or the top */
        pos = z->looped ? z->ls : 0u;
        tx.up = z->looped ? z->pred : 0;
        tx.ui = z->looped ? z->idx : 0;
        tx.uwrap = 1;
    }
    tx.upos = pos;
    return tx.up;
}
/* n samples of source src (both sides) into l, r, at about 8000 rms */
static void texture_gen(uint32_t src, int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i, k;
    if (src >= TX_USR1) {
        uint32_t s = src - TX_USR1;
        const smp_zone_t *z = usr_nz[s] ? &usr_zone[s][0] : 0;
        if (!z || !z->n) {
            for (i = 0; i < n; i++)
                l[i] = r[i] = 0;
            return;
        }
        if (tx.ugen != smp_user_gen) {                  /* a new upload (or the first block): from the top */
            tx.ugen = smp_user_gen;
            tx.upos = 0, tx.ufrac = 0, tx.up = tx.ui = 0;
            tx.us0 = 0, tx.us1 = tx_usr_next(z);
            tx.uwrap = 1;                               /* (from where the old one was: no click) */
        }
        for (i = 0; i < n; i++) {
            int32_t v;
            tx.ufrac += z->rate;                        /* (its own rate: Q16 of 44.1 kHz) */
            while (tx.ufrac >= 65536u) {
                tx.ufrac -= 65536u;
                tx.us0 = tx.us1;
                tx.us1 = tx_usr_next(z);
            }
            v = (tx.us0 + (((tx.us1 - tx.us0) * (int32_t)(tx.ufrac >> 1)) >> 15)) >> 1;
            if (tx.uwrap) {                             /* (2.6) round again: from the value it was at */
                tx.uwrap = 0;
                tx.uoff = tx.ulast - v;
            }
            tx.uoff -= (tx.uoff >> 6) + (tx.uoff > 0) - (tx.uoff < 0);   /* (~1.5 ms) */
            l[i] = r[i] = tx.ulast = v + tx.uoff;
        }
        return;
    }
    switch (src) {
    case TX_VINYL:                                      /* a: rumble, b: crackle, c: its decay, d: hiss tilt */
        for (i = 0; i < n; i++) {
            uint32_t z = noise32(&tx.rnd);
            int32_t w = (int32_t)(z >> 16) - 32768, h;
            tx.a[0] += (w >> 5) - (tx.a[0] >> 7);       /* rumble: integrated, leaky */
            if ((z & 0xFFFFu) < 60u) {                  /* crackle, ~40 a second, anywhere left to right */
                int32_t amp = 6000 + (int32_t)((z >> 8) & 0x3FFFu) + ((z & 1u) ? 0 : 12000 * ((z & 0x30u) == 0u));
                int32_t pan = (int32_t)((z >> 20) & 0xFFu);
                amp = (z & 0x100u) ? -amp : amp;
                tx.b[0] = (amp * (256 - pan)) >> 8;
                tx.b[1] = (amp * pan) >> 8;
            }
            for (k = 0; k < 2u; k++) {
                h = (k ? (int32_t)(noise32(&tx.rnd) >> 16) - 32768 : w);
                tx.d[k] += (h - tx.d[k]) >> 2;          /* the hiss: dark, low (the crackle on top) */
                (k ? r : l)[i] = (tx.d[k] >> 4) + (tx.a[0] >> 2) + tx.b[k];
                tx.b[k] -= tx.b[k] >> 2;                /* (a click: a few samples) */
            }
        }
        break;
    case TX_TAPE:                                       /* t0: the level drifting (Q8), t2: its next target */
        if (!(tx.t1++ & 31))
            tx.t2 = (24 << 8) + (int32_t)((noise32(&tx.rnd) >> 20) & 0x7FFu) * 4;
        tx.t0 += (tx.t2 - tx.t0) >> 6;
        for (i = 0; i < n; i++)
            for (k = 0; k < 2u; k++) {
                int32_t w = tx_n16();
                tx.a[k] += (w - tx.a[k]) >> 2;          /* tilted dark: mostly the low-passed part */
                (k ? r : l)[i] = (((tx.a[k] >> 1) + (w >> 3)) * (tx.t0 >> 8)) >> 5;
            }
        break;
    case TX_RAIN:                                       /* a: the bed's low-pass, b: a droplet's level, c: its noise */
        for (i = 0; i < n; i++)
            for (k = 0; k < 2u; k++) {
                uint32_t z = noise32(&tx.rnd);
                int32_t w = (int32_t)(z >> 16) - 32768;
                tx.a[k] += (w - tx.a[k]) >> 4;          /* the bed: ~450 Hz */
                if ((z & 0xFFFFu) < 420u)               /* a droplet: ~280 a second a side */
                    tx.b[k] = 4000 + (int32_t)((z >> 4) & 0x3FFFu);
                tx.c[k] += (w - tx.c[k]) >> 1;
                (k ? r : l)[i] = (tx.a[k] >> 1) + (((w - tx.c[k]) * tx.b[k]) >> 14);
                tx.b[k] -= tx.b[k] >> 5;
            }
        break;
    case TX_WIND: {                                     /* t0, t1: the cutoffs wandering (Q8 index), t2: their targets */
        if (!(noise32(&tx.rnd) & 31u))
            tx.t2 = (int32_t)(noise32(&tx.rnd) & 0xFFFFu);   /* new targets now and then: 34 .. 78 */
        tx.t0 += (((34 << 8) + (tx.t2 & 0xFF) * 44) - tx.t0) >> 7;
        tx.t1 += (((34 << 8) + (tx.t2 >> 8) * 44) - tx.t1) >> 7;
        tsvf_coef(&tx.wc[0], tx.t0, 95);
        tsvf_coef(&tx.wc[1], tx.t1, 95);
        for (i = 0; i < n; i++)
            for (k = 0; k < 2u; k++) {
                int32_t y = tsvf_lp(&tx.wc[k], tx_n16() >> 2, &tx.wz[k][0], &tx.wz[k][1]);
                (k ? r : l)[i] = clamp(y, -32767, 32767);
            }
        break;
    }
    case TX_RADIO:                                      /* a, b: the band's two low-passes, t0: the burst level (Q8),
                                                         * t1: samples left of the burst, c: the held sample */
        if ((tx.t1 -= (int32_t)n) <= 0) {
            uint32_t z = noise32(&tx.rnd);
            tx.t1 = 1500 + (int32_t)((z >> 8) & 0x3FFFu);   /* 35 .. 400 ms */
            tx.t2 = (z & 3u) == 0u ? 40 << 8 : (110 + (int32_t)((z >> 24) & 0x7Fu)) << 8;   /* (a gap now and then) */
        }
        tx.t0 += (tx.t2 - tx.t0) >> 2;
        for (i = 0; i < n; i++) {
            int32_t w = tx_n16(), y;
            if (!(i & 3u))
                tx.c[0] = w;                            /* coarse: held four samples */
            tx.a[0] += (tx.c[0] - tx.a[0]) >> 1;        /* < ~3 kHz */
            tx.b[0] += (tx.a[0] - tx.b[0]) >> 4;        /* the lows taken out: > ~500 Hz */
            y = (((tx.a[0] - tx.b[0]) >> 1) * (tx.t0 >> 8)) >> 6;
            l[i] = y;
            r[i] = y - (y >> 2);
        }
        break;
    case TX_ROOM:                                       /* a: the rumble, b: its low-pass, c: air */
        for (i = 0; i < n; i++)
            for (k = 0; k < 2u; k++) {
                int32_t w = tx_n16();
                tx.a[k] += (w >> 4) - (tx.a[k] >> 8);
                tx.b[k] += (tx.a[k] - tx.b[k]) >> 5;
                (k ? r : l)[i] = (tx.b[k] >> 1) + (w >> 6);
            }
        break;
    default:
        for (i = 0; i < n; i++)
            l[i] = r[i] = 0;
        break;
    }
}
static void texture_mix(int32_t *l, int32_t *r, uint32_t n)
{
    static int32_t tl[CTL], tr[CTL];
    static const uint8_t BARS[5] = {1, 2, 4, 8, 16};
    uint32_t src = (uint32_t)clamp(song.g[G_TXSRC], 0, TX_N - 1), i, bar;
    /* the sources to about 8000 rms (VINYL ~5000: its crackle peaks), Q8; the user slots as they are (/ 2) */
    static const int16_t TX_SCALE[TX_USR1 + 1] = {0, 800, 301, 693, 1918, 149, 343, 256};
    int32_t lvl = clamp(song.g[G_TXLVL], 0, 127), on, g0, g1, mv, depth, u, lfo, k0, k1, sc;
    tsvf_t fc;
    if (song.playing && !tx.play)
        tx.ph = 0;                                      /* MOVE from the transport's start */
    tx.play = (uint8_t)song.playing;
    on = src && src == tx.src && (song.playing || song.g[G_TXWHEN]) && lvl;
    if (!tx.g && !on) {                                 /* silent: nothing runs (and a new source starts from rest) */
        if (src != tx.src)
            texture_reset(src);
        return;
    }
    if (n > CTL)
        return;
    g0 = tx.g;                                          /* fades over ~0.1 s (238 a block), as DUST's bed */
    g1 = tx.g = clamp(tx.g + (on ? 238 : -238), 0, 32767);
    bar = (uint32_t)FS * 240u / (uint32_t)clamp(song.g[G_BPM], 20, 300);   /* a bar of 4/4, samples */
    tx.ph += (uint32_t)((0xFFFFFFFFu / (bar * BARS[clamp(song.g[G_TXRATE], 0, 4)])) * n);
    lfo = sine_i(tx.ph - 0x40000000u);                  /* (from its top: a start at full) */
    depth = clamp(song.g[G_TXMOVE], 0, 127) * 258;
    u = mulq15(depth, (32767 - lfo) >> 1);              /* 0 .. depth, the dip */
    mv = 32767 - mulq15(u, 21000);                      /* the level breathing: down to ~36 % */
    k0 = 32767 - mulq15(u, 28000);                      /* the brightness: a one-pole closing */
    texture_gen(tx.src, tl, tr, n);
    sc = TX_SCALE[tx.src < TX_USR1 ? tx.src : TX_USR1];
    for (i = 0; i < n; i++) {                           /* each source to its level, MOVE's colour (Q13: no overflow */
        tx.lpm[0] += ((((tl[i] * sc) >> 8) - tx.lpm[0]) >> 2) * (k0 >> 2) >> 11;   /* up to +-260000) */
        tx.lpm[1] += ((((tr[i] * sc) >> 8) - tx.lpm[1]) >> 2) * (k0 >> 2) >> 11;
        tl[i] = tx.lpm[0], tr[i] = tx.lpm[1];
    }
    if (djf_block(&tx.f, clamp(song.g[G_TXTONE], -64, 63), &fc, 40)) {
        djf_run(&tx.f, &fc, tl, n, 0);
        djf_run(&tx.f, &fc, tr, n, 1);
    }
    k1 = mulq15(lvl * lvl * 2, mv);                     /* LEVEL^2 (x 2: Q15 of 4096 at 127 .. ) x MOVE */
    for (i = 0; i < n; i++) {
        int32_t g = g0 + (((g1 - g0) * (int32_t)i) >> CTL_LOG2), gk = mulq15(g, k1);
        if (song.g[G_TXDUCK])
            gk = mulq15(gk, duck.g0 + (((duck.g1 - duck.g0) * (int32_t)i) >> CTL_LOG2));
        l[i] += (tl[i] * (gk >> 4)) >> 11;              /* x LEVEL^2 / 16384: (x * gk / 32768) at full */
        r[i] += (tr[i] * (gk >> 4)) >> 11;
    }
    if (!g1 && src != tx.src)
        texture_reset(src);                             /* faded out: the new source from rest */
}

static void mix_block(int32_t *out, uint32_t n)
{
    uint32_t i, c1;
    int32_t m0, m1, *mix_l = acc[0].l, *mix_r = acc[0].r;
    for (i = 0; i < n; i++)
        acc[0].l[i] = acc[0].r[i] = acc[0].sc[i] = acc[0].sd[i] = acc[0].sr[i] = 0;
    {   /* REV / ECHO THROW (2.6): the sends glide to full and back over ~12 ms (17 blocks); set before the hand-off:
         * the units on both cores read them (send_thr) */
        uint32_t q = thr_req | thr_ed;                  /* (2.7: the editor's LIVE page too) */
        thr_rev = (q & 1u) ? (thr_rev + 1928 > 32767 ? 32767 : thr_rev + 1928) : (thr_rev > 1928 ? thr_rev - 1928 : 0);
        thr_dly = (q & 2u) ? (thr_dly + 1928 > 32767 ? 32767 : thr_dly + 1928) : (thr_dly > 1928 ? thr_dly - 1928 : 0);
    }
    events_block(n);
    duck_block(n * (uint32_t)song.g[G_BPM]);
    c1 = cpu1_on() ? units_split() : 0;                 /* core 1's units this block */
    if (c1) {
        for (i = 0; i < n; i++)
            acc[1].l[i] = acc[1].r[i] = acc[1].sc[i] = acc[1].sd[i] = acc[1].sr[i] = 0;
        if (!cpu1_run(cpu1_units_job, c1 | n << 8))
            c1 = 0;
    }
    mix_units(U_ALL & ~c1, n, &acc[0], part_buf[0]);    /* this core's units, meanwhile */
    if (c1) {
        uint32_t w0 = cpu1_ticks();
        int late = cpu1_wait();
        cpu1_wait_ticks += cpu1_ticks() - w0;
        if (late) {                                     /* core 1 did not finish in time: held from now on (cpu1.h);
                                                         * its units here, once (what it half did is this block's glitch) */
            for (i = 0; i < n; i++)
                acc[1].l[i] = acc[1].r[i] = acc[1].sc[i] = acc[1].sd[i] = acc[1].sr[i] = 0;
            cpu1_units_job(c1 | n << 8);
        }
        for (i = 0; i < n; i++) {
            acc[0].l[i] += acc[1].l[i];
            acc[0].r[i] += acc[1].r[i];
            acc[0].sc[i] += acc[1].sc[i];
            acc[0].sd[i] += acc[1].sd[i];
            acc[0].sr[i] += acc[1].sr[i];
        }
    }
    texture_mix(mix_l, mix_r, n);                       /* GLO > TEXTURE (2.6): the bed, dry; after both cores' units
                                                         * (what it adds does not depend on them: the same sum) */
    amb_block(mix_l, mix_r, acc[0].sr, n);              /* RAPTOR: Paleotone's ambience, with the tracks */
    punch_freeze_feed(acc[0].sr, mix_l, mix_r, n);     /* punch-in FREEZE: the whole dry mix into the held reverb */
    fx_buses(acc[0].sc, acc[0].sd, acc[0].sr, wet_l, wet_r, n);
    for (i = 0; i < n; i++) {
        mix_l[i] += wet_l[i];
        mix_r[i] += wet_r[i];
    }
    tape_block(mix_l, mix_r, n);                        /* RAPTOR: Paleotone's TAPE / CRSH, before MASTER */
    dust_process(mix_l, mix_r, n);
    punch_process(mix_l, mix_r, n);
    djf_process(mix_l, mix_r, n);
    m1 = (int32_t)song.master_q12;
    m0 = master_cur < 0 ? m1 : master_cur;
    master_cur = m1;
    if (n > CTL)
        n = CTL;                                        /* (the buffers' size) */
    {
        static int32_t ol[CTL], orr[CTL], ul[CTL], ur[CTL];
        static uint8_t usb_was;
        for (i = 0; i < n; i++) {
            int32_t m = m0 + (((m1 - m0) * (int32_t)i) >> CTL_LOG2);
            ol[i] = MULQ12R(mix_l[i], m, 2);           /* (#4: rounded; (x >> 2) * m >> 10 floored to 4 LSB) */
            orr[i] = MULQ12R(mix_r[i], m, 2);
            master_pre(&ol[i], &orr[i]);
            if ((i & 1u)) {                             /* the visualiser: every other sample, as audio.c reads it */
                vis_tap[2u * i] = (int16_t)knee(mix_l[i]);
                vis_tap[2u * i + 1u] = (int16_t)knee(mix_r[i]);
            }
            if (usb_full_now) {                         /* USB AUDIO = FULL: MASTER all the way up (4096) */
                ul[i] = mix_l[i], ur[i] = mix_r[i];     /* (#4: x 4096 / 4096, every bit; it was floored to 4 LSB) */
                master_pre_usb(&ul[i], &ur[i]);
            }
        }
        lim_block(&lim_dac, ol, orr, n);
        for (i = 0; i < n; i++) {
            out[2u * i] = knee(ol[i]);
            out[2u * i + 1u] = knee(orr[i]);
        }
        if (usb_full_now && !usb_was) {                 /* (a new recording: no old block in its delay) */
            memset(&lim_usb, 0, sizeof lim_usb);
            lim_usb.env = LIM_TH, lim_usb.g = 32768, lim_usb.hold = 0;
        }
        usb_was = usb_full_now;
        if (usb_full_now) {
            lim_block(&lim_usb, ul, ur, n);
            for (i = 0; i < n; i++) {
                usb_out[2u * i] = knee(ul[i]);
                usb_out[2u * i + 1u] = knee(ur[i]);
            }
        }
    }
}
