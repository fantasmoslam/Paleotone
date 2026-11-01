/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 GHOULBOX (on Felucca by Leo Kuroshita, Hügelton Instruments) */
/* GURDY (GHOULBOX): a hurdy-gurdy. One wheel bows every string at once, so it is monophonic (poly 1) and its
 * drones sound under every note, whatever key is played:
 *   the chanterelle  the melody: a band-limited saw (a bowed string's Helmholtz motion), darker with less
 *                    wheel pressure (WHL), with the rosin's friction noise
 *   the bourdon      a drone on DRN (C2 .. B2), DLVL loud
 *   the mouche       a drone a fifth above it, 5TH loud
 *   the trompette    the bourdon an octave up on the buzzing bridge: its "dog" rattles on the soundboard.
 *                    BUZZ: how hard; COUP: the wrist's strokes that kick it, in time (HOLD: always, or
 *                    1/4 1/8 1/8T 1/16 of the song tempo from the note-on: each stroke a burst that fades)
 *   the crank        COUP CRANK (GHOULBOX): SELECT is the wheel's crank (ui_input.c). Its speed is the bow's
 *                    pressure: the strings' level, brightness and rosin; stopped, they fall silent with the key
 *                    held. A sharp speed-up is a wrist stroke: the trompette's burst. WHL is then GEAR, the
 *                    speed of full pressure
 *   the body         two wooden resonances (~270 Hz box, ~1.1 kHz nose), BODY of them against the dry strings
 * WOBL: the wheel's unevenness, a slow random wander of pitch and loudness.
 * RING (GHOULBOX 2.3, the track's P_RING, its own page): the wheel goes on turning between the notes. While the
 * transport runs, a note's end leaves the voice at its sustain (voice.c voice_release): the melody string and the
 * trompette fade over the release (v->ring), the drones sound on until the next note, the stop, or RING OFF.
 * State: s[0..1] the box's SVF, s[2..3] the nose's, s[4] noise, s[5] the wander, s[6] samples since the
 * note-on (the strokes; COUP CRANK: the pressure, the stroke's envelope and the strokes it has seen), s[7] the
 * pressure low-pass. ph[0] chanterelle, ph[1] bourdon (x2: the trompette),
 * ph[2] mouche. */
