/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX SCAPE: a generative soundscape beside the four tracks (docs/NOTES.md "The soundscape maker"), with no track
 * and no voice, as the ambience (ambience.c). Eight layers, all procedural (noise through filters, sine chirps and
 * pulses, random timing), each with its LEVEL and DENSITY; a biome sets them all. Globals: DIST (near .. far: darker,
 * wetter), TIME (day .. night: birds and cicadas by day, crickets and frogs by night).
 *   WIND    two band-passed noises, centres and gusts on slow random walks (quarter rate)
 *   WATER   a stream: band-passed noise whose level burbles (a fast random walk), and bubbles (rising blips)
 *   RAIN    rain on leaves: bright noise, its level on a slow walk, and drops (short ticks)
 *   INSECTS crickets (night: a sine chirped by a pulse train, 3..5 pulses) or cicadas (day: a long fast buzz)
 *   FROGS   croaks: two harmonics of a low sine under a ~20..35 Hz gate, in ones, twos and threes
 *   BIRDS   calls: 2..6 chirps, each a fast sine sweep
 *   DRIPS   cave drips: a plink rising in pitch, sometimes its echo
 *   RUMBLE  far thunder / the earth: low noise swelling in and dying away, rare
 * Event layers share SC_NEV slots (the quietest given up first). Levels are squared (the first half of a knob quiet). */
#define SC_NL 8
enum { SC_WIND, SC_WATER, SC_RAIN, SC_INSECT, SC_FROG, SC_BIRD, SC_DRIP, SC_RUMBLE };
#define SC_NEV 6u
#define SC_DIV 4u                                       /* the noise beds' rate: FS / 4 */

/* the settings: the globals of HOME > SCAPE (core.h G_SCAPE .., params.c), stored with the project (project.c) */
#define SC_LEVEL(k) ((uint32_t)clamp(song.g[G_SL0 + (k)], 0, 127))
#define SC_DENS(k) ((uint32_t)clamp(song.g[G_SD0 + (k)], 0, 127))
#define SC_TIME ((uint32_t)clamp(song.g[G_STIME], 0, 127))
#define SC_DIST ((int32_t)clamp(song.g[G_SDIST], 0, 127))
#define SC_MASTER ((uint32_t)clamp(song.g[G_SCAPE], 0, 127))

typedef struct {
    uint32_t ph, inc;                                   /* the sine */
    int32_t dinc;                                       /* its glide a sample (x 1/16) */
    int32_t env, dec;                                   /* Q24 level, Q16 decay a sample */
    uint32_t att, n;                                    /* attack samples left, samples left */
    int32_t ainc;                                       /* the attack's step (Q24) */
    uint32_t gate, gper, gon;                           /* the pulse gate: count, period, on part (0: none) */
    int32_t gl, gr;                                     /* gains Q15 (the layer's level and the pan) */
    uint8_t kind, reps, h2, nzon;                       /* layer, repeats left, with a 2nd harmonic, noisy (cicadas) */
    int32_t nz;                                         /* the noise that roughens a cicada (low-passed) */
    uint16_t gap;                                       /* blocks between repeats */
    uint32_t inc0;
} sc_ev_t;

static struct {
    int32_t rng, on;
    int32_t w1[2][2], wc[2], gust, gust_to;             /* WIND: SVF states, centre walks, the gust */
    int32_t s1[2], wbl, wbl_to;                         /* WATER: SVF state, the burble */
    int32_t rp, rlp, rlvl, rlvl_to;                     /* RAIN: last noise, its low-pass, its level's walk */
    int32_t rum1, rum2, renv, rdec, ratt;               /* RUMBLE: a 2-pole low-pass state, its envelope */
    int32_t bl, br, dl, dr;                             /* the beds' last quarter-rate step, the way to the next */
    int32_t lpl, lpr;                                   /* DIST's low-pass */
    uint32_t wait[SC_NL];                               /* blocks to the layer's next event */
    uint16_t hold[SC_NEV];                              /* a slot's wait before its next repeat (blocks) */
    sc_ev_t ev[SC_NEV];
} sc;

static inline uint32_t sc_rnd(uint32_t m) { return noise32(&sc.rng) % (m ? m : 1u); }
static inline int32_t sc_lvl(uint32_t v) { return (int32_t)(v * v * 32767u / (127u * 127u)); }   /* squared, Q15 */
#define SC_HZ(f) ((uint32_t)((f) * 97391u))             /* 2^32 / FS a Hz */

/* each layer's trim (Q15): at LEVEL 127 all eight sound about as loud (measured while sounding: tests/scape_render.c),
 * the bus then up 12 dB (SC_BUS) */
