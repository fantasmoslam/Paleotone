/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the dino groovebox contributors (on SLOOP by isod89 and Felucca by Leo Kuroshita, Hügelton Instruments) */
/* SFX: the prehistoric world's sounds, sixteen of them: thunder, wind, rain, a volcano, lava, a quake, a footstep, wings,
 * insects, a far roar, a river, fire, a rockfall, a tar pit, a squawk. SOUND KEYS plays one sound a key (C, C#, D .. up to
 * D# an octave and a half above, then again: a sound kit on the keys); SOUND 1..16 plays that one sound on every key,
 * key-tracked.
 *
 * Each sound is a recipe for eng_noise.c's source and filter (its MODE, COLR, FREQ, RES, DENS, DRFT, CRSH: noise_core
 * renders it, bit for bit NOISE's own render), plus what makes it move:
 *   sweep   the cutoff starts CENV filter steps away and glides back over CTIME (a crack that darkens, a step's thud)
 *   motion  an LFO on the cutoff (LCUT) and the loudness (LAMP): gusts, wing beats, an insect's buzz, a quake's tremor
 *   thump   a sine falling from TH0 to TH1 Hz over THDEC, mixed under the noise: the weight of a footstep or a quake
 *   level   the engine's own envelope (ownenv): ATT, DEC to SUS (0: a one-shot that ends by itself, even held), REL
 * The user's knobs scale the recipe: SIZE (darker, the thump lower), LEN (every time), TONE (the cutoff alone),
 * MOVE (the motion and the drift), DENS (the impulses), THMP (the thump), WILD (each note a little its own).
 *
 * Voice state: v->s[], v->ph[] are NOISE's (noise_core); the rest in the part's arena (sfx_part_t): the envelope, the
 * sweep, the LFO, the thump. */

enum { SFX_NSND = 16, SFX_GAIN = 4 };
static const char *const N_SFX_SOUND[SFX_NSND + 2] = {"KEYS", "THUNDER", "RUMBLE", "WIND", "RAIN", "VOLCANO", "LAVA",
                                                      "QUAKE", "FOOTSTEP", "WINGS", "INSECTS", "FAR ROAR", "RIVER",
                                                      "FIRE", "ROCKFALL", "TAR PIT", "SQUAWK", 0};
/* times: 0..127 = 1 ms .. 10 s (ENV_LIN's), freq: the filter's 0..127 (30 Hz .. 16 kHz) */
typedef struct {
    int8_t nz[7];        /* NOISE's MODE, COLR, FREQ, RES, DENS, DRFT, CRSH (TRK: 0, or 64 with one SOUND) */
    int8_t cenv;         /* the sweep's start, filter steps (+ brighter), back to 0 over ctime */
    uint8_t ctime;
    uint8_t lrate;       /* the LFO, 0.1 Hz */
    uint8_t lcut, lamp;  /* its depth on the cutoff (filter steps) and the loudness (127 = all of it) */
    uint8_t att, dec, sus, rel;
    uint8_t th0, th1;    /* the thump, Hz: from, to (0 = none) */
    uint8_t thdec, thlvl;
} sfx_recipe_t;
static const sfx_recipe_t SFX_R[SFX_NSND] = {
    /*  MODE COLR FREQ RES DENS DRFT CRSH   CENV CT   LFO LCUT LAMP  ATT DEC SUS REL   TH0 TH1 THD THL */
    {{NZ_ANLG, 0, 93, 30, 90, 0, 0}, -55, 100, 0, 0, 0, 5, 105, 0, 90, 60, 35, 95, 100},       /* THUNDER: a crack, darkening */
    {{NZ_ANLG, 127, 40, 10, 0, 110, 0}, 0, 0, 6, 10, 90, 60, 120, 0, 100, 0, 0, 0, 0},         /* RUMBLE: thunder rolling away */
    {{NZ_ANLG, 64, 70, 75, 0, 90, 0}, 0, 0, 2, 18, 40, 90, 0, 127, 100, 0, 0, 0, 0},           /* WIND: a howl, gusting */
    {{NZ_DUST, 30, 88, 80, 105, 40, 0}, 0, 0, 0, 0, 0, 70, 0, 127, 95, 0, 0, 0, 0},            /* RAIN */
    {{NZ_ANLG, 127, 28, 20, 60, 100, 0}, 0, 0, 3, 0, 30, 80, 0, 127, 105, 40, 30, 110, 60},    /* VOLCANO: a deep roar */
    {{NZ_DUST, 0, 50, 120, 60, 80, 0}, 0, 0, 0, 0, 0, 40, 0, 127, 90, 0, 0, 0, 0},             /* LAVA: bubbling */
    {{NZ_ANLG, 127, 14, 40, 0, 60, 0}, 0, 0, 60, 0, 100, 70, 115, 0, 100, 30, 25, 115, 80},    /* QUAKE: a tremor */
    {{NZ_ANLG, 100, 45, 20, 0, 0, 0}, 20, 70, 0, 0, 0, 0, 84, 0, 80, 70, 38, 80, 127},         /* FOOTSTEP: a giant's */
    {{NZ_ANLG, 64, 66, 50, 0, 0, 0}, 0, 0, 40, 15, 120, 30, 105, 0, 90, 0, 0, 0, 0},           /* WINGS: beating */
    {{NZ_ANLG, 0, 103, 115, 0, 50, 0}, 0, 0, 250, 0, 110, 60, 0, 127, 90, 0, 0, 0, 0},         /* INSECTS: a buzzing swarm */
    {{NZ_ANLG, 110, 55, 70, 0, 40, 0}, 15, 100, 0, 0, 0, 70, 108, 0, 95, 90, 55, 105, 70},     /* FAR ROAR */
    {{NZ_DUST, 64, 79, 60, 120, 70, 0}, 0, 0, 3, 8, 0, 80, 0, 127, 100, 0, 0, 0, 0},           /* RIVER */
    {{NZ_DUST, 0, 95, 10, 70, 60, 0}, 0, 0, 0, 0, 0, 50, 0, 127, 95, 0, 0, 0, 0},              /* FIRE: crackling */
    {{NZ_DUST, 80, 70, 30, 100, 60, 0}, -20, 110, 0, 0, 0, 5, 110, 0, 95, 50, 35, 95, 60},     /* ROCKFALL */
    {{NZ_DUST, 50, 38, 125, 40, 90, 0}, 0, 0, 0, 0, 0, 60, 0, 127, 100, 0, 0, 0, 0},           /* TAR PIT: slow gloops */
    {{NZ_ANLG, 0, 79, 125, 0, 0, 0}, 18, 70, 120, 12, 0, 0, 84, 0, 70, 0, 0, 0, 0},            /* SQUAWK: a bird's cry */
};

