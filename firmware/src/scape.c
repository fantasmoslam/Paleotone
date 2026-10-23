/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 DINOBOX (on GHOULBOX and Felucca by Leo Kuroshita, Hügelton Instruments) */
/* SCAPE (DINOBOX): the soundscape maker. A generative ambience beside the four tracks, made on the device from noise,
 * sines and saws (no samples: the flash has ~18 KB to spare), into the mix and the reverb before TAPE and MASTER
 * (fx.c mix_block), like GHOULBOX's AMB. Quiet and sparse by default: an ambience under the music, not over it.
 *
 *   a biome        8 layers, each a kind of sound with a LEVEL, a DENSity (how often its events come) and a DISTance
 *                  (near: bright and dry; far: quieter, darker, more of the room); a drone; the room's size
 *   the layers     beds (continuous: wind, stream, waves, rain, cicadas, lava, rumble) and events (birds, frogs,
 *                  crickets, drips, bubbles, rocks, snaps, creaks, steam, distant creatures, a herd passing)
 *   the globals    TIME (DAWN DAY DUSK NIGHT: who sings), WEATHER (CLEAR WIND RAIN STORM), DANGER (how often
 *                  something big comes through, and how big), DENSITY (all events), the SCAPE level
 *   an encounter   far steps, the insects and birds hush, the steps come closer, a growl, a roar, the steps go away
 *   the drone      PAD CHOIR THROAT AIR on the key (KEY), its movement (MOVE)
 *
 * Every event's time, pitch, place and distance is drawn by chance under the biome's rules, so nothing repeats. The
 * creatures' calls are DINOBOX's SAURUS (the hook scape_call: a voice of the shared 12; the scape's give way first);
 * everything else is here. Cost: the beds a filtered noise pair each, the events at most SC_NV small voices, the
 * drone a few oscillators; worked out per block where it can be (envelopes, glides, filters' coefficients). */

#define SC_NL 8u                                        /* a biome's layers */
#define SC_NW 2u                                        /* + the weather's (rain, thunder) */
#define SC_NV 10u                                       /* event voices */
#define SC_BPS (FS / CTL)                               /* blocks a second (1378) */
#define SC_OUT_SH 9                                     /* the scape's own gain: x8 (its sources sit low inside) */

enum { SK_OFF, SK_WIND, SK_STREAM, SK_WAVES, SK_RAIN, SK_CICADA, SK_LAVA, SK_RUMBLE,          /* beds (+ events) */
       SK_SONG, SK_WHISTLE, SK_HOOT, SK_KNOCK, SK_WADER, SK_RAPTOR, SK_CRICKET, SK_FROG, SK_TOAD,
       SK_DRIP, SK_ROCKS, SK_SNAP, SK_CREAK, SK_STEAM, SK_CALLS, SK_HERD, SK_THUNDER, SK_COUNT };
#define SK_BEDS SK_SONG                                 /* kinds below it have a bed */

enum { SD_OFF, SD_PAD, SD_CHOIR, SD_THROAT, SD_AIR };   /* the drone's types */
enum { ST_DAWN, ST_DAY, ST_DUSK, ST_NIGHT };            /* TIME */
enum { SW_CLEAR, SW_WIND, SW_RAIN, SW_STORM };          /* WEATHER */
enum { MV_FREE, MV_CHIRP, MV_BUZZ, MV_BURST, MV_THUMP }; /* an event voice's sound */

/* the SAURUS creatures the scape calls (eng_saurus.c presets, factory order) */
enum { SC_BRACHIO = 0, SC_TYRANT = 1, SC_RAPTOR_BARK = 2, SC_RAPTOR_CHAT = 3, SC_CREST_HOOT = 4, SC_PTERO = 5,
       SC_HERD_MOAN = 6, SC_DEEP_RUMBLE = 9, SC_TITAN_APE = 10, SC_SWAMP_CROAK = 12, SC_MONSTER = 13,
       SC_DISTANT_HERD = 14, SC_LOW_GROWL = 15, SC_SEA_MONSTER = 17, SC_HADRO = 18, SC_COMPY = 19 };
static void scape_call(uint32_t creature, int32_t lvl, int32_t dist, int32_t pan, uint32_t ms);   /* the integration's:
                                                         * lvl Q15 with SCAPE in it (32767: a track's full level) */

typedef struct { uint8_t kind, level, dens, dist; } sc_layer_t;
typedef struct {
    const char *name;
    sc_layer_t l[SC_NL];
    uint8_t drone, dlevel, room, howl;                  /* room: the reverb's share; howl: the wind's whistle */
    uint8_t calls[4];                                   /* SK_CALLS: who lives here */
    uint8_t pred[3];                                    /* the encounters: small, middle, big (by DANGER) */
} sc_biome_t;

/* The biomes. LEVEL, DENS, DIST 0..127 (a level squared, as AMB; DENS 64 the rate below, squared). Quiet by default:
 * the wind 8 dB under the first previews' (owner, 2026-10-23), and sparse */