static const uint16_t SC_TRIM[SC_NL] = {21200, 16600, 14900, 7500, 14400, 19000, 16400, 26000};
#define SC_BUS 2                                        /* the soundscape's bus: x4 */
/* how loud a layer is now: its LEVEL, the trim, the master, and TIME's share (day / night) */
static int32_t sc_gain(uint32_t k)
{
    int32_t g = mulq15(mulq15(sc_lvl(SC_LEVEL(k)), sc_lvl(SC_MASTER)), SC_TRIM[k]), night = (int32_t)SC_TIME * 258;
    if (k == SC_BIRD)
        g = mulq15(g, 32767 - mulq15(night, night));    /* birds fall quiet towards night */
    else if (k == SC_FROG)
        g = mulq15(g, 6000 + mulq15(night, 26767));     /* frogs mostly at night */
    return g;
}

/* a new event of layer k in a free slot (or the quietest), panned at random */
static void sc_spawn(uint32_t k)
{
    uint32_t i, s = 0, night = SC_TIME;
    int32_t g = sc_gain(k), q = 0x7FFFFFFF, pan = (int32_t)sc_rnd(32768u);
    sc_ev_t *e;
    if (g < 40)
        return;
    for (i = 0; i < SC_NEV; i++) {                      /* a free one, else the quietest */
        int32_t lv = sc.ev[i].n || sc.ev[i].reps ? (sc.ev[i].env >> 9) * ((sc.ev[i].gl + sc.ev[i].gr) >> 6) : -1;
        if (lv < q) {
            q = lv;
            s = i;
        }
    }
    e = &sc.ev[s];
    memset(e, 0, sizeof *e);
    sc.hold[s] = 0;
    e->kind = (uint8_t)k;
    e->gl = mulq15(g, 32767 - pan);
    e->gr = mulq15(g, pan);
    e->env = 1 << 24;
    switch (k) {
    case SC_WATER:                                      /* a bubble: 300..900 Hz, up fast, 15..35 ms */
        e->inc = SC_HZ(300u + sc_rnd(600u));
        e->dinc = (int32_t)(e->inc >> 6);
        e->n = FS / 1000u * (15u + sc_rnd(20u));
        e->dec = 65536 - 65536 * 4 / (int32_t)e->n;
        e->gl >>= 1;
        e->gr >>= 1;
        break;
    case SC_INSECT:
        if (night >= 64) {                              /* a cricket: 3.5..5 kHz, 3..5 pulses at ~30 a s, again */
            e->inc = SC_HZ(3500u + sc_rnd(1500u));
            e->gper = FS / (26u + sc_rnd(10u));
            e->gon = e->gper / 2u;
            e->n = e->gper * (3u + sc_rnd(3u));
            e->reps = (uint8_t)(3u + sc_rnd(8u));
            e->gap = (uint16_t)(FS / CTL * (250u + sc_rnd(250u)) / 1000u);
            e->dec = 65536;
        } else {                                        /* a cicada: 4..6 kHz buzzed at 120..200 a s, 1..3 s */
            e->inc = SC_HZ(4000u + sc_rnd(2000u));
            e->gper = FS / (120u + sc_rnd(80u));
            e->gon = e->gper / 3u;
            e->n = FS * (1u + sc_rnd(3u));
            e->att = FS / 3u;
            e->dec = 65536 - 65536 * 3 / (int32_t)e->n;
            e->nzon = 1;                                /* a hissing band round the pitch, not a whistle */
        }
        e->gl >>= 1;
        e->gr >>= 1;
        break;
    case SC_FROG:                                       /* a croak: 180..450 Hz + its octave, gated ~20..35 Hz */
        e->inc = SC_HZ(180u + sc_rnd(270u));
        e->h2 = 1;
        e->gper = FS / (20u + sc_rnd(15u));
        e->gon = e->gper * 2u / 5u;
        e->n = FS / 1000u * (120u + sc_rnd(250u));
        e->dec = 65536 - 65536 * 2 / (int32_t)e->n;
        e->reps = (uint8_t)sc_rnd(3u);
        e->gap = (uint16_t)(FS / CTL * (90u + sc_rnd(120u)) / 1000u);
        break;
    case SC_BIRD:                                       /* a call: chirps 2..5 kHz sweeping, 40..110 ms */
        e->inc = SC_HZ(2000u + sc_rnd(3000u));
        e->dinc = (int32_t)(sc_rnd(2u) ? e->inc >> 9 : 0u - (e->inc >> 8));
        e->n = FS / 1000u * (40u + sc_rnd(70u));
        e->att = FS / 200u;
        e->dec = 65536 - 65536 * 3 / (int32_t)e->n;
        e->reps = (uint8_t)(1u + sc_rnd(5u));
        e->gap = (uint16_t)(FS / CTL * (60u + sc_rnd(110u)) / 1000u);
        break;
    case SC_DRIP:                                       /* a plink: 800..2200 Hz rising, 40..90 ms, now and then an echo */
        e->inc = SC_HZ(800u + sc_rnd(1400u));
        e->dinc = (int32_t)(e->inc >> 7);
        e->n = FS / 1000u * (70u + sc_rnd(80u));
        e->dec = 65536 - 65536 * 3 / (int32_t)e->n;
        e->reps = (uint8_t)(sc_rnd(3u) == 0u);
        e->gap = (uint16_t)(FS / CTL * (110u + sc_rnd(80u)) / 1000u);
        break;
    }
    if (e->att) {                                       /* from silence, a straight ramp */
        e->env = 0;
        e->ainc = (1 << 24) / (int32_t)e->att;
    }
    e->inc0 = e->inc;
}