typedef struct {
    int32_t env;         /* Q24 */
    int32_t sweep;       /* the sweep's progress, Q24 (1 << 24: done) */
    uint32_t lfo;        /* the LFO's phase */
    int32_t lfo_v;       /* its value this tick, -32768..32767 */
    uint32_t thph;       /* the thump's phase */
    int32_t thenv;       /* its fall, Q24 (1 << 24 at the note-on, 0: at TH1 and silent) */
    int32_t dcx, dcy;    /* the DC blocker (brown noise and the thump's start carry some) */
    uint8_t stage;       /* 0 attack, 1 decay, 2 sustain, 3 release, 4 done */
    uint8_t snd, rnd;    /* this note's sound and its random number (WILD) */
} sfx_voice_t;
typedef struct sfx_part {
    sfx_voice_t v[NPOLY];
} sfx_part_t;

static sfx_voice_t *sfx_sv(track_t *t, voice_t *v)
{
    sfx_part_t *a = (sfx_part_t *)eng_arena_of(t, ENGI_SFX);
    uint32_t i = (uint32_t)(v - t->v);
    return a && i < NPOLY ? &a->v[i] : 0;
}

/* a recipe time, LEN (and WILD) applied: an index into ENV_LIN */
static uint32_t sfx_time(const track_t *t, const sfx_voice_t *s, int32_t x)
{
    int32_t w = (((int32_t)s->rnd - 64) * t->p[P_E7]) >> 9;   /* WILD: up to +-16 steps (x 0.7 .. x 1.4) */
    return (uint32_t)clamp(x + ((t->p[P_E2] - 64) >> 1) + w, 0, 127);
}

static void sfx_note_on(track_t *t, voice_t *v)
{
    sfx_voice_t *s = sfx_sv(t, v);
    noise_note_on(t, v);
    if (!s)
        return;
    s->snd = (uint8_t)(t->p[P_E0] ? (t->p[P_E0] - 1) % SFX_NSND : (v->note + 128u - 36u) % SFX_NSND);
    s->rnd = (uint8_t)((uint32_t)v->s[2] >> 25);   /* (NOISE's seed: the note's) */
    s->env = 0;
    s->stage = 0;
    s->sweep = SFX_R[s->snd].cenv ? 0 : 1 << 24;
    s->lfo = (uint32_t)v->s[2];                    /* each note its own phase */
    s->lfo_v = 0;
    s->thph = 0;
    s->thenv = SFX_R[s->snd].th0 ? 1 << 24 : 0;
    s->dcx = s->dcy = 0;
}