static const char *const N_GURDY_DRN[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const N_GURDY_COUP[] = {"HOLD", "1/4", "1/8", "1/8T", "1/16", "TURN", "DRM4", "DRM8", "GALOP", "TRES",
                                           "WAR", "RAIN", "QUAKE", "WAR2", "TALK", "SEVEN", "PENTA", "SPIRL", "CHAOS"};   /* (TURN: GHOULBOX's CRANK)    append-only */
static const uint8_t GURDY_COUP_DIV[19] = {0, 0, 1, 4, 2, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};   /* N_DIV indices (fx.c div_samples) */
#define GURDY_DRUM 6u                                   /* DINOBOX: DRM4 DRM8, the strokes with a deep drum on each */
#define GURDY_KICK (FS / 25u)                          /* DINOBOX: the drum's pitch falls to its note in 40 ms */
#define GURDY_RQ ((1u << 24) / (2u * GURDY_KICK))      /* (its fall's 1 / (2 x KICK), Q24) */
#define GURDY_RHYTHM 8u                                 /* DINOBOX: GALOP .. CHAOS, the drum on 1/16s */
/* DINOBOX: the rhythms, on sixteenths from the note-on: per step 0 rest, 1 soft, 2 loud (the low drum), 3 a high drum
 * (a fifth up), 4 a talking drum (bending up into its note). Every fourth bar ends in a fill; WOBL adds ghost notes
 * on the rests and moves each hit's pitch a little (gurdy_hit) */
static const struct { uint8_t len, v[32]; } GURDY_RHY[] = {
    {16, {2, 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1}},                   /* GALOP: a herd running, da-da-DUM */
    {16, {2, 0, 0, 1, 0, 0, 1, 0, 2, 0, 0, 1, 0, 0, 1, 0}},                   /* TRES: 3-3-2 */
    {16, {2, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 0, 1, 1}},                   /* WAR: a march, a boom on the halves */
    {16, {2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1}},                   /* RAIN: hands never stopping */
    {16, {2, 0, 1, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0}},                   /* QUAKE: a slow heartbeat */
    {32, {2, 0, 0, 2, 0, 0, 3, 0, 2, 0, 1, 1, 3, 0, 0, 0,                     /* WAR2: war drums, two bars, low and */
          2, 0, 0, 2, 0, 0, 3, 0, 2, 0, 3, 0, 2, 3, 2, 3}},                   /* high answering */
    {16, {4, 0, 1, 4, 0, 3, 0, 1, 4, 0, 0, 4, 1, 0, 3, 1}},                   /* TALK: talking drums */
    {14, {2, 0, 1, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0}},                         /* SEVEN: 7/8, 2+2+3 */
    {20, {2, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0}},       /* PENTA: five hits over four beats .. */
    {32, {2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 3,   /* SPIRL: hits closing in, a roll into */
          0, 1, 0, 3, 1, 3, 1, 3, 4, 4}},                                     /* the downbeat */
    {16, {2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0}},                   /* CHAOS: the downbeats, the rest by chance */
};
#define GURDY_CHAOS 18u
static uint32_t gurdy_hash(uint32_t x)
{
    x = (x + 0x9E3779B9u) * 0x85EBCA6Bu;
    x ^= x >> 13;
    x *= 0xC2B2AE35u;
    return x ^ (x >> 16);
}
/* rhythm r's hit on step st (from the note-on): 0 none, else its kind (1 .. 4) */
static uint32_t gurdy_hit(uint32_t r, uint32_t st, int32_t wobl)
{
    uint32_t len = GURDY_RHY[r].len, pos = st % len, v = GURDY_RHY[r].v[pos], h = gurdy_hash(st);
    if (r + GURDY_RHYTHM == GURDY_CHAOS && !v && (h >> 28) < 7u)
        v = (h >> 28) < 3u ? 1u : (h >> 28) < 5u ? 3u : (h >> 28) < 6u ? 4u : 2u;
    if (((st / len) & 3u) == 3u && pos + 4u >= len)     /* the fill: the bar's last four steps a roll */
        v = pos + 1u == len ? 2u : (pos & 1u) ? 3u : 1u;
    if (!v && (int32_t)(h >> 25) < wobl / 3)            /* a ghost note */
        v = 1;
    return v;
}
/* the drum on step st: the hit sounding then (its kind, 0 none), how many steps back it was struck, its pitch */
static uint32_t gurdy_drum_at(uint32_t coup, uint32_t st, int32_t wobl, uint32_t dinc, uint32_t *rk, uint32_t *inc)
{
    uint32_t lv = 0, k;
    if (coup < GURDY_RHYTHM) {                          /* DRM4 DRM8: every stroke, every other one softer */
        *rk = 0;
        *inc = dinc;
        return (st & 1u) ? 1u : 2u;
    }
    for (k = 0; k < 8u && k <= st && !(lv = gurdy_hit(coup - GURDY_RHYTHM, st - k, wobl)); k++)
        ;
    *rk = k;
    *inc = dinc + (uint32_t)((int32_t)(dinc >> 9) * ((((int32_t)(gurdy_hash(st - k) >> 26) - 32) * wobl) >> 7));
    if (lv == 3u)
        *inc += *inc >> 1;                              /* the high drum: a fifth up */
    return lv;
}
#define GURDY_CRANK 5u                                  /* COUP CRANK (GHOULBOX) */
static uint32_t div_samples(uint32_t div);              /* fx.c */

/* The crank (GHOULBOX: COUP CRANK). ui_input.c hands it SELECT's clicks with the input scan's frame of the last one
 * (hal/fm1_input.h enc_dfr, ~909 frames/s): speed = clicks over the time since the click before; with none, it can
 * be no more than one click over the time since the last, and nothing after CRANK_STOP. A wrist stroke: a speed
 * 1.7 x the recent one (smoothed, ~140 ms) on a wheel already turning, at most one per CRANK_GAP. One crank: every
 * CRANK track's wheel follows it (gurdy_render). Written by the UI, read by the audio: word-sized values */
#define CRANK_FPS 909u                                  /* input frames per second (10 kHz / 11 columns) */
#define CRANK_STOP (CRANK_FPS * 3u / 10u)               /* 300 ms without a click: stopped */
#define CRANK_GAP (CRANK_FPS * 7u / 100u)               /* 70 ms between strokes */
#define CRANK_TURNING (6u * 16u)                        /* strokes only on a wheel turning at 6 clicks/s or more */
static volatile struct {
    uint32_t speed;                                     /* clicks per second x 16, 0 = stopped */
    uint8_t strokes;                                    /* wrist strokes so far (wraps): a change is a new one */
} crank;
static struct { uint32_t last, avg, stroke; } crank_m;  /* the last click's frame, the recent speed, the last stroke's */

static void crank_clicks(uint32_t n, uint32_t fr)      /* n clicks (either way), the last at frame fr */
{
    uint32_t dt = fr - crank_m.last, v = dt > CRANK_STOP ? 0u : n * CRANK_FPS * 16u / (dt ? dt : 1u);
    crank_m.last = fr;                                  /* (from a stop: no speed yet, the next click has one) */
    if (!crank_m.avg) {                                 /* turning again: the recent speed is this one (no stroke) */
        crank_m.avg = v;
    } else {
        if (v * 10u > crank_m.avg * 17u && crank_m.avg >= CRANK_TURNING && fr - crank_m.stroke >= CRANK_GAP) {
            crank.strokes++;
            crank_m.stroke = fr;
        }
        crank_m.avg = v ? (uint32_t)((int32_t)crank_m.avg + ((int32_t)v - (int32_t)crank_m.avg) * (int32_t)(dt < 128u ? dt : 128u) / 128) : 0u;
    }
    crank.speed = v;
}

static void crank_idle(uint32_t now)                   /* no click since the last: slowing, then stopped */
{
    uint32_t e = now - crank_m.last;
    if (e > CRANK_STOP) {
        crank.speed = 0;
        crank_m.avg = 0;
    } else if (e && crank.speed > CRANK_FPS * 16u / e) {
        crank.speed = CRANK_FPS * 16u / e;
    }
}

/* a track that cranks: GURDY at COUP CRANK (ui_input.c: SELECT is the crank while there is one) */
static int crank_track(const track_t *t)
{
    return t->eng_req % NENGINES == ENGI_GURDY && t->p[P_E5] == (int16_t)GURDY_CRANK;
}

/* RING: the drones ring through the rests: a GURDY track with RING ON while the transport runs (voice.c voice_release),
 * not muted (a muted track plays no next note: its drones would ring on, and GLO's mute would not silence them) */
static int gurdy_rings(const track_t *t)
{
    return t->p[P_RING] && !t->p[P_MUTE] && t->engine == ENGI_GURDY && song.playing;
}

/* the presets that ring (bit k: GURDY_PRESETS[k]; ui.c sound loads): HURDY GURDY, DRONE WHEEL, BAGPIPE */
#define GURDY_RING 0x23u

/* the bow's pressure (Q15) for the crank's speed: GEAR (WHL) is the speed of full pressure, 60 clicks/s (0) .. 12
 * (127); below 2 clicks/s none; linear between (a slow crank ~6 dB under a full one at half its speed) */
static int32_t crank_pressure(int32_t gear)
{
    int32_t full = (60 - gear * 48 / 127) * 16 - 32, x = (int32_t)crank.speed - 32;
    if (x <= 0)
        return 0;
    return x >= full ? 32767 : x * 32767 / full;
}

static void gurdy_note_on(track_t *t, voice_t *v)
{
    v->s[6] = 0;                                        /* the strokes start with the note */
    if (!v->env_out || v->ring == 255u)                 /* RING: the melody string plays; on drones still */
        v->ring = 255;                                  /* sounding it comes back over the attack (no click) */
    if (t->p[P_E5] == (int16_t)GURDY_CRANK)             /* the crank: the wheel's pressure now (already turning: */
        v->s[6] = crank_pressure(t->p[P_E3]) | (int32_t)((uint32_t)crank.strokes << 24);   /* at once), no burst */
    if (!v->s[4])
        v->s[4] = 0x5EED1234 + (int32_t)v->age;
}

/* the SVF's band-pass (tsvf_lp's v1): the same state update, the other output */
static inline __attribute__((always_inline)) int32_t gurdy_bp(const tsvf_t *c, int32_t in, int32_t *ic1, int32_t *ic2)
{
    int32_t v3 = in - *ic2;
    int32_t v1 = (c->a1 * *ic1 + c->a2 * v3) >> 13;
    int32_t v2 = *ic2 + ((c->a2 * *ic1 + c->a3 * v3) >> 13);
    *ic1 = clamp(2 * v1 - *ic1, -150000, 150000);
    *ic2 = clamp(2 * v2 - *ic2, -150000, 150000);
    return v1;
}

static void gurdy_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    int32_t drn = 36 + clamp(p[P_E0], 0, 11), dlvl = p[P_E1] * 200, flvl = p[P_E2] * 170, whl = p[P_E3];
    int32_t buzz = p[P_E4] * 258, body = p[P_E6] * 258, wobl = p[P_E7];
    uint32_t coup = (uint32_t)clamp(p[P_E5], 0, 18), i, dper = 0, dlen = 1, ha = 0, lv = 0, di = 0, bnd = ~0u, nlv = 0;
    uint32_t ninc = 0, invd = 0;
    int32_t dd, fd, df = temper(p, (uint32_t)drn, &dd) - v->tfine, ff = temper(p, (uint32_t)drn + 7u, &fd) - v->tfine;
    uint32_t inc = m->inc, dinc = cents_inc(drn * 16 + dd, 0, m->fine + df), finc = cents_inc((drn + 7) * 16 + fd, 0, m->fine + ff);
                                                        /* GHOULBOX: the drones on TEMP's own degrees, not the note's */
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1], ph2 = v->ph[2];
    int32_t b1 = v->s[0], b2 = v->s[1], n1 = v->s[2], n2 = v->s[3], nst = v->s[4], wob = v->s[5], cnt = v->s[6];
    int32_t lp = v->s[7], kp = 6000 + whl * 200, fric = whl * 24, e0, e1, de, e, w, pr = 32767, dpr = 0, g = 32767, dg = 0;
    int rg;
    tsvf_t box, nose;
    /* the wheel's wander: a leaky random walk, once per block; pitch +-~12 ct and loudness +-~8 % at WOBL 127 */
    wob += (int32_t)(noise32(&nst) >> 22) - 512;
    wob -= wob >> 6;
    w = (wob * wobl) >> 10;                             /* +-~32000 at the extremes */
    inc += (uint32_t)((int32_t)(inc >> 12) * (w >> 10));
    /* the trompette's strokes: its envelope at this block's start and end, linear between */
    if (coup == GURDY_CRANK) {                          /* the crank: the pressure (to the speed's, ~15 ms up, ~60 ms
                                                         * down), the strokes' envelope (a burst fading over ~90 ms) */
        uint32_t st = (uint32_t)cnt, ev = (st >> 15) & 0x1FFu, seen = st >> 24, fall = n * 511u / (FS * 90u / 1000u);
        int32_t pa = (int32_t)(st & 0x7FFFu), pb = crank_pressure(whl);
        int32_t d = ((pb - pa) * (int32_t)(n * 32768u / (FS * (pb > pa ? 15u : 60u) / 1000u))) >> 15;
        pb = d ? pa + d : pb;                           /* (the last step below -50 dB: at once) */
        if (seen != crank.strokes) {
            seen = crank.strokes;
            ev = 511u;
        }
        e0 = buzz ? (int32_t)ev << 6 : 0;
        ev -= ev < fall ? ev : fall;
        e1 = buzz ? (int32_t)ev << 6 : 0;
        cnt = (int32_t)((uint32_t)pb | ev << 15 | seen << 24);
        whl = (pa * 127) >> 15;                         /* the pressure is the wheel's: the tone and the rosin */
        kp = 6000 + whl * 200;
        fric = whl * 24;
        pr = pa;
        dpr = (pb - pa) / (int32_t)n;
    } else if (!buzz && coup < GURDY_DRUM)
        e0 = e1 = 0;
    else if (!coup)
        e0 = e1 = 32767;
    else {
        uint32_t per = div_samples(GURDY_COUP_DIV[coup]), a = (uint32_t)cnt % per, fall = per * 2u / 5u + 1u;
        uint32_t kf = (32767u << 8) / fall;             /* each stroke falls over 2/5 of its division */
        e0 = a < fall ? 32767 - (int32_t)((a * kf) >> 8) : 0;
        a += n;
        e1 = a < fall ? 32767 - (int32_t)((a * kf) >> 8) : a >= per ? 32767 : 0;
        if (!buzz)
            e0 = e1 = 0;
        if (coup >= GURDY_DRUM)
            e0 = e1 = 0;                                /* DINOBOX: the drum takes the trompette's place (no chatter) */
        if (coup >= GURDY_RHYTHM) {                     /* DINOBOX: a rhythm: ~0.3 s a hit (QUAKE 0.45) */
            dper = per;
            dlen = coup == 12u ? FS * 45u / 100u : FS * 3u / 10u;
        } else if (coup >= GURDY_DRUM) {                /* DINOBOX: the drum's stroke length (at most ~0.3 s) */
            dper = per;
            dlen = per * 9u / 10u < FS * 3u / 10u ? per * 9u / 10u : FS * 3u / 10u;
        }
    }
    if (!v->gate && v->stage < 3u && !gurdy_rings(t))
        v->stage = 3;                                   /* RING off, or the transport stopped: the drones' release */
    rg = v->ring < 255u || (!v->gate && v->stage < 3u);
    if (rg) {                                           /* RING: the melody string and the trompette fade over the */
        uint32_t r = v->ring, k = (r * ENV_EXP[p[P_REL] & 127]) >> 16;   /* release's curve (Q8: a step a block at */
        int32_t g1;                                     /* least, so ~0.2 s at most), back over the attack's */
        if (!v->gate)
            v->ring = (uint8_t)(r > (k ? k : 1u) ? r - (k ? k : 1u) : 0u);
        else if ((k = ENV_LIN[p[P_ATK] & 127] >> 16) < 255u - r)
            v->ring = (uint8_t)(r + (k ? k : 1u));
        else
            v->ring = 255;
        g = (int32_t)(r << 7 | r >> 1);                 /* (Q15: 255 is 32767) */
        g1 = (int32_t)((uint32_t)v->ring << 7 | (uint32_t)v->ring >> 1);
        dg = (g1 - g) / (int32_t)n;
        e0 = mulq15(e0, g);
        e1 = mulq15(e1, g1);
    }
    de = (e1 - e0) / (int32_t)n;
    if (dper) {                                         /* DINOBOX: the drum this block: the hit sounding, and the
                                                         * next step's if it starts one inside the block (a block is
                                                         * shorter than a step) */
        uint32_t st = (uint32_t)cnt / dper, a = (uint32_t)cnt % dper, rk, rk2;
        lv = gurdy_drum_at(coup, st, wobl, dinc, &rk, &di);
        ha = a + rk * dper;
        nlv = gurdy_drum_at(coup, st + 1u, wobl, dinc, &rk2, &ninc);
        bnd = rk2 ? ~0u : dper - a;
        invd = (1u << 30) / dlen;
    }
    e = e0;
    tsvf_coef(&box, 44 << 8, 70);                      /* ~270 Hz (CUTOFF_HZ: 30 Hz .. 16 kHz over 0..127) */
    tsvf_coef(&nose, 73 << 8, 90);                     /* ~1.1 kHz */
    for (i = 0; i < n; i++) {
        int32_t s, x, r, tr, nz = (int32_t)(noise32(&nst) >> 17) - 16384;
        s = osc_saw(ph0, inc);                          /* the chanterelle, through the pressure low-pass */
        lp += mulq15(s - lp, kp);
        x = lp + mulq15(nz, fric);                      /* .. and the rosin */
        if (rg) {                                       /* RING: fading after its note */
            x = mulq15(x, g);
            g += dg;
        }
        x += mulq15(osc_saw(ph1, dinc), dlvl) + mulq15(osc_saw(ph2, finc), flvl);   /* the drones */
        if (e > 0) {                                    /* the trompette: the dog chatters at each period's start */
            tr = osc_saw(ph1 << 1, dinc << 1);
            tr = clamp(tr * 3, -24000, 24000) + ((ph1 << 1) < 0x30000000u ? nz * 2 : 0);   /* 1.3: the chatter x2 */
            x += mulq15(mulq15(tr, e), buzz);           /* (|tr| < 56768: no overflow at BUZZ 127) */
        }
        e += de;
        ph0 += inc;
        ph1 += dinc;
        ph2 += finc;
        x >>= 1;                                        /* (four sources: the filters' range) */
        r = gurdy_bp(&box, x, &b1, &b2) + (gurdy_bp(&nose, x, &n1, &n2) >> 1);
        x += mulq15(r - x, body);
        if (dper) {                                     /* DINOBOX DRM4 .. CHAOS: a deep skin drum at the drone's
                                                         * pitch, kicked from 2.5x and falling to it in ~40 ms (talking:
                                                         * bending up into it over ~150 ms), the phase in closed form
                                                         * from the hit (no state), a slap of noise at the start */
            if (i == bnd) {
                ha = 0;
                lv = nlv;
                di = ninc;
            }
            x = (x * 5) >> 3;                           /* (the strings under the drum) */
            if (ha < dlen && lv) {
                uint32_t kl = lv == 4u ? GURDY_KICK * 4u : GURDY_KICK, ak = ha < kl ? ha : kl;
                uint32_t kick = ak - ((((ak * ak) >> 8) * (lv == 4u ? GURDY_RQ / 4u : GURDY_RQ)) >> 16);
                int32_t env = 32767 - (int32_t)((ha * invd) >> 15), dr;
                env = mulq15(env, env);
                dr = osc_sine(lv == 4u ? di * ha - (di >> 1) * kick : di * ha + (di + (di >> 1)) * kick);
                dr = mulq15(dr, env) + (ha < FS / 300u ? nz : 0);
                x += mulq15(dr, lv == 1u ? 17000 : lv == 2u ? 30000 : 23000);
            }
            ha++;
        }
        x = soft_knee(x, 20000);
        if (coup == GURDY_CRANK) {                      /* the crank's pressure: the strings the wheel bows, and the */
            x = mulq15(x, pr);                          /* body with them (still: silent, the body's ring too) */
            pr += dpr;
        }
        out[i] += voice_amp(x, m, i) << 1;
    }
    (void)cnt;
    v->ph[0] = ph0;
    v->ph[1] = ph1;
    v->ph[2] = ph2;
    v->s[0] = b1;
    v->s[1] = b2;
    v->s[2] = n1;
    v->s[3] = n2;
    v->s[4] = nst;
    v->s[5] = wob;
    v->s[6] = coup == GURDY_CRANK ? cnt : cnt + (int32_t)n;
    v->s[7] = lp;
}