static const sc_biome_t SC_BIOMES[] = {
    {"WATER", {{SK_STREAM, 70, 64, 40}, {SK_WAVES, 72, 64, 70}, {SK_FROG, 52, 40, 50}, {SK_TOAD, 60, 30, 60},
               {SK_WADER, 50, 40, 70}, {SK_DRIP, 40, 30, 50}, {SK_CALLS, 72, 40, 90}, {SK_WIND, 24, 64, 60}},
     SD_CHOIR, 30, 40, 0, {SC_SEA_MONSTER, SC_SWAMP_CROAK, SC_HADRO, SC_CREST_HOOT}, {SC_RAPTOR_CHAT, SC_LOW_GROWL, SC_SEA_MONSTER}},
    {"JUNGLE", {{SK_CICADA, 46, 64, 50}, {SK_CRICKET, 54, 50, 50}, {SK_FROG, 44, 44, 60}, {SK_SONG, 44, 36, 70},
                {SK_WHISTLE, 46, 30, 80}, {SK_DRIP, 40, 36, 40}, {SK_CALLS, 64, 36, 100}, {SK_HOOT, 46, 30, 80}},
     SD_PAD, 26, 36, 0, {SC_CREST_HOOT, SC_RAPTOR_CHAT, SC_TITAN_APE, SC_COMPY}, {SC_RAPTOR_BARK, SC_LOW_GROWL, SC_TYRANT}},
    {"TUNDRA", {{SK_WIND, 56, 64, 60}, {SK_WIND, 40, 50, 90}, {SK_CREAK, 50, 36, 70}, {SK_HERD, 64, 30, 100},
                {SK_CALLS, 64, 30, 100}, {SK_RAPTOR, 36, 20, 100}, {SK_OFF, 0, 0, 0}, {SK_OFF, 0, 0, 0}},
     SD_AIR, 26, 56, 40, {SC_DISTANT_HERD, SC_HERD_MOAN, SC_DEEP_RUMBLE, SC_DISTANT_HERD}, {SC_HERD_MOAN, SC_LOW_GROWL, SC_MONSTER}},
    {"FOREST", {{SK_WIND, 36, 50, 80}, {SK_SONG, 56, 50, 60}, {SK_KNOCK, 50, 30, 70}, {SK_CRICKET, 40, 36, 60},
                {SK_SNAP, 50, 30, 70}, {SK_CALLS, 64, 36, 90}, {SK_HOOT, 46, 30, 70}, {SK_WHISTLE, 40, 24, 80}},
     SD_CHOIR, 28, 40, 0, {SC_CREST_HOOT, SC_HADRO, SC_HERD_MOAN, SC_CREST_HOOT}, {SC_RAPTOR_BARK, SC_LOW_GROWL, SC_TYRANT}},
    {"PLAINS", {{SK_WIND, 44, 50, 70}, {SK_CICADA, 30, 40, 80}, {SK_CALLS, 66, 34, 110}, {SK_HERD, 70, 30, 110},
                {SK_RAPTOR, 40, 24, 90}, {SK_CRICKET, 38, 30, 70}, {SK_SONG, 36, 24, 80}, {SK_OFF, 0, 0, 0}},
     SD_THROAT, 24, 30, 0, {SC_BRACHIO, SC_HERD_MOAN, SC_DISTANT_HERD, SC_HADRO}, {SC_COMPY, SC_RAPTOR_BARK, SC_TYRANT}},
    {"MOUNTAINS", {{SK_WIND, 54, 64, 50}, {SK_WIND, 38, 50, 90}, {SK_ROCKS, 56, 30, 80}, {SK_RAPTOR, 44, 30, 80},
                   {SK_CALLS, 60, 30, 100}, {SK_OFF, 0, 0, 0}, {SK_OFF, 0, 0, 0}, {SK_OFF, 0, 0, 0}},
     SD_AIR, 26, 64, 50, {SC_PTERO, SC_PTERO, SC_DEEP_RUMBLE, SC_TITAN_APE}, {SC_PTERO, SC_TITAN_APE, SC_MONSTER}},
    {"CANYONS", {{SK_WIND, 50, 60, 60}, {SK_ROCKS, 56, 34, 70}, {SK_RAPTOR, 40, 24, 80}, {SK_CALLS, 64, 34, 90},
                 {SK_SNAP, 36, 20, 80}, {SK_OFF, 0, 0, 0}, {SK_OFF, 0, 0, 0}, {SK_OFF, 0, 0, 0}},
     SD_THROAT, 24, 70, 60, {SC_RAPTOR_BARK, SC_PTERO, SC_MONSTER, SC_RAPTOR_CHAT}, {SC_RAPTOR_BARK, SC_LOW_GROWL, SC_MONSTER}},
    {"CALDERA", {{SK_RUMBLE, 66, 64, 40}, {SK_LAVA, 50, 50, 50}, {SK_LAVA, 40, 40, 80}, {SK_STEAM, 50, 30, 60},
                 {SK_ROCKS, 46, 24, 80}, {SK_CALLS, 56, 24, 100}, {SK_OFF, 0, 0, 0}, {SK_OFF, 0, 0, 0}},
     SD_THROAT, 26, 44, 0, {SC_DEEP_RUMBLE, SC_MONSTER, SC_LOW_GROWL, SC_PTERO}, {SC_LOW_GROWL, SC_MONSTER, SC_TYRANT}},
};
#define SC_NBIOME (sizeof SC_BIOMES / sizeof SC_BIOMES[0])

/* each kind: its events a minute at DENS 64 (0: a bed only), who sings when (DAWN DAY DUSK NIGHT, quarters: 4 is
 * x1), the encounter hushes it (1) */
static const struct { uint8_t per_min; uint8_t tod[4]; uint8_t hush; } SK_INFO[SK_COUNT] = {
    [SK_WIND] = {0, {4, 4, 4, 4}, 0},     [SK_STREAM] = {90, {4, 4, 4, 4}, 0}, [SK_WAVES] = {0, {4, 4, 4, 4}, 0},
    [SK_RAIN] = {200, {4, 4, 4, 4}, 0},   [SK_CICADA] = {0, {1, 4, 3, 0}, 1}, [SK_LAVA] = {20, {4, 4, 4, 4}, 0},
    [SK_RUMBLE] = {0, {4, 4, 4, 4}, 0},   [SK_SONG] = {7, {6, 4, 1, 0}, 1},   [SK_WHISTLE] = {3, {4, 4, 2, 0}, 1},
    [SK_HOOT] = {3, {1, 0, 3, 5}, 1},     [SK_KNOCK] = {2, {4, 4, 1, 0}, 1},  [SK_WADER] = {3, {5, 4, 2, 0}, 1},
    [SK_RAPTOR] = {1, {2, 4, 1, 0}, 1},   [SK_CRICKET] = {20, {1, 0, 3, 5}, 1}, [SK_FROG] = {12, {2, 1, 4, 5}, 1},
    [SK_TOAD] = {4, {1, 0, 4, 5}, 1},     [SK_DRIP] = {12, {4, 4, 4, 4}, 0}, [SK_ROCKS] = {2, {4, 4, 4, 4}, 0},
    [SK_SNAP] = {3, {4, 4, 4, 4}, 1},     [SK_CREAK] = {4, {4, 4, 4, 4}, 0}, [SK_STEAM] = {3, {4, 4, 4, 4}, 0},
    [SK_CALLS] = {2, {4, 4, 4, 3}, 1},    [SK_HERD] = {1, {4, 4, 3, 1}, 1},  [SK_THUNDER] = {1, {4, 4, 4, 4}, 0},
};

/* an event voice: a sine chirp (glide, vibrato), a buzz (a saw gated by a pulse, through a formant), a burst of
 * filtered noise or a thump (a sine falling to its note, a thud of low noise); notes repeated as a phrase */
typedef struct {
    uint8_t type, reps, mode, layer;                    /* mode: the burst's filter (0 lp, 1 bp, 2 hp) */
    uint32_t ph, inc, inc_lo, aph, ainc;                /* (inc_lo: a thump's note, a chirp's glide floor) */
    int32_t glide;                                      /* Q16 per block (65536 none) */
    int32_t env, peak, att, dec;                        /* env Q15 at the block's start; att per block, dec Q16 */
    int32_t gl, gr, gs;                                 /* the pan's gains with the level, the reverb's share */
    int32_t lpk, lpl, lpr;                              /* the distance's low-pass (one pole) and its state */
    int32_t f1, f2, rng;                                /* the filter's state */
    tsvf_t f;
    int16_t lo, hi, pan, panstep;                       /* a phrase's notes (Hz) and place */
    uint16_t gap, wait, vib;                            /* blocks between the notes, to the next; vibrato depth */
    uint8_t stage, dist;                                /* 0 attack, 1 decay, 2 between notes; the distance */
    int32_t lvl;
} sc_voice_t;