/* the level, once per control tick: the envelope, the LFO's loudness, the velocity */
static int32_t sfx_amp(track_t *t, voice_t *v, int32_t adsr)
{
    sfx_voice_t *s = sfx_sv(t, v);
    const sfx_recipe_t *r;
    int32_t a, mv = t->p[P_E4];
    if (!s)
        return 0;
    r = &SFX_R[s->snd];
    if (!v->gate && s->stage < 3u)
        s->stage = 3;
    switch (s->stage) {
    case 0:
        s->env += (int32_t)ENV_LIN[sfx_time(t, s, r->att)];
        if (s->env >= 1 << 24) {
            s->env = 1 << 24;
            s->stage = r->dec ? 1 : 2;
        }
        break;
    case 1: {
        int32_t sus = r->sus << 17;                   /* (127 << 17: about 1.0) */
        s->env -= (int32_t)ENV_LIN[sfx_time(t, s, r->dec)];
        if (s->env <= sus) {
            s->env = sus;
            s->stage = sus ? 2 : 4;
        }
        break;
    }
    case 2:
        break;
    case 3:
        s->env -= (int32_t)ENV_LIN[sfx_time(t, s, r->rel)];
        if (s->env <= 0) {
            s->env = 0;
            s->stage = 4;
        }
        break;
    default:
        s->env = 0;
        break;
    }
    if (s->sweep < 1 << 24)
        s->sweep += (int32_t)ENV_LIN[sfx_time(t, s, r->ctime)];
    if (s->thenv > 0)
        s->thenv -= (int32_t)ENV_LIN[sfx_time(t, s, r->thdec)];
    if (r->lrate) {                                  /* 0.1 .. 25 Hz, a sine with a little wobble */
        s->lfo += (uint32_t)r->lrate * 9739u * CTL + ((uint32_t)s->rnd << 10);
        s->lfo_v = sine_i(s->lfo);
    }
    a = (s->env >> 9) * (adsr >> 0) >> 15;            /* Q15 (adsr: 32767, or the steal's fade) */
    a = (a * a) >> 15;                               /* (squared: a level that sounds even) */
    if (r->lamp && mv) {                             /* the LFO on the loudness: MOVE 64 = the recipe */
        int32_t d = clamp((r->lamp * mv) >> 6, 0, 127) * 258;
        a = (a * (32767 - ((d * (32767 - ((s->lfo_v + 32768) >> 1))) >> 15))) >> 15;
    }
    return (a * (v->vel * 258)) >> 15;
}

static int sfx_done(track_t *t, voice_t *v)
{
    sfx_voice_t *s = sfx_sv(t, v);
    return !s || s->stage >= 4u;
}