/* the mean wait between a layer's events, in blocks, at DENSITY d (127: the busiest) */
static uint32_t sc_wait(uint32_t k, uint32_t d)
{
    static const uint16_t MS[SC_NL] = {0, 120, 0, 900, 2500, 2500, 1500, 12000};   /* at DENSITY 127 */
    uint32_t ms;
    d = d < 8u ? 8u : d;
    ms = MS[k] * 127u / d;                              /* (half the DENSITY: twice the wait) */
    return ms * (FS / CTL) / 1000u + 1u;
}

/* the events, a sample */
static inline int32_t sc_ev_run(sc_ev_t *e, int32_t *r)
{
    int32_t s, a;
    if (!e->n)
        return *r = 0;
    s = sine_i(e->ph);
    if (e->h2)
        s = (s + (sine_i(e->ph << 1) >> 1)) * 2 / 3;
    if (e->nzon) {                                      /* ~1.2 kHz of noise ring-modulating the sine */
        e->nz += (((int32_t)(noise32(&sc.rng) >> 17) - 16384) - e->nz) >> 2;
        s = clamp(mulq15(s, e->nz) << 3, -32767, 32767);
    }
    e->ph += e->inc;
    e->inc += (uint32_t)(e->dinc >> 4);
    if (e->gper) {
        if (++e->gate >= e->gper)
            e->gate = 0;
        if (e->gate >= e->gon)
            s = 0;
    }
    if (e->att) {
        e->att--;
        e->env += e->ainc;
    } else
        e->env = mulq16(e->env, (uint32_t)e->dec);
    a = e->env >> 9;                                    /* Q24 -> Q15 */
    s = mulq15(s, a);
    e->n--;
    *r = mulq15(s, e->gr);
    return mulq15(s, e->gl);
}