typedef struct { int32_t s1[2], s2[2], lvl, lvl_to, cut, base, qa, qc; uint32_t ph, aph, wph; } sc_bed_t;

static struct {
    uint8_t on, biome, tod, weather, danger, dens, level;   /* the globals (the SCAPE page; integration: params) */
    uint8_t dtype, dkey, dmove, dlevel;                     /* the drone */
    sc_layer_t l[SC_NL + SC_NW];
    sc_bed_t bed[SC_NL + SC_NW];
    uint32_t wait[SC_NL + SC_NW];
    sc_voice_t v[SC_NV];
    int32_t rng, hush, hush_to;                             /* the encounter's hush (Q15: 32767 none) */
    uint32_t enc_wait, enc_t, enc_len, enc_step;            /* blocks to the next encounter; into it; its length */
    uint8_t enc_kind, enc_side;
    int32_t dlp[2], dph[4], dmod, dmod_to, dmod_q;          /* the drone's filter, phases, movement (AIR's last) */
    uint32_t dinc, nspawn;                                  /* (nspawn: events started, for the tests) */
    tsvf_t df;
} sc;

static const uint16_t SC_C2[12] = {6541, 6930, 7342, 7778, 8241, 8731, 9250, 9800, 10383, 11000, 11654, 12347};
#define MIDI_HZ_X100(n) ((int32_t)SC_C2[((n) - 36u) % 12u] << (((n) - 36u) / 12u))   /* (C2 .. : Hz x 100) */

static uint32_t sc_rnd(void) { return noise32(&sc.rng); }
static int32_t sc_range(int32_t lo, int32_t hi) { return lo + (int32_t)((sc_rnd() >> 8) % (uint32_t)(hi - lo + 1)); }
static uint32_t sc_hz(int32_t hz) { return (uint32_t)hz * 97392u; }   /* phase increment (2^32 / FS = 97391.5) */

/* a level (Q15) through SCAPE's (a level squared) */
static int32_t sc_out(int32_t lvl) { return mulq15(lvl, (int32_t)sc.level * sc.level * 2); }

/* Hz -> the SVF's cutoff (CUTOFF_HZ's index << 8, between its points) */
static int32_t sc_cut(int32_t hz)
{
    int32_t i = 0, step;
    if (hz <= CUTOFF_HZ[0])
        return 0;
    if (hz >= CUTOFF_HZ[127])
        return 127 << 8;
    for (step = 64; step; step >>= 1)                   /* the last point at or under hz */
        if (i + step < 127 && CUTOFF_HZ[i + step] <= hz)
            i += step;
    return (i << 8) + clamp((hz - CUTOFF_HZ[i]) * 256 / (CUTOFF_HZ[i + 1] - CUTOFF_HZ[i] + 1), 0, 255);
}

/* blocks to wait for an event that comes `per_min` x k (Q8) times a minute: exponential (-ln u, from the bits) */
static uint32_t sc_wait(uint32_t per_min, uint32_t k)
{
    uint32_t u = sc_rnd() | 1u, z = (uint32_t)__builtin_clz(u), f = (u << z) >> 23 & 0xFFu;
    uint32_t nl = ((z + 1u) * 256u - f) * 177u / 256u;       /* -ln u in Q8 (log2 from the bits, x 0.693) */
    uint32_t rate = per_min * k;                              /* x 256 */
    if (!rate)
        return (uint32_t)SC_BPS * 10u;                 /* (looked at again in 10 s: TIME may have changed) */
    return (uint32_t)((uint64_t)SC_BPS * 60u * 256u * nl / 256u / rate) + 1u;
}

/* -------------------------------------------------------------------------------------------- the event voices */
static sc_voice_t *sc_voice(void)                       /* a free one, else none (the scape never steals its own) */
{
    uint32_t i;
    for (i = 0; i < SC_NV; i++)
        if (sc.v[i].type == MV_FREE)
            return &sc.v[i];
    return 0;
}

/* place it: pan -32767..32767, distance 0..127 (+ the room), level Q15 */
static void sc_place(sc_voice_t *v, int32_t pan, int32_t dist, int32_t lvl)
{
    uint32_t a = (uint32_t)(pan + 32768) << 14;          /* 0 .. pi/2 */
    int32_t d = clamp(dist, 0, 127), g = mulq15(lvl, 32767 - d * 200), fc;
    v->gl = mulq15(sine_i(a + 0x40000000u), g);
    v->gr = mulq15(sine_i(a), g);
    v->gs = 5000 + d * 140;                             /* far: more of the room */
    fc = 9000 * (127 - d) / 127 * (127 - d) / 127 + 500;  /* near 9.5 kHz .. far 500 Hz */
    v->lpk = clamp(fc * 46, 1000, 32767);               /* ~ 2 pi fc / FS, Q15 */
    v->pan = (int16_t)pan;
    v->dist = (uint8_t)d;
    v->lvl = lvl;
}

static void sc_env(sc_voice_t *v, uint32_t att_ms, uint32_t dec_ms, int32_t peak)
{
    uint32_t ab = att_ms * SC_BPS / 1000u + 1u, db = dec_ms * SC_BPS / 1000u + 1u;
    v->env = 0;
    v->peak = peak;
    v->att = peak / (int32_t)ab + 1;
    v->dec = 65536 - (int32_t)(65536u * 3u / db);       /* ~5 % left after dec_ms (e^-3) */
    v->stage = 0;
}

/* a note of the phrase (the first, or the next): its pitch drawn in lo .. hi */
static void sc_note(sc_voice_t *v)
{
    int32_t hz = sc_range(v->lo, v->hi);
    v->inc = sc_hz(hz);
    v->ph = 0;
    v->stage = 0;
    v->env = 0;
    if (v->type == MV_THUMP) {                          /* from 2.5x down to the note in ~40 ms */
        v->inc_lo = v->inc;
        v->inc = v->inc * 5u / 2u;
    }
    if (v->panstep) {
        int32_t p = v->pan + v->panstep;
        sc_place(v, clamp(p, -32767, 32767), v->dist, v->lvl);
    }
}