static const preset_t GURDY_PRESETS[] = {
    /* name, {DRN, DLVL, 5TH, WHL, BUZZ, COUP, BODY, WOBL}, {A D S R}, fenv, mono */
    /* GHOULBOX presets (tools/ghoulbox_presets.py) */
    {"PRIMEVAL", {2, 100, 90, 60, 75, 0, 110, 70}, {25, 64, 127, 60}, 0, 1, FX(12, 15, 0, 100), PAT(15)},
    {"EARTHQUAKE", {2, 127, 110, 30, 50, 0, 127, 110}, {70, 64, 127, 110}, 0, 1, FX(25, 15, 0, 115), PAT(17)},
    {"STAMPEDE", {2, 40, 30, 80, 40, 8, 60, 45}, {2, 64, 127, 25}, 0, 1, FX(25, 0, 0, 75), PAT(21)},
    {"REX GROWL", {2, 60, 0, 100, 127, 3, 100, 90}, {8, 64, 127, 45}, 0, 1, FX(40, 0, 0, 90), PAT(18)},
    {"SWAMP FIDDLE", {2, 10, 20, 85, 20, 0, 90, 80}, {45, 64, 120, 80}, 0, 1, FX(5, 30, 15, 105), PAT(19)},
    {"TRIBE PIPES", {2, 110, 127, 127, 5, 0, 30, 30}, {6, 64, 127, 35}, 0, 1, FX(20, 0, 0, 85), PAT(15)},
    {"WAR CHANT", {2, 100, 60, 90, 90, 2, 100, 55}, {5, 64, 127, 40}, 0, 1, FX(20, 0, 10, 95), PAT(18)},
    {"TUNDRA", {2, 120, 100, 40, 30, 0, 127, 127}, {90, 64, 127, 120}, 0, 1, FX(0, 40, 10, 120), PAT(17)},
    {"WAR PARADE", {2, 70, 60, 70, 30, 10, 80, 45}, {5, 64, 127, 40}, 0, 1, FX(15, 0, 10, 100), PAT(18)},
    {"JUNGLE CAMP", {2, 45, 50, 60, 20, 9, 50, 60}, {10, 64, 127, 50}, 0, 1, FX(0, 10, 15, 95), PAT(17)},
    {"RITUAL FIRE", {2, 55, 70, 50, 10, 11, 70, 70}, {20, 64, 127, 60}, 0, 1, FX(5, 10, 20, 105), PAT(16)},
    {"TITAN MARCH", {2, 90, 40, 70, 60, 6, 100, 30}, {5, 64, 127, 45}, 0, 1, FX(20, 0, 0, 100), PAT(17)},
    {"HEARTBEAT", {2, 30, 20, 40, 0, 12, 40, 60}, {40, 64, 127, 90}, 0, 1, FX(0, 20, 0, 115), PAT(17)},
    {"HERD RUN", {2, 35, 40, 90, 30, 7, 50, 50}, {2, 64, 127, 30}, 0, 1, FX(10, 0, 10, 85), PAT(17)},
    {"TRIBAL WAR", {2, 75, 60, 60, 20, 13, 90, 40}, {5, 64, 127, 45}, 0, 1, FX(20, 0, 10, 105), PAT(17)},
    {"TALK DRUMS", {2, 40, 40, 50, 10, 14, 60, 50}, {10, 64, 127, 50}, 0, 1, FX(0, 10, 15, 95), PAT(17)},
    {"ODD RITUAL", {2, 50, 60, 60, 20, 15, 70, 40}, {15, 64, 127, 60}, 0, 1, FX(5, 10, 20, 105), PAT(17)},
    {"FIVE MOONS", {2, 40, 70, 50, 10, 16, 60, 60}, {30, 64, 127, 80}, 0, 1, FX(0, 30, 25, 115), PAT(17)},
    {"SPIRAL", {2, 45, 50, 60, 30, 17, 60, 50}, {10, 64, 127, 50}, 0, 1, FX(10, 0, 20, 100), PAT(17)},
    {"CHAOS DRUMS", {2, 40, 40, 70, 30, 18, 60, 90}, {5, 64, 127, 40}, 0, 1, FX(15, 0, 15, 95), PAT(17)},
    /* GHOULBOX end */
};