static void scape_block(int32_t *l, int32_t *r, int32_t *rev, uint32_t n)
{
    int32_t gw = sc_gain(SC_WIND), gs = sc_gain(SC_WATER), gr = sc_gain(SC_RAIN), gm = sc_gain(SC_RUMBLE);
    int32_t dist = SC_DIST, lpk = 32767 - dist * 220, wet = 2048 + dist * 48;
    tsvf_t fw[2], fs;
    uint32_t i, k;
    if (!SC_MASTER) {
        sc.on = 0;
        return;
    }
    if (!sc.on) {
        memset(&sc, 0, sizeof sc);
        sc.rng = 0x5CA9E001;
        sc.gust = sc.gust_to = 24576;
        sc.wbl = sc.wbl_to = 16384;
        sc.rlvl = sc.rlvl_to = 16384;
        for (k = 0; k < SC_NL; k++)
            sc.wait[k] = sc_rnd(sc_wait(k, SC_DENS(k)) + 1u);
        sc.on = 1;
    }
    /* once a block: the walks, the events due */
    for (k = 0; k < 2u; k++)
        sc.wc[k] = clamp(sc.wc[k] + (int32_t)(noise32(&sc.rng) >> 25) - 64 - (sc.wc[k] >> 11), -6 << 8, 6 << 8);
    if (sc.gust - sc.gust_to < 256 && sc.gust_to - sc.gust < 256)
        sc.gust_to = 32767 - (int32_t)sc_rnd(1u + SC_DENS(SC_WIND) * 193u);   /* gusts: WIND's DENSITY, 0 steady ..
                                                                          * 127 -12 dB .. 0 */
    sc.gust += (sc.gust_to - sc.gust) >> (10 - (SC_DENS(SC_WIND) >> 5));   /* (and quicker) */
    if (sc.wbl - sc.wbl_to < 512 && sc.wbl_to - sc.wbl < 512)
        sc.wbl_to = 6000 + (int32_t)sc_rnd(26000u);                        /* the stream's burble */
    sc.wbl += (sc.wbl_to - sc.wbl) >> 3;
    if (sc.rlvl - sc.rlvl_to < 256 && sc.rlvl_to - sc.rlvl < 256)
        sc.rlvl_to = 8192 + (int32_t)sc_rnd(24576u);
    sc.rlvl += (sc.rlvl_to - sc.rlvl) >> 9;
    tsvf_coef(&fw[0], ((49 + 28) << 8) + sc.wc[0], 60);                    /* ~350 Hz, ~400 Hz (x4 rate) */
    tsvf_coef(&fw[1], ((52 + 28) << 8) + sc.wc[1], 60);
    tsvf_coef(&fs, ((66 + 28) << 8), 30);                                  /* the stream ~800 Hz, broad */
    for (k = 0; k < SC_NL; k++) {
        if (k == SC_WIND || k == SC_RAIN || !SC_DENS(k))
            continue;
        if (sc.wait[k])
            sc.wait[k]--;
        else {
            uint32_t m = sc_wait(k, SC_DENS(k));
            sc.wait[k] = m / 2u + sc_rnd(m);
            if (k == SC_RUMBLE) {
                if (gm > 40 && !sc.renv) {
                    sc.ratt = (int32_t)(FS / SC_DIV * (1u + sc_rnd(2u)) / 2u);
                    sc.renv = 1;
                    sc.rdec = 65536 - 65536 * 4 / (int32_t)(FS / SC_DIV * (3u + sc_rnd(4u)));
                }
            } else
                sc_spawn(k);
        }
    }
    for (k = 0; k < SC_NEV; k++)                                           /* repeats due */
        if (!sc.ev[k].n && sc.ev[k].reps) {
            if (sc.hold[k] < sc.ev[k].gap)
                sc.hold[k]++;
            else {
                sc_ev_t *e = &sc.ev[k];
                uint32_t len = e->kind == SC_INSECT && e->gper && SC_TIME >= 64 ? e->gper * (3u + sc_rnd(3u))
                             : e->kind == SC_FROG ? FS / 1000u * (120u + sc_rnd(150u))
                             : FS / 1000u * (40u + sc_rnd(70u));
                e->reps--;
                sc.hold[k] = 0;
                e->n = e->kind == SC_DRIP ? FS / 1000u * 60u : len;
                e->env = e->kind == SC_DRIP ? 1 << 22 : 1 << 24;           /* (a drip's echo: quieter) */
                e->ph = 0;
                e->gate = 0;
                e->inc = e->kind == SC_BIRD ? e->inc0 + (uint32_t)((int32_t)sc_rnd(e->inc0 >> 3) - (int32_t)(e->inc0 >> 4))
                                            : e->inc0;
                if (e->kind == SC_BIRD || e->kind == SC_INSECT) {
                    e->att = FS / 200u;
                    e->env = 0;
                    e->ainc = (1 << 24) / (int32_t)e->att;
                }
            }
        }
    for (i = 0; i < n; i++) {
        int32_t al = 0, ar = 0, el, er, t;
        if (!(i % SC_DIV)) {                            /* the beds' next quarter-rate step */
            int32_t bl = 0, br = 0, x;
            if (gw > 40) {
                int32_t g = mulq15(sc.gust, gw);
                bl += mulq15(gurdy_bp(&fw[0], (int32_t)(noise32(&sc.rng) >> 17) - 16384, &sc.w1[0][0], &sc.w1[0][1]), g);
                br += mulq15(gurdy_bp(&fw[1], (int32_t)(noise32(&sc.rng) >> 17) - 16384, &sc.w1[1][0], &sc.w1[1][1]), g);
            }
            if (gs > 40) {
                x = mulq15(gurdy_bp(&fs, (int32_t)(noise32(&sc.rng) >> 17) - 16384, &sc.s1[0], &sc.s1[1]),
                           mulq15(sc.wbl, gs));
                bl += x;
                br += mulq15(x, 23000);
            }
            if (sc.renv) {                              /* RUMBLE: swells in, dies away */
                int32_t y;
                x = (int32_t)(noise32(&sc.rng) >> 17) - 16384;
                sc.rum1 += (x - sc.rum1) >> 5;          /* ~90 Hz (x4 rate), twice */
                sc.rum2 += (sc.rum1 - sc.rum2) >> 5;
                if (sc.ratt) {
                    sc.ratt--;
                    sc.renv = sc.renv < (1 << 24) ? sc.renv + (1 << 24) / (FS / SC_DIV) : sc.renv;
                } else
                    sc.renv = sc.renv > 4096 ? mulq16(sc.renv, (uint32_t)sc.rdec) : 0;
                y = mulq15(sc.rum2 * 6, mulq15(sc.renv >> 9, gm));
                bl += y;
                br += y;
            }
            sc.dl = (bl - sc.bl) / (int32_t)SC_DIV;
            sc.dr = (br - sc.br) / (int32_t)SC_DIV;
        }
        sc.bl += sc.dl;
        sc.br += sc.dr;
        al = sc.bl;
        ar = sc.br;
        if (gr > 40) {                                  /* RAIN: bright noise (a difference, softened) and ticks */
            int32_t x = (int32_t)(noise32(&sc.rng) >> 17) - 16384, y = x - sc.rp;
            sc.rp = x;
            sc.rlp += (y - sc.rlp) >> 1;
            t = mulq15(mulq15(sc.rlp, sc.rlvl), gr) >> 1;
            if (!(noise32(&sc.rng) >> (19 + (SC_DENS(SC_RAIN) >> 5))))   /* drops: ~5 .. ~40 a s */
                t += mulq15((int32_t)(noise32(&sc.rng) >> 17), gr) >> 1;
            al += t;
            ar += t;
        }
        for (k = 0; k < SC_NEV; k++) {
            el = sc_ev_run(&sc.ev[k], &er);
            al += el;
            ar += er;
        }
        sc.lpl += mulq15(al - sc.lpl, lpk);             /* DIST: darker */
        sc.lpr += mulq15(ar - sc.lpr, lpk);
        l[i] += sc.lpl << SC_BUS;
        r[i] += sc.lpr << SC_BUS;
        rev[i] += mulq15(sc.lpl + sc.lpr, wet) << SC_BUS;   /* and wetter: further into the reverb */
    }
}