/* spawn an event of layer k's kind (or of the encounter: k = 255), at distance d, level lvl (Q15) */
static void sc_spawn(uint32_t kind, uint32_t k, int32_t d, int32_t lvl)
{
    sc_voice_t *v;
    int32_t pan = (int32_t)(sc_rnd() >> 16) - 32768, r = (int32_t)(sc_rnd() >> 17);
#ifdef SCAPE_TRACE
    SCAPE_TRACE(kind, k, d, lvl);
#endif
    if (kind == SK_CALLS) {                             /* a creature: SAURUS (a voice of the 12) */
        const sc_biome_t *b = &SC_BIOMES[sc.biome];
        scape_call(b->calls[sc_rnd() >> 30], sc_out(clamp(lvl * 6, 0, 32767)), clamp(d + sc_range(-10, 20), 0, 127), pan, 0);
        return;
    }
    if (!(v = sc_voice()))
        return;
    memset(v, 0, sizeof *v);
    sc.nspawn++;
    v->rng = (int32_t)(sc_rnd() | 1u);
    v->layer = (uint8_t)k;
    v->glide = 65536;
    lvl = mulq15(lvl, 16384 + (r >> 1));                /* each a little louder or softer */
    sc_place(v, pan, d, lvl);
    switch (kind) {
    case SK_STREAM:                                     /* a bubble on the stream: a rising sine, quick */
    case SK_DRIP:
    case SK_RAIN:
        v->type = kind == SK_RAIN ? MV_BURST : MV_CHIRP;
        if (kind == SK_STREAM) { v->lo = 300; v->hi = 1300; v->glide = 66240; sc_env(v, 2, 20, 9000); }
        else if (kind == SK_DRIP) { v->lo = 700; v->hi = 1500; v->glide = 66075; sc_env(v, 1, 35, 20000); }
        else {                                          /* a raindrop: a click of bright noise */
            v->mode = 2;
            tsvf_coef(&v->f, sc_cut(2500), 20);
            sc_env(v, 0, 6, 9000 + (r >> 1));
        }
        break;
    case SK_LAVA:                                       /* a thick bubble, low and slow, then its pop */
        v->type = MV_CHIRP;
        v->lo = 70; v->hi = 170; v->glide = 65576;
        sc_env(v, 30, 220, 26000);
        v->reps = 1; v->gap = 2;
        break;
    case SK_SONG:                                       /* a songbird: a phrase of 4 .. 8 quick notes */
        v->type = MV_CHIRP;
        v->lo = 2200; v->hi = 4200;
        v->glide = 65536 + sc_range(-125, 125);
        v->vib = (uint16_t)sc_range(0, 900);
        v->ainc = sc_hz(sc_range(20, 60));
        sc_env(v, 15, 60, 20000);
        v->reps = (uint8_t)sc_range(3, 7); v->gap = (uint16_t)sc_range(30, 110);
        break;
    case SK_WHISTLE:                                    /* a long falling whistle */
        v->type = MV_CHIRP;
        v->lo = 2000; v->hi = 2500; v->glide = 65502;
        v->vib = 600; v->ainc = sc_hz(13);
        sc_env(v, 200, 380, 18000);
        break;
    case SK_HOOT:                                       /* an owl-ish hoot, twice */
        v->type = MV_CHIRP;
        v->lo = 380; v->hi = 440; v->glide = 65528;
        sc_env(v, 80, 180, 22000);
        v->reps = 1; v->gap = 340;
        break;
    case SK_KNOCK:                                      /* a woodpecker on hollow wood */
        v->type = MV_BURST; v->mode = 1;
        tsvf_coef(&v->f, sc_cut(1000), 90);
        sc_env(v, 0, 18, 32767);
        v->reps = (uint8_t)sc_range(8, 15); v->gap = 62;
        break;
    case SK_WADER:                                      /* a water bird: a nasal two-note honk */
        v->type = MV_BUZZ;
        v->lo = 600; v->hi = 640; v->ainc = 0;
        tsvf_coef(&v->f, sc_cut(1600), 60);
        sc_env(v, 30, 150, 14000);
        v->reps = 1; v->gap = 120;
        break;
    case SK_RAPTOR:                                     /* a soaring cry, high and thin */
        v->type = MV_CHIRP;
        v->lo = 2600; v->hi = 3000; v->glide = 65517;
        v->vib = 900; v->ainc = sc_hz(9);
        sc_env(v, 250, 650, 13000);
        break;
    case SK_CRICKET:                                    /* a cricket: a trill of three pulses, a few times */
        v->type = MV_CHIRP;
        v->lo = 3600; v->hi = 4500;
        v->hi = v->lo = (int16_t)sc_range(3600, 4500);
        sc_env(v, 4, 10, 9000);
        v->reps = (uint8_t)sc_range(5, 11); v->gap = 44;
        break;
    case SK_FROG:                                       /* a tree frog: a short buzz of pulses */
        v->type = MV_BUZZ;
        v->lo = 240; v->hi = 280; v->ainc = sc_hz(sc_range(35, 50));
        tsvf_coef(&v->f, sc_cut(sc_range(1600, 2300)), 80);
        sc_env(v, 10, 80, 30000);
        v->reps = (uint8_t)sc_range(0, 2); v->gap = 260;
        break;
    case SK_TOAD:                                       /* a bullfrog: a low throaty croak */
        v->type = MV_BUZZ;
        v->lo = 105; v->hi = 125; v->ainc = sc_hz(22); v->glide = 65525;
        tsvf_coef(&v->f, sc_cut(400), 50);
        sc_env(v, 60, 300, 32767);
        v->reps = (uint8_t)sc_range(0, 1); v->gap = 500;
        break;
    case SK_ROCKS:                                      /* a rock fall: clacks, tumbling, slowing */
        v->type = MV_BURST; v->mode = 1;
        tsvf_coef(&v->f, sc_cut(sc_range(600, 1600)), 60);
        sc_env(v, 0, 40, 32767);
        v->reps = (uint8_t)sc_range(4, 10); v->gap = (uint16_t)sc_range(60, 220);
        v->panstep = (int16_t)sc_range(-2500, 2500);
        break;
    case SK_SNAP:                                       /* a branch snaps */
        v->type = MV_BURST; v->mode = 2;
        tsvf_coef(&v->f, sc_cut(1800), 30);
        sc_env(v, 0, 25, 32767);
        v->reps = (uint8_t)sc_range(0, 2); v->gap = 70;
        break;
    case SK_CREAK:                                      /* ice creaking: a slow buzz rising */
        v->type = MV_BUZZ;
        v->lo = 90; v->hi = 160; v->glide = 65563; v->ainc = sc_hz(sc_range(9, 30));
        tsvf_coef(&v->f, sc_cut(900), 70);
        sc_env(v, 200, 400, 20000);
        break;
    case SK_STEAM:                                      /* a steam vent: a long breath of hiss */
        v->type = MV_BURST; v->mode = 1;
        tsvf_coef(&v->f, sc_cut(3500), 10);
        sc_env(v, 700, 1400, 16000);
        break;
    case SK_HERD:                                       /* a herd passing far off: footfalls rolling across */
        v->type = MV_THUMP;
        v->lo = v->hi = (int16_t)sc_range(45, 60);
        tsvf_coef(&v->f, sc_cut(180), 0);
        sc_env(v, 2, 220, 26000);
        v->reps = (uint8_t)sc_range(25, 45); v->gap = (uint16_t)sc_range(220, 300);
        v->pan = (int16_t)(sc_rnd() >> 31 ? -30000 : 30000);
        v->panstep = (int16_t)(v->pan > 0 ? -1600 : 1600);
        sc_place(v, v->pan, d, lvl);
        break;
    case SK_THUNDER:                                    /* thunder: a long low roll */
        v->type = MV_BURST; v->mode = 0;
        tsvf_coef(&v->f, sc_cut(sc_range(120, 260)), 20);
        sc_env(v, 60, 2600, 32767);
        v->reps = (uint8_t)sc_range(0, 1); v->gap = 500;
        break;
    default:
        v->type = MV_FREE;
        return;
    }
    sc_note(v);
}