static void sfx_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    sfx_voice_t *s = sfx_sv(t, v);
    const sfx_recipe_t *r;
    int16_t e[8];
    vmod_t mm = *m;
    int32_t size = p[P_E1] - 64, cut, sw, w;
    uint32_t i;
    if (!s)
        return;
    r = &SFX_R[s->snd];
    w = (((int32_t)s->rnd - 64) * p[P_E7]) >> 10;     /* WILD: up to +-8 filter steps */
    e[0] = r->nz[0];
    e[1] = r->nz[1];
    e[2] = (int16_t)clamp(r->nz[2] - (size >> 1) + ((p[P_E3] - 64) >> 1) + w, 0, 127);   /* SIZE, TONE */
    e[3] = r->nz[3];
    e[4] = p[P_E0] ? 64 : 0;                          /* one SOUND: the keys move it; KEYS: each key its sound */
    e[5] = (int16_t)clamp((r->nz[4] * p[P_E5]) >> 6, 0, 127);   /* DENS: 64 = the recipe */
    e[6] = (int16_t)clamp((r->nz[5] * p[P_E4]) >> 6, 0, 127);   /* MOVE: the drift too */
    e[7] = r->nz[6];
    /* the sweep (a square fall back to the recipe's cutoff) and the LFO on the cutoff, in filter steps << 8 */
    sw = s->sweep >= 1 << 24 ? 0 : (1 << 12) - (s->sweep >> 12);
    cut = ((r->cenv * ((sw * sw) >> 12)) >> 4) + (((r->lcut * p[P_E4]) >> 6) * s->lfo_v >> 7);
    mm.cutoff = m->cutoff + cut;
    if (!p[P_E0])                                     /* KEYS: the sound at its own pitch (C4) */
        mm.pitch16 = 60 * 16;
    {   /* the noise, then SFX_GAIN: the recipes' filters take most of the noise's level (NOISE's own makeup is for
         * its presets); in blocks of CTL (a stack buffer), saturating */
        int32_t buf[CTL];
        uint32_t o, k, c;
        for (o = 0; o < n; o += c) {
            vmod_t mb = mm;
            c = n - o < CTL ? n - o : CTL;
            for (k = 0; k < c; k++)
                buf[k] = 0;
            if (o) {                                  /* (the ramp of this chunk: amp_at at its offset) */
                mb.amp0 = amp_at(&mm, o);
                mb.amp1 = amp_at(&mm, o + c < n ? o + c : n);
            }
            noise_core(e, v, buf, c, &mb);
            for (k = 0; k < c; k++) {                 /* a DC blocker (1 - 5/1024: ~35 Hz; below, the master's blocker rang on), then the gain */
                int32_t x = clamp(buf[k], -(1 << 24), 1 << 24), y = x - s->dcx + s->dcy - (s->dcy >> 8) - (s->dcy >> 10);
                s->dcx = x;
                s->dcy = y;
                out[o + k] += y * SFX_GAIN;
            }
        }
    }
    if (s->thenv > 0 && p[P_E6]) {                    /* the thump */
        int32_t f = r->th1 + (int32_t)(((int64_t)(r->th0 - r->th1) * s->thenv) >> 24), g, x;
        uint32_t inc;
        f = (f * (128 - (size >> 1))) >> 7;           /* SIZE: bigger is lower */
        if (p[P_E0])
            f = (int32_t)(((int64_t)f * fmt_ratio((m->pitch16 - 60 * 16) >> 1)) >> 16);   /* half the key's moves */
        inc = (uint32_t)clamp(f, 10, 400) * 97391u;
        g = (((s->thenv >> 9) * r->thlvl) >> 7) * p[P_E6] >> 6;   /* Q15, THMP 64 = the recipe */
        g = (g * ((s->thenv >> 9))) >> 15;            /* falls with its pitch */
        for (i = 0; i < n; i++) {
            x = (sine_i(s->thph) * g) >> 15;
            s->thph += inc;
            out[i] += voice_amp(x >> 1, m, i);
        }
    }
}

/* {SOUND, SIZE, LEN, TONE, MOVE, DENS, THMP, WILD}; the ADSR is not used (the engine's own envelope) */
#define SFXP(snd, rev) {snd, 64, 64, 64, 64, 64, 64, 30}, {0, 64, 127, 64}, 0, 0, FX(0, 0, 0, rev)
static const preset_t SFX_PRESETS[] = {
    {"DINO WORLD", SFXP(0, 60)},
    {"THUNDER", SFXP(1, 70)},
    {"RUMBLE", SFXP(2, 60)},
    {"WIND HOWL", SFXP(3, 50)},
    {"RAIN", SFXP(4, 40)},
    {"VOLCANO", SFXP(5, 50)},
    {"LAVA", SFXP(6, 30)},
    {"QUAKE", SFXP(7, 40)},
    {"FOOTSTEP", SFXP(8, 50)},
    {"WINGS", SFXP(9, 40)},
    {"INSECTS", SFXP(10, 30)},
    {"FAR ROAR", SFXP(11, 80)},
    {"RIVER", SFXP(12, 40)},
    {"FIRE", SFXP(13, 30)},
    {"ROCKFALL", SFXP(14, 60)},
    {"TAR PIT", SFXP(15, 40)},
    {"SQUAWK", SFXP(16, 60)},
};

static const engine_t ENG_SFX = {
    .name = "SFX",
    .page_title = {"SOUND", "SHAPE"},
    .edit = {
        {"SOUND", F_ENUM, 0, SFX_NSND, 0, N_SFX_SOUND, 0},
        {"SIZE", F_INT, 0, 127, 64, 0, 0},
        {"LEN", F_INT, 0, 127, 64, 0, 0},
        {"TONE", F_INT, 0, 127, 64, 0, 0},
        {"MOVE", F_PCT, 0, 127, 64, 0, 0},
        {"DENS", F_PCT, 0, 127, 64, 0, 0},
        {"THMP", F_PCT, 0, 127, 64, 0, 0},
        {"WILD", F_PCT, 0, 127, 30, 0, 0},
    },
    .presets = SFX_PRESETS,
    .npresets = NELEM(SFX_PRESETS),
    .note_on = sfx_note_on,
    .render = sfx_render,
    .fil_page = -1,
    .color = 0x7BEF,
    .macro = {P_E0, P_E1, P_E2, P_E3},
    .amp = sfx_amp,
    .ownenv = 1,
    .done = sfx_done,
};