/* the biomes (the 2026-10-23 previews' eight; quiet and sparse by default, the wind well under the rest) */
#define SC_NBIOME 8u
static const struct { const char *name; uint8_t level[SC_NL], density[SC_NL], dist, time; } SC_BIOME[SC_NBIOME] = {
    /*                WIND WATR RAIN INSC FROG BIRD DRIP RUMB */
    {"WATER",     {35, 90,  0, 40, 70, 60,  0,  0}, {50, 60,  0, 30, 40, 35,  0,  0}, 30, 40},
    {"JUNGLE",    {25, 40, 50, 85, 70, 75, 30, 50}, {40, 30, 40, 70, 50, 55, 20, 30}, 40, 70},
    {"TUNDRA",    {70,  0,  0,  0,  0, 30,  0, 55}, {90,  0,  0,  0,  0, 15,  0, 25}, 60, 30},
    {"FOREST",    {40, 25,  0, 35, 30, 85,  0,  0}, {50, 20,  0, 25, 20, 60,  0,  0}, 30, 25},
    {"PLAINS",    {55,  0,  0, 75,  0, 50,  0, 40}, {64,  0,  0, 60,  0, 30,  0, 20}, 50, 30},
    {"MOUNTAINS", {65, 30,  0,  0,  0, 40,  0, 50}, {90, 20,  0,  0,  0, 20,  0, 25}, 70, 35},
    {"CANYONS",   {55,  0,  0,  0,  0, 35, 40, 45}, {70,  0,  0,  0,  0, 15, 20, 20}, 80, 40},
    {"CALDERA",   {40, 50,  0,  0,  0,  0,  0, 85}, {50, 70,  0,  0,  0,  0,  0, 60}, 45, 60},
};
/* biome b into the settings (its BIOME, the layers' levels and densities, DIST, TIME; not the LEVEL): a BIOME turned on
 * the SCAPE page (ui_input.c) */
static void scape_biome(uint32_t b)
{
    uint32_t k;
    b %= SC_NBIOME;
    song.g[G_SBIOME] = (int16_t)b;
    for (k = 0; k < SC_NL; k++) {
        song.g[G_SL0 + k] = SC_BIOME[b].level[k];
        song.g[G_SD0 + k] = SC_BIOME[b].density[k];
    }
    song.g[G_SDIST] = SC_BIOME[b].dist;
    song.g[G_STIME] = SC_BIOME[b].time;
}