/* one event voice's block: its envelope (and glide, vibrato) per block, linear between */
static void sc_voice_run(sc_voice_t *v, int32_t *l, int32_t *r, int32_t *rev, uint32_t n)
{
    int32_t e0 = v->env, e1, de, x, y, i, xb[CTL];
    uint32_t inc = v->inc;
    if (v->stage == 2) {                                /* between the notes */
        if (!--v->wait) {
            sc_note(v);
        }
        return;
    }
    if (!v->stage) {
        e1 = e0 + v->att;
        if (e1 >= v->peak) {
            e1 = v->peak;
            v->stage = 1;
        }
    } else
        e1 = (int32_t)(((int64_t)e0 * v->dec) >> 16);
    v->env = e1;
    if (v->vib) {                                       /* vibrato: per block */
        v->aph += v->ainc * n;
        inc += (uint32_t)((((int32_t)(inc >> 15) * v->vib) >> 15) * sine_i(v->aph));   /* (vib: Q15 of the pitch) */
    }
    de = (e1 - e0) / (int32_t)n;
    switch (v->type) {                                  /* the sound into xb, a loop per kind */
    case MV_CHIRP:
        for (i = 0; i < (int32_t)n; i++) {
            xb[i] = sine_i(v->ph);
            v->ph += inc;
        }
        break;
    case MV_BUZZ:                                       /* a saw, gated by a half-wave pulse (ainc 0: none) */
        for (i = 0; i < (int32_t)n; i++) {
            x = osc_saw(v->ph, inc) >> 1;
            v->ph += inc;
            if (v->ainc) {
                int32_t g = sine_i(v->aph);
                v->aph += v->ainc;
                x = g > 0 ? mulq15(x, g) : 0;
            }
            xb[i] = gurdy_bp(&v->f, x, &v->f1, &v->f2) << 1;
        }
        break;
    case MV_BURST:
        for (i = 0; i < (int32_t)n; i++) {
            int32_t z = (int32_t)(noise32(&v->rng) >> 17) - 16384;
            xb[i] = v->mode == 1 ? gurdy_bp(&v->f, z, &v->f1, &v->f2) << 1
                    : v->mode == 2 ? z - tsvf_lp(&v->f, z, &v->f1, &v->f2) : tsvf_lp(&v->f, z, &v->f1, &v->f2) << 1;
        }
        break;
    default:                                            /* MV_THUMP: the falling sine and a low thud */
        for (i = 0; i < (int32_t)n; i++) {
            int32_t z = (int32_t)(noise32(&v->rng) >> 17) - 16384;
            xb[i] = (sine_i(v->ph) >> 1) + tsvf_lp(&v->f, z, &v->f1, &v->f2);
            if (v->mode == 3u) {                         /* heavy foot: short ground impact above the sub body */
                xb[i] += (sine_i(v->ph) >> 2) + (sine_i(v->ph * 2u) >> 2)
                         + mulq15(z, (int32_t)v->aph) / 2;
                if (++v->ainc == FS * 45u / 1000u)
                    v->aph = 14000u;                   /* ground settling after the initial strike */
                v->aph -= v->aph >> 9;                  /* short gritty impact, below the long low body */
            }
            v->ph += inc;
        }
        break;
    }
    for (i = 0; i < (int32_t)n; i++) {
        x = mulq15(xb[i], e0);
        e0 += de;
        v->lpl += mulq15(x - v->lpl, v->lpk);           /* the distance */
        y = v->lpl;
        l[i] += mulq15(y, v->gl);
        r[i] += mulq15(y, v->gr);
        rev[i] += mulq15(mulq15(y, v->gl + v->gr), v->gs);
    }
    if (v->glide != 65536)                              /* the glide: per block */
        v->inc = (uint32_t)(((uint64_t)v->inc * (uint32_t)v->glide) >> 16);
    if (v->type == MV_THUMP && v->inc > v->inc_lo)
        v->inc -= (v->inc - v->inc_lo) >> (v->mode == 3u ? 6 : 4);
    if (v->stage == 1 && v->env < 40) {                 /* the note is over */
        if (v->reps) {
            v->reps--;
            v->stage = 2;
            v->wait = (uint16_t)(v->gap * (uint32_t)SC_BPS / 1000u + 1u);
        } else
            v->type = MV_FREE;
    }
}

/* ---------------------------------------------------------------------------------------------------- the beds */
/* the layer's level now (Q15): LEVEL squared, the time of day's weight, the weather, the encounter's hush */
static int32_t sc_gain(uint32_t k)
{
    const sc_layer_t *y = &sc.l[k];
    int32_t g = (int32_t)y->level * y->level * 2;       /* 127: ~32258 */
    g = g * SK_INFO[y->kind].tod[sc.tod] >> 3;          /* (x1 is half: room for DAWN's x1.5 and the gale) */
    if (y->kind == SK_WIND && sc.weather != SW_CLEAR && sc.weather != SW_RAIN)
        g <<= 1;                                        /* WIND, STORM: +6 dB */
    if (SK_INFO[y->kind].hush) {
        g = mulq15(g, sc.hush);
        if (sc.weather >= SW_RAIN)
            g >>= sc.weather == SW_STORM ? 2 : 1;       /* the birds and insects quieter in the rain */
    }
    return clamp(g, 0, 32767);
}