/* COUP CRANK: WHL is GEAR (crank_pressure) */
static const param_desc_t GURDY_D_GEAR = {"GEAR", F_PCT, 0, 127, 80, 0, 0};
static const param_desc_t *gurdy_desc(const track_t *t, uint32_t k)
{
    return k == 3u && t->p[P_E5] == (int16_t)GURDY_CRANK ? &GURDY_D_GEAR : 0;
}

static const engine_t ENG_GURDY = {
    .name = "ROAR",                                     /* DINOBOX (GHOULBOX: GURDY, its hurdy-gurdy) */
    .page_title = {"DRONE", "CALL"},
    .edit = {
        {"DRN", F_ENUM, 0, 11, 2, N_GURDY_DRN, 0},
        {"DLVL", F_PCT, 0, 127, 90, 0, 0},
        {"5TH", F_PCT, 0, 127, 50, 0, 0},
        {"BRTH", F_PCT, 0, 127, 80, 0, 0},         /* (GHOULBOX: WHL, the wheel's pressure) */
        {"RASP", F_PCT, 0, 127, 60, 0, 0},         /* (BUZZ) */
        {"PULS", F_ENUM, 0, 18, 2, N_GURDY_COUP, 0},   /* (COUP: the wheel's strokes) */
        {"BODY", F_PCT, 0, 127, 70, 0, 0},
        {"WOBL", F_PCT, 0, 127, 40, 0, 0},
    },
    .presets = GURDY_PRESETS,
    .npresets = NELEM(GURDY_PRESETS),
    .note_on = gurdy_note_on,
    .render = gurdy_render,
    .desc = gurdy_desc,
    .knob = {P_E4, P_E3, P_E1, P_REL},
    .poly = 1,                                          /* one wheel: one melody note */
    .keep = 0xAF,                                       /* the body, the wander and the pressure carry on */
};