static void sc_bed_run(uint32_t k, int32_t *l, int32_t *r, int32_t *rev, uint32_t n)
{
    sc_bed_t *b = &sc.bed[k];
    uint32_t kind = sc.l[k].kind, i;
    int32_t g = sc_gain(k), d = sc.l[k].dist, cut, reso = 0, gl, send = 4000 + d * 120, sh = 0;
    tsvf_t f;
    /* the slow movement: toward a new level when there (WIND gusts, CICADA swells, RUMBLE) */
    if (b->lvl - b->lvl_to < 256 && b->lvl_to - b->lvl < 256)
        b->lvl_to = (kind == SK_CICADA ? 3000 : 9000) + (int32_t)(sc_rnd() >> 18) * (kind == SK_CICADA ? 2 : 1);
    b->lvl += (b->lvl_to - b->lvl) >> (kind == SK_WIND && sc.weather == SW_STORM ? 8 : 10);
    b->cut = clamp(b->cut + (int32_t)(sc_rnd() >> 25) - 64 - (b->cut >> 11), -1536, 1536);
    g = mulq15(g, clamp(b->lvl, 0, 32767));            /* (the level walks about -6 dB .. 0 at its middle) */
    g = mulq15(g, 32767 - d * 160);                     /* far: quieter */
    if (!b->base)                                       /* the band's centre, once */
        b->base = sc_cut(kind == SK_WIND ? 260 : kind == SK_STREAM ? 1100 : kind == SK_WAVES ? 650 : kind == SK_RAIN ? 3000
                         : kind == SK_CICADA ? 5200 : kind == SK_LAVA ? 260 : 55);
    cut = b->base + b->cut;
    switch (kind) {
    case SK_WIND:   cut += b->lvl >> 3; reso = 50; sh = 1; break;   /* gusts brighten it (up to an octave) */
    case SK_STREAM: reso = 0; break;
    case SK_WAVES:  reso = 0; sh = 1; break;
    case SK_RAIN:   reso = 0; break;
    case SK_CICADA: cut = b->base + (b->cut >> 2); reso = 100; break;
    case SK_LAVA:   reso = 10; break;
    default:        cut = b->base; reso = 0; sh = 3; break;   /* SK_RUMBLE */
    }
    if (kind == SK_WAVES) {                             /* the swell: a wave every ~8 s, cubed */
        int32_t s = (sine_i(b->ph) + 32768) >> 1;
        b->ph += sc_hz(1) / 8u * n;
        g = mulq15(g, mulq15(mulq15(s, s), s));
    }
    if (g < 8)
        return;
    gl = mulq15(g, 23170);                              /* (the beds sit in the middle, wide by their noise) */
    if (kind == SK_CICADA || kind == SK_RAIN) {         /* bright: at the full rate */
        tsvf_coef(&f, cut, reso);
        for (i = 0; i < n; i++) {
            uint32_t u = noise32(&sc.rng);              /* (a noise each side from one draw) */
            int32_t a = gurdy_bp(&f, (int32_t)(int16_t)u >> 1, &b->s1[0], &b->s2[0]);
            int32_t c = gurdy_bp(&f, (int32_t)(int16_t)(u >> 16) >> 1, &b->s1[1], &b->s2[1]);
            if (kind == SK_CICADA) {                    /* the buzz: a 40 Hz pulse, cubed */
                int32_t p = (sine_i(b->aph) + 32768) >> 1;
                b->aph += sc_hz(41);
                p = mulq15(mulq15(p, p), p);
                a = mulq15(a, p) << 1;
                c = mulq15(c, p) << 1;
            }
            a = mulq15(a, gl) << sh;
            c = mulq15(c, gl) << sh;
            l[i] += a;
            r[i] += c;
            rev[i] += mulq15(a + c, send);
        }
    } else {                                            /* low (under ~2 kHz): at a quarter of the rate (ambience.c's
                                                         * way: the cutoff + 28 steps, linear between) */
        int32_t pa = b->qa, pc = b->qc, da = 0, dc = 0, a = pa, c = pc, hg = 0;
        uint32_t hinc = 0;
        tsvf_coef(&f, clamp(cut + (28 << 8), 0, 127 << 8), reso);
        if (kind == SK_WIND && SC_BIOMES[sc.biome].howl) {   /* the wind's whistle: a sine an octave over the band,
                                                         * with the gusts (squared) */
            hg = mulq15(mulq15(g, SC_BIOMES[sc.biome].howl * 200), mulq15(b->lvl, b->lvl));
            hinc = sc_hz(CUTOFF_HZ[clamp(cut >> 8, 0, 127)]) * 8u;   /* (x2, at a quarter of the rate) */
        }
        sh -= 1;                                        /* (the quarter rate's noise is 6 dB more in the band) */
        for (i = 0; i < n; i++) {
            if (!(i & 3u)) {
                uint32_t u = noise32(&sc.rng);
                int32_t zl = (int32_t)(int16_t)u >> 1, zr = (int32_t)(int16_t)(u >> 16) >> 1, na, nc;
                if (kind == SK_LAVA || kind == SK_RUMBLE) {
                    na = tsvf_lp(&f, zl, &b->s1[0], &b->s2[0]);
                    nc = tsvf_lp(&f, zr, &b->s1[1], &b->s2[1]);
                } else {
                    na = gurdy_bp(&f, zl, &b->s1[0], &b->s2[0]);
                    nc = gurdy_bp(&f, zr, &b->s1[1], &b->s2[1]);
                }
                na = sh >= 0 ? mulq15(na, gl) << sh : mulq15(na, gl) >> -sh;
                nc = sh >= 0 ? mulq15(nc, gl) << sh : mulq15(nc, gl) >> -sh;
                if (hg) {
                    int32_t w = mulq15(sine_i(b->wph), hg);
                    b->wph += hinc;
                    na += w;
                    nc += w >> 1;
                }
                a = pa;
                c = pc;
                da = (na - pa) >> 2;
                dc = (nc - pc) >> 2;
                pa = na;
                pc = nc;
            }
            a += da;
            c += dc;
            l[i] += a;
            r[i] += c;
            rev[i] += mulq15(a + c, send);
        }
        b->qa = pa;
        b->qc = pc;
    }
}

/* -------------------------------------------------------------------------------------------------- the drone */
static void sc_drone(int32_t *l, int32_t *r, int32_t *rev, uint32_t n)
{
    int32_t g = (int32_t)sc.dlevel * sc.dlevel * 2, m, cut;
    uint32_t i, root = 36u + sc.dkey % 12u;             /* C2 .. B2 */
    if (!sc.dtype || !sc.dlevel)
        return;
    if (sc.dmod - sc.dmod_to < 256 && sc.dmod_to - sc.dmod < 256)
        sc.dmod_to = (int32_t)(sc_rnd() >> 17);
    sc.dmod += (sc.dmod_to - sc.dmod) >> (13 - sc.dmove / 32);   /* MOVE: how fast it wanders */
    m = sc.dmod;
    sc.dinc = 974u * (uint32_t)MIDI_HZ_X100(root);   /* (2^32 / FS / 100) */
    g = mulq15(g, 8000 + (m >> 3));                   /* (a bed under the rest: ~-8 dB, breathing with MOVE) */
    switch (sc.dtype) {
    case SD_PAD:                                        /* four saws (one a fifth up), a low-pass that wanders */
        cut = sc_cut(500) + (m >> 5);
        tsvf_coef(&sc.df, cut, 20);
        for (i = 0; i < n; i++) {
            int32_t a = osc_saw((uint32_t)sc.dph[0], sc.dinc) + osc_saw((uint32_t)sc.dph[1], sc.dinc + (sc.dinc >> 8));
            int32_t c = osc_saw((uint32_t)sc.dph[2], sc.dinc * 3u / 2u - (sc.dinc >> 9)) + osc_saw((uint32_t)sc.dph[3], sc.dinc * 2u + (sc.dinc >> 9));
            sc.dph[0] += (int32_t)sc.dinc;
            sc.dph[1] += (int32_t)(sc.dinc + (sc.dinc >> 8));
            sc.dph[2] += (int32_t)(sc.dinc * 3u / 2u - (sc.dinc >> 9));
            sc.dph[3] += (int32_t)(sc.dinc * 2u + (sc.dinc >> 9));
            a = tsvf_lp(&sc.df, (a >> 2) + (c >> 3), &sc.dlp[0], &sc.dlp[1]);
            c = (a >> 1) + (osc_saw((uint32_t)sc.dph[2], sc.dinc) >> 5);   /* (the fifth a little to the right) */
            a = mulq15(a, g);
            c = mulq15(c, g);
            l[i] += a;
            r[i] += a + (c >> 2);
            rev[i] += a >> 1;
        }
        break;
    case SD_CHOIR: {                                    /* sines on the harmonics, a vowel moving between them */
        int32_t h1 = 26000, h2 = 10000 + (m >> 2), h3 = 14000 - (m >> 3), h4 = 4000 + (m >> 3);
        uint32_t inc = sc.dinc * 2u;                    /* (an octave up: C3 .. B3) */
        for (i = 0; i < n; i++) {
            uint32_t p = (uint32_t)sc.dph[0];
            int32_t a = mulq15(sine_i(p), h1) + mulq15(sine_i(p * 2u), h2) + mulq15(sine_i(p * 3u), h3) + mulq15(sine_i(p * 4u), h4);
            int32_t c = mulq15(sine_i((uint32_t)sc.dph[1]), h1);
            sc.dph[0] += (int32_t)inc;
            sc.dph[1] += (int32_t)(inc + (inc >> 9));
            a = mulq15(a >> 2, g);
            c = mulq15(c >> 2, g);
            l[i] += a + (c >> 1);
            r[i] += a - (c >> 2);
            rev[i] += a;
        }
        break;
    }
    case SD_THROAT:                                     /* a low creature hum: a saw an octave down, one a fourth up */
        cut = sc_cut(320) + (m >> 6);
        tsvf_coef(&sc.df, cut, 30);
        for (i = 0; i < n; i++) {
            int32_t a = osc_saw((uint32_t)sc.dph[0], sc.dinc >> 1) + (osc_saw((uint32_t)sc.dph[1], sc.dinc * 3u / 4u) * 3 >> 3);
            sc.dph[0] += (int32_t)(sc.dinc >> 1);
            sc.dph[1] += (int32_t)(sc.dinc * 3u / 4u);
            a = mulq15(tsvf_lp(&sc.df, a >> 1, &sc.dlp[0], &sc.dlp[1]), g);
            l[i] += a;
            r[i] += a;
            rev[i] += a >> 1;
        }
        break;
    /* (THROAT's saws keep the full rate: at a quarter they would alias) */
    default: {                                          /* SD_AIR: wind sounding the key: noise on the 2nd and 3rd
                                                         * harmonics, at a quarter of the rate (+ 28 steps), linear */
        tsvf_t f2;
        int32_t a = sc.dmod_q, da = 0;
        tsvf_coef(&sc.df, clamp(sc_cut(MIDI_HZ_X100(root) * 2 / 100) + (28 << 8), 0, 127 << 8), 122);
        tsvf_coef(&f2, clamp(sc_cut(MIDI_HZ_X100(root) * 3 / 100) + (28 << 8), 0, 127 << 8), 120);
        for (i = 0; i < n; i++) {
            if (!(i & 3u)) {
                int32_t z = (int32_t)(noise32(&sc.rng) >> 17) - 16384;
                int32_t na = gurdy_bp(&sc.df, z, &sc.dlp[0], &sc.dlp[1]) * 3 + gurdy_bp(&f2, z, &sc.dph[2], &sc.dph[3]) * 2;
                na = mulq15(na >> 1, g);                /* (the quarter rate's 6 dB) */
                a = sc.dmod_q;
                da = (na - a) >> 2;
                sc.dmod_q = na;
            }
            a += da;
            l[i] += a;
            r[i] += a >> 1;
            rev[i] += a >> 1;
        }
        break;
    }
    }
}

/* ---------------------------------------------------------------------------------------------- the encounter */
/* DANGER 0: never; 127: one every ~40 s. From far on one side, closest in the middle, away on the other */
static void sc_encounter(void)
{
    const sc_biome_t *b = &SC_BIOMES[sc.biome];
    uint32_t big = sc.danger < 43u ? 0u : sc.danger < 86u ? 1u : 2u, t, step, half;
    if (!sc.danger)
        return;
    if (!sc.enc_len) {                                  /* waiting for the next */
        if (sc.enc_wait > 1u) {
            sc.enc_wait--;
            return;
        }
        sc.enc_len = (uint32_t)SC_BPS * (10u + 4u * big);
        sc.enc_t = 0;
        sc.enc_step = 0;
        sc.enc_kind = (uint8_t)big;
        sc.enc_side = (uint8_t)(sc_rnd() >> 31);
        sc.hush_to = 32767;
        return;
    }
    t = ++sc.enc_t;
    half = sc.enc_len / 2u;
    sc.hush_to = t > sc.enc_len / 6u && t < sc.enc_len * 5u / 6u ? 4000 : 32767;   /* the jungle goes quiet */
    step = (uint32_t)SC_BPS * (big == 2u ? 95u : big ? 70u : 40u) / 100u;   /* the steps: heavier, slower, bigger */
    if (t >= sc.enc_step) {
        int32_t x = (int32_t)(t * 2u * 32767u / sc.enc_len) - 32767;   /* -1 .. 1 across */
        int32_t near = 127 - (int32_t)(t < half ? t : sc.enc_len - t) * (big == 2u ? 110 : 80) / (int32_t)half;
        sc_voice_t *v = sc_voice();
        sc.enc_step = t + step * (90u + (sc_rnd() >> 28)) / 100u;
        if (v && big) {                                 /* a footfall (a raptor's: claws, a burst) */
            memset(v, 0, sizeof *v);
            v->rng = (int32_t)(sc_rnd() | 1u);
            v->type = MV_THUMP;
            v->glide = 65536;
            v->lo = v->hi = (int16_t)(big == 2u ? 32 : 55);
            tsvf_coef(&v->f, sc_cut(big == 2u ? 140 : 220), 0);
            sc_env(v, big == 2u ? 5 : 2, big == 2u ? 900 : 220, 32767);
            /* Heavy steps emerge from the distance, rise steeply close by, then recede. */
            if (big == 2u)
                x = x * (x < 0 ? -x : x) / 32767;      /* linger near the centre at the closest pass */
            sc_place(v, sc.enc_side ? -x : x, near,
                     big == 2u ? 1800 + (127 - near) * (127 - near) * 30000 / (110 * 110) : 9000);
            sc_note(v);
            if (big == 2u) { v->mode = 3u; v->aph = 32767u; }
        } else if (v) {
            memset(v, 0, sizeof *v);
            v->rng = (int32_t)(sc_rnd() | 1u);
            v->type = MV_BURST; v->mode = 2; v->glide = 65536;
            tsvf_coef(&v->f, sc_cut(2200), 20);
            sc_env(v, 0, 15, 20000);
            v->reps = 1; v->gap = 60;
            sc_place(v, sc.enc_side ? -x : x, near, 6000);   /* (claws: sharp, so soft) */
            sc_note(v);
        }
        if ((sc_rnd() >> 29) == 0u)                     /* now and then a branch under its weight */
            sc_spawn(SK_SNAP, 255, near + 10, 9000);
    }
    if (t == half - (uint32_t)SC_BPS * 3u / 2u)         /* close: a growl */
        scape_call(big ? SC_LOW_GROWL : b->pred[0], sc_out(16000), 40, 0, 2500);
    if (t == half)                                      /* and the roar */
        scape_call(b->pred[big], sc_out(big == 2u ? 26000 : 18000), big == 2u ? 30 : 50, sc.enc_side ? 9000 : -9000, 4000);
    if (t >= sc.enc_len) {
        sc.enc_len = 0;
        sc.hush_to = 32767;
        sc.enc_wait = sc_wait(1, (uint32_t)sc.danger * sc.danger / 32u + 1u) + (uint32_t)SC_BPS * 25u;
    }
}

/* -------------------------------------------------------------------------------------------------- the block */
static void scape_biome(uint32_t b)                    /* a biome's layers, drone and room (the SCAPE page's BIOME) */
{
    uint32_t i;
    sc.biome = (uint8_t)(b % SC_NBIOME);
    for (i = 0; i < SC_NL; i++)
        sc.l[i] = SC_BIOMES[sc.biome].l[i];
    sc.dtype = SC_BIOMES[sc.biome].drone;
    sc.dlevel = SC_BIOMES[sc.biome].dlevel;
}

static void scape_reset(void)
{
    uint32_t i;
    memset(&sc, 0, sizeof sc);
    sc.rng = 0x5CA9E001;
    sc.hush = sc.hush_to = 32767;
    sc.tod = ST_DUSK;
    sc.dens = 72;
    sc.level = 112;                                    /* (~16 dB under the music, alone ~-33 dBFS RMS) */
    sc.danger = 40;
    sc.dmove = 40;
    sc.dkey = 2;                                       /* D */
    scape_biome(1);
    for (i = 0; i < SC_NL + SC_NW; i++) {
        sc.bed[i].lvl = sc.bed[i].lvl_to = 20000;
        sc.wait[i] = (uint32_t)SC_BPS * (1u + i);      /* the layers come in one by one */
    }
    sc.enc_wait = (uint32_t)SC_BPS * 20u;
    sc.on = 1;
}

/* the scape into l, r (the dry mix) and rev (the reverb's send), at the SCAPE level */
static void scape_block(int32_t *l, int32_t *r, int32_t *rev, uint32_t n)
{
    static int32_t bl[CTL], br[CTL], bv[CTL];
    uint32_t k, i;
    int32_t g;
    if (!sc.on || !sc.level)
        return;
    /* the weather's own layers */
    sc.l[SC_NL] = (sc_layer_t){sc.weather >= SW_RAIN ? SK_RAIN : SK_OFF, (uint8_t)(sc.weather == SW_STORM ? 80 : 56), 64, 40};
    sc.l[SC_NL + 1u] = (sc_layer_t){sc.weather == SW_STORM ? SK_THUNDER : SK_OFF, 110, 64, 100};
    sc.hush += (sc.hush_to - sc.hush) >> 8;             /* (the hush: ~0.2 s in and out) */
    sc_encounter();
    for (i = 0; i < n; i++)
        bl[i] = br[i] = bv[i] = 0;
    for (k = 0; k < SC_NL + SC_NW; k++) {
        const sc_layer_t *y = &sc.l[k];
        uint32_t per;
        if (!y->kind || !y->level)
            continue;
        if (y->kind < SK_BEDS)
            sc_bed_run(k, bl, br, bv, n);
        per = SK_INFO[y->kind].per_min;
        if (!per)
            continue;
        if (sc.wait[k] > 1u) {
            sc.wait[k]--;
            continue;
        }
        if (sc.wait[k] == 1u && sc_gain(k) > 64)
            sc_spawn(y->kind, k, y->dist + sc_range(-20, 20), clamp(sc_gain(k), 0, 32767));
        {                                               /* the next: DENS and DENSITY squared, the time of day */
            uint32_t kk = (uint32_t)y->dens * y->dens / 16u * sc.dens * sc.dens / 4096u;   /* Q8 at 64, 64 */
            kk = kk * SK_INFO[y->kind].tod[sc.tod] / 4u;
            if (SK_INFO[y->kind].hush && sc.hush < 16000)
                kk >>= 3;
            sc.wait[k] = sc_wait(per, kk);
        }
    }
    for (k = 0; k < SC_NV; k++)
        if (sc.v[k].type != MV_FREE)
            sc_voice_run(&sc.v[k], bl, br, bv, n);
    sc_drone(bl, br, bv, n);
    g = (int32_t)sc.level * sc.level * 2;               /* SCAPE: a level squared, 127 ~ 0 dB */
    for (i = 0; i < n; i++) {
        l[i] += ((bl[i] >> 3) * g) >> SC_OUT_SH;
        r[i] += ((br[i] >> 3) * g) >> SC_OUT_SH;
        rev[i] += ((bv[i] >> 3) * g) >> SC_OUT_SH;
    }
}
