/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS: Paleotone's sounds as voices for NACHTFAHRT's layers (pt.h), a unit of its own.
 *
 * The files in this folder are Paleotone's synth: its engines, voices, parameters and presets, from
 * github.com/fantasmoslam/paleotone at 6f027f0 (on SLOOP and Felucca, by Leo Kuroshita and the others named in each
 * file), as they are there, but for:
 *   core.h        NSTEP may be set before it (here 16: no patterns are played, ~6 KB of RAM less); the parts (trk)
 *                 may go in a section of the includer's (here the pool)
 *   engines.c     the big engines' state (eng_arena) for the first ENG_ARENAS parts (here the four layers')
 *   eng_sample.c  a stub: the SAMPLE engine's recordings (205 KB) are not built; engine 4 keeps its number, silent
 *   eng_grain.c   a stub: GRAIN reads those recordings; engine 8 keeps its number, silent
 *   felucca_*.h   the tables Paleotone's build generates (tools/gen_tables.py, gen_fm6_patches.py, gen_drumkits.py)
 * Each layer with a Paleotone sound is a Paleotone part (track 0..3; Paleotone's shared budget of 8 voices across
 * them), rendered as Paleotone renders it (voice.c track_render, 32-sample blocks), its LEVEL and the sound's level
 * trim applied; its own sends, drive and insert are not: the layer's echo and plate are NACHTFAHRT's. The blocks are
 * read out as floats at whatever length the render asks (pt_render), so a note starts at the next block's edge
 * (0.7 ms at most). */
#include "pt.h"
#define memset pt_memset             /* (Paleotone's libc.c, renamed: the firmware has its own) */
#define memcpy pt_memcpy
#define memcmp pt_memcmp
#define bootguard pt_bootguard       /* (core.h's; the firmware's is its own) */
#ifndef NSTEP
#define NSTEP 16
#endif
#define ENG_ARENAS PT_NLAYER         /* (engines.c: the big engines' state, the four layers' parts only) */
#ifdef EN_HOST
#define TRK_SECTION
#else
#define TRK_SECTION __attribute__((section(".pool")))   /* (core.h's parts: RAM's .bss is full) */
#endif
#include "felucca_tables.h"
#include "libc.c"
#include "core.h"
#include "cpu1.h"
static int cpu1_run(cpu1_job_t fn, uint32_t arg) { (void)fn, (void)arg; return 0; }   /* (one core: voice.c never asks) */
static int cpu1_wait(void) { return 0; }
static int cpu1_on(void) { return 0; }
static uint32_t cpu1_ticks(void) { return 0; }
#include "engines.c"
#include "drums.c"
#include "params.c"
#include "voice.c"

/* ---- the lists: each layer's sounds, in the order its knob turns through them (dark first: pads and drones for the
 * Drone, struck things for the Sequences, horns and voices for the Lead), each a preset of one engine by its name */
typedef struct {
    uint8_t e;                       /* the engine (core.h ENGI_*, the ENGINES[] index) */
    const char *preset, *name;       /* its preset's name in the engine, and the name the screen shows */
    float g;                         /* its level against the list's others (tests/host/twr_voice_test.c: by RMS, each
                                      * at its layer's median; 0: 1.0) */
} pt_snd_t;
static const pt_snd_t DRONE_SND[PT_NDRONE] = {
    {14, "TUNDRA", "Tundra", 2.065f}, {14, "EARTHQUAKE", "Quake", 1.905f}, {14, "PRIMEVAL", "Primeval", 1.818f}, {14, "SWAMP FIDDLE", "Fiddle", 2.618f},
    {10, "BOWED METAL", "Bow Metal", 5.702f}, {10, "BOWED BONE", "Bow Bone", 6.124f},
    {5, "DEEP CHANT", "Chant", 0.351f}, {5, "NIGHT WAIL", "Wail", 1.300f}, {5, "SPIRIT VOX", "Spirit", 0.850f},
    {5, "CHOIR AAH", "Aah Choir", 0.582f}, {5, "EPIC CHOIR", "Big Choir", 0.910f}, {3, "DINO CHOIR", "Dino Vox", 1.116f},
    {0, "ICE AGE PAD", "Ice Age", 5.598f}, {0, "JUNGLE PAD", "Jungle", 1.268f}, {0, "SOFT PAD", "Soft Pad", 1.143f},
    {0, "STRINGS", "Strings", 0.976f}, {0, "EPIC STRINGS", "Epic Str", 1.028f}, {0, "FERN SWELL", "Swell", 2.529f},
    {0, "STAR SKY", "Star Sky", 0.806f}, {2, "RESIN PAD", "Resin Pad", 1.019f}, {6, "PANGAEA PAD", "Pangaea", 1.149f},
    {6, "SKY STRINGS", "Sky Str", 0.877f}, {6, "SPORE CLOUD", "Spores", 1.271f}, {6, "AURORA PAD", "Aurora", 0.974f},
    {9, "GLACIER PAD", "Glacier", 2.556f}, {7, "TEMPLE ORGAN", "Temple", 1.327f},
    {0, "CONCERT CELLO", "Cello", 1.0f}, {0, "DARK CELLO", "Cello Low", 1.0f},
    {0, "DEEP SUB PAD", "Sub Pad", 0.65f}, {0, "FAT BASS PAD", "Fat Pad", 0.85f},
};
static const pt_snd_t SEQ_SND[PT_NSEQ] = {
    {10, "KALIMBA", "Kalimba", 1.975f}, {10, "LOG MARIMBA", "Log Bars", 2.848f}, {10, "MARIMBA", "Marimba", 3.886f},
    {10, "TUSK TINES", "Tusk Tine", 0.908f}, {10, "GEODE BELLS", "Geode", 0.788f}, {10, "BELL TREE", "Bell Tree", 1.142f},
    {10, "WATER DROPS", "Waterdrop", 0.942f}, {10, "HARP", "Harp", 1.811f}, {10, "VALLEY HARP", "Vale Harp", 2.415f},
    {10, "SITAR VINE", "Sitar", 0.969f}, {10, "PLUCK", "Pluck", 3.690f}, {9, "SLIT DRUM", "Slit Drum", 3.281f},
    {9, "AMBER BELLS", "Amber", 0.893f}, {9, "BELL", "FM Bell", 0.935f}, {9, "TINE EP", "Tine EP", 0.638f}, {2, "BELL", "Bell", 1.027f},
    {2, "WIRE", "Wire", 0.558f}, {2, "ICE CRYSTAL", "Crystal", 0.487f}, {3, "EGG ARP", "Egg Arp", 0.623f}, {6, "RING BELL", "Ring Bell", 2.323f},
    {6, "PRIMAL STAB", "Primal", 1.187f}, {0, "SINE KEY", "Sine Key", 0.815f},
    {0, "CONCERT CELLO", "Cello", 1.0f}, {0, "DARK CELLO", "Cello Low", 1.0f},
    {0, "DEEP SUB PAD", "Sub Pad", 0.65f}, {0, "FAT BASS PAD", "Fat Pad", 0.85f},
};
static const pt_snd_t LEAD_SND[PT_NLEAD] = {
    {9, "MAMMOTH", "Mammoth", 2.097f}, {9, "MAMMOTH CALL", "Mammoth 2", 1.822f}, {6, "BRONTO HORN", "Bronto", 2.084f},
    {0, "CANYON HORN", "Canyon", 0.715f}, {3, "BONE WHISTLE", "Whistle", 1.048f}, {3, "CAVE ECHO", "Cave Echo", 1.094f},
    {5, "SHAMAN", "Shaman", 0.576f}, {5, "VOX LEAD", "Vox Lead", 0.592f}, {3, "FOSSIL LEAD", "Fossil", 1.242f}, {0, "WIND", "Wind", 0.711f},
    {2, "TITAN BRASS", "Titan", 1.146f}, {6, "LAVA BRASS", "Lava", 0.685f}, {9, "BRASS", "FM Brass", 1.766f},
    {0, "AMBER LEAD", "Amber", 0.617f}, {0, "SAW LEAD", "Saw Lead", 1.014f}, {2, "RESO", "Reso", 3.451f}, {3, "WAVE LEAD", "Wave Lead", 2.075f},
    {6, "SYNC LEAD", "Sync Lead", 1.059f},
    {0, "CONCERT CELLO", "Cello", 1.0f}, {0, "DARK CELLO", "Cello Low", 1.0f},
    {0, "DEEP SUB PAD", "Sub Pad", 0.65f}, {0, "FAT BASS PAD", "Fat Pad", 0.85f},
};
static const pt_snd_t BASS_SND[PT_NSEQ2 - PT_NSEQ] = {   /* (Sequence 2's: these, then the Sequence's) */
    {9, "TAR PIT BASS", "Tar Bass", 0.977f}, {9, "TAR GROWL", "Tar Growl", 1.142f}, {0, "SUB BASS", "Sub Bass", 0.590f},
    {0, "SWAMP BASS", "Swamp", 0.742f}, {0, "STOMP BASS", "Stomp", 0.486f}, {2, "DEEP PULSE", "Pulse", 0.434f},
    {3, "WAVE BASS", "Wave Bass", 1.552f}, {5, "WOW BASS", "Wow Bass", 0.638f}, {6, "FAT BASS", "Fat Bass", 1.439f}, {9, "FM BASS", "FM Bass", 1.308f},
    {0, "SQR BASS", "Sqr Bass", 1.016f}, {0, "ACID", "Acid", 1.627f},
};
static const pt_snd_t CALL_SND[PT_NCALL] = {
    {12, "DISTANT HERD", "Herd", 0.455f}, {12, "RAPTOR CHAT", "Raptor", 1.027f},
    {12, "CREST HOOT", "Hoot", 0.386f}, {12, "HERD MOAN", "Moan", 0.607f},
    {12, "NEST CHIRPS", "Nest", 1.717f}, {12, "SWAMP CROAK", "Croak", 1.030f},
    {12, "HADRO HORN", "Hadro", 0.366f}, {12, "SAURO SNORT", "Snort", 1.884f},
};
const uint8_t PT_N[PT_NPART] = {PT_NDRONE, PT_NSEQ, PT_NLEAD, PT_NSEQ2, PT_NCALL};
static const pt_snd_t *snd_of(int layer, int k)
{
    if ((unsigned)layer >= PT_NPART || k < 0 || k >= PT_N[layer])
        return 0;
    switch (layer) {
    case PT_DRONE: return &DRONE_SND[k];
    case PT_SEQ: return &SEQ_SND[k];
    case PT_LEAD: return &LEAD_SND[k];
    case PT_CALLS: return &CALL_SND[k];
    default: return k < PT_NSEQ2 - PT_NSEQ ? &BASS_SND[k] : &SEQ_SND[k - (PT_NSEQ2 - PT_NSEQ)];
    }
}
static int preset_of(const pt_snd_t *s)          /* its preset's index in its engine, -1: none */
{
    const engine_t *e = ENGINES[s->e % NENGINES];
    uint32_t k;
    for (k = 0; k < e->npresets; k++)
        if (str_eq(e->presets[k].name, s->preset))
            return (int)k;
    return -1;
}
const char *pt_name(int layer, int k)
{
    const pt_snd_t *s = snd_of(layer, k);
    return s ? s->name : "-";
}
int pt_mono(int layer, int k)
{
    const pt_snd_t *s = snd_of(layer, k);
    int pi = s ? preset_of(s) : -1;
    return pi >= 0 && ENGINES[s->e % NENGINES]->presets[pi].mono;
}

/* ---- the layers as parts */
#define NGATE 8
typedef struct {
    uint8_t snd;                     /* its sound + 1, 0: none */
    uint8_t pos;                     /* where pt_render reads in buf (CTL: a block is due) */
    uint8_t live;                    /* 1: buf holds sound (0: the block was silent) */
    float tg;                        /* its sound's level (pt_snd_t g) */
    uint8_t gn[NGATE];               /* the notes that let go by themselves, and the samples left of each (0: free) */
    uint32_t gl[NGATE];
    float buf[CTL];
} pt_layer_t;
static pt_layer_t lay[PT_NPART];

void pt_init(void)
{
    uint32_t i, k;
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    for (i = 0; i < NTRK; i++)
        for (k = 0; k < P_E0; k++)
            trk[i].p[k] = TP[k].def;
    fm6_init();
    for (i = 0; i < PT_NPART; i++) {
        lay[i].snd = 0;
        lay[i].pos = CTL;
    }
}

/* Paleotone's ui.c apply_preset_to and set_engine_of, for a part that plays no pattern and has no user presets, no
 * arpeggiator kept, no locks: the engine and its preset, the rest of the sound to its defaults; the mix (LEVEL) kept */
static void load(track_t *t, uint32_t ei, uint32_t pi)
{
    const engine_t *e = ENGINES[ei % NENGINES];
    const preset_t *pr = &e->presets[pi % e->npresets];
    uint32_t i;
    for (i = 0; i < NVOICE; i++) {                  /* (the old sound's voices gone at once: the caller faded it) */
        t->v[i].active = t->v[i].gate = 0;
        t->v[i].stage = 0;
        t->v[i].env = t->v[i].env_out = 0;
        t->v[i].pend = 0;
    }
    t->nmono = 0, t->mono_note = 0, t->xp_n = 0, t->xf_on = 0, t->xf = 0, t->pold = 0;
    for (i = 0; i < P_E0; i++)
        if (i != P_LEVEL)
            t->p[i] = TP[i].def;
    t->eng_req = t->engine = (uint8_t)(ei % NENGINES);
    t->preset = (uint8_t)(pi % e->npresets);
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + i] = (int16_t)pr->e[i];
    if (ei == ENGI_ROAR)
        t->p[P_FTYPE] = (int16_t)(pi < 32u && ((GURDY_RING >> pi) & 1u));
    t->p[P_ATK] = pr->env[0];
    t->p[P_DEC] = pr->env[1];
    t->p[P_SUS] = pr->env[2];
    t->p[P_REL] = pr->env[3];
    t->p[P_ED_FLT] = pr->fenv;
    t->p[P_ED_FX] = preset_trim(ei % NENGINES, pi);
    t->p[P_VOICE] = pr->mono ? V_LEGATO : V_POLY;
    for (i = 0; i < 4u; i++)                        /* (no arpeggiator: the layers play their own notes) */
        t->p[P_DIST + i] = (int16_t)(pr->fx[i] ? pr->fx[i] - 1 : 0);
    preset_extras(t->p, pr);
    t->p[P_AMODE] = 0;
    for (i = 0; i < 8u; i++)
        t->pe_old[i] = t->p[P_E0 + i];
    fm6_track_loaded(t);
    fm6_poll();
}

void pt_sound(int layer, int k)
{
    const pt_snd_t *s;
    int pi;
    if ((unsigned)layer >= PT_NPART)
        return;
    s = snd_of(layer, k);
    pi = s ? preset_of(s) : -1;
    if (pi < 0) {
        pt_quiet(layer);
        lay[layer].snd = 0;
        return;
    }
    if (lay[layer].snd == k + 1)
        return;
    load(&trk[layer], s->e, (uint32_t)pi);
    if (layer == PT_CALLS) trk[layer].p[P_VOICE] = V_MONO;
    if (s->e == 0 && pi >= 21) trk[layer].p[P_ED_FX] = 0; /* new voices have explicit list gains */
    lay[layer].tg = s->g > 0.0f ? s->g : 1.0f;
    if (layer == PT_SEQ2 && k >= PT_NSEQ2 - PT_NSEQ)
        lay[layer].tg *= 1.62f;                     /* (the Sequence's sounds on Sequence 2, lower: +4.2 dB, their median
                                                     * there; tools/pt_levels.py levels them on the Sequence) */
    memset(lay[layer].gl, 0, sizeof lay[layer].gl);
    lay[layer].snd = (uint8_t)(k + 1);
}
void pt_clean_organ(int on)
{
    track_t *t=&trk[PT_DRONE];
    if(lay[PT_DRONE].snd!=26u)return;
    t->p[P_E1]=on?-8:6;t->p[P_E3]=on?-1:2;t->p[P_E5]=on?0:8;t->p[P_E6]=on?0:10;t->p[P_E7]=on?0:1;
}
int pt_is(int layer) { return (unsigned)layer < PT_NPART ? lay[layer].snd : 0; }

/* Drone chords sit high in the composer: give the new lower instruments
 * their intended register. Keyboard and sequence notes keep their written pitch. */
static int instrument_note(int layer, int note)
{
    if (layer == PT_DRONE && lay[layer].snd >= 27u) {
        int down = lay[layer].snd >= 29u ? 24 : 12;
        return note >= down ? note - down : 0;
    }
    return note;
}
void pt_note_on(int layer, int note, int vel, uint32_t gate)
{
    pt_layer_t *L;
    int i, f = -1;
    if ((unsigned)layer >= PT_NPART || !lay[layer].snd || note < 0 || note > 127)
        return;
    note = instrument_note(layer, note);
    L = &lay[layer];
    for (i = 0; i < NGATE; i++)                     /* (the same note again: its old gate gone) */
        if (L->gl[i] && L->gn[i] == note)
            L->gl[i] = 0;
    trk_note_on(&trk[layer], (uint32_t)note, (uint32_t)(vel < 1 ? 1 : vel > 127 ? 127 : vel));
    if (!gate)
        return;
    for (i = 0; i < NGATE; i++)
        if (!L->gl[i] && f < 0)
            f = i;
    if (f < 0) {                                    /* (every gate in use: the nearest to its end lets go now) */
        f = 0;
        for (i = 1; i < NGATE; i++)
            if (L->gl[i] < L->gl[f])
                f = i;
        trk_note_off(&trk[layer], L->gn[f]);
    }
    L->gn[f] = (uint8_t)note;
    L->gl[f] = gate;
}
void pt_note_off(int layer, int note)
{
    int i;
    if ((unsigned)layer >= PT_NPART || !lay[layer].snd)
        return;
    note = instrument_note(layer, note);
    for (i = 0; i < NGATE; i++)
        if (lay[layer].gl[i] && lay[layer].gn[i] == note)
            lay[layer].gl[i] = 0;
    trk_note_off(&trk[layer], (uint32_t)note);
}
void pt_notes_off(int layer)
{
    track_t *t;
    uint32_t i;
    if ((unsigned)layer >= PT_NPART)
        return;
    t = &trk[layer];
    memset(lay[layer].gl, 0, sizeof lay[layer].gl);
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].gate)
            trk_note_off(t, t->v[i].note);
    t->nmono = 0, t->mono_note = 0;
}
void pt_quiet(int layer)
{
    track_t *t;
    uint32_t i;
    if ((unsigned)layer >= PT_NPART)
        return;
    t = &trk[layer];
    memset(lay[layer].gl, 0, sizeof lay[layer].gl);
    for (i = 0; i < NVOICE; i++) {
        t->v[i].active = t->v[i].gate = 0;
        t->v[i].stage = 0;
        t->v[i].env = t->v[i].env_out = 0;
        t->v[i].pend = 0;
    }
    t->nmono = 0, t->mono_note = 0, t->xp_n = 0;
    lay[layer].pos = CTL;
    lay[layer].live = 0;
}

int pt_voices(void) { return (int)voices_busy(); }

static int block(int layer)                         /* the layer's next 32 samples into its buf; 0: none sounding */
{
    pt_layer_t *L = &lay[layer];
    track_t *t = &trk[layer];
    int32_t b[CTL];
    uint32_t i;
    float g;
    for (i = 0; i < NGATE; i++)                     /* the gates run down: their notes let go */
        if (L->gl[i]) {
            if (L->gl[i] > CTL)
                L->gl[i] -= CTL;
            else {
                L->gl[i] = 0;
                trk_note_off(t, L->gn[i]);
            }
        }
    engine_block(t);
    if (!track_render(t, b, CTL))
        return 0;
    /* LEVEL and the sound's trim (fx.c mix_part_run: LEVEL_Q12, Q12), Paleotone's full voice (+-32767 at Q15) at 1.0 */
    g = (float)LEVEL_Q12[t->p[P_LEVEL] ? clamp(t->p[P_LEVEL] + t->p[P_ED_FX], 1, 127) : 0] * (L->tg / (4096.0f * 32768.0f));
    for (i = 0; i < CTL; i++)
        L->buf[i] = (float)b[i] * g;
    return 1;
}

int pt_render(int layer, float *out, uint32_t n)
{
    pt_layer_t *L;
    int any = 0;
    uint32_t j = 0;
    if ((unsigned)layer >= PT_NPART || !lay[layer].snd)
        return 0;
    L = &lay[layer];
    while (j < n) {
        uint32_t k;
        if (L->pos >= CTL) {
            L->live = (uint8_t)block(layer);
            if (!L->live) {                         /* (nothing sounding this block: silence, the block's time passed) */
                uint32_t i;
                for (i = 0; i < CTL; i++)
                    L->buf[i] = 0.0f;
            }
            L->pos = 0;
        }
        any |= L->live;
        k = CTL - L->pos;
        if (k > n - j)
            k = n - j;
        while (k--)
            out[j++] = L->buf[L->pos++];
    }
    return any;
}

/* Reuse Paleotone's exact TRIBE lane models without allocating a fifth
 * general-purpose synth arena or consuming the eight melodic voices. */
static struct {
    dv_coef_t coef;
    dv_voice_t voice;
    float gain;
} skin[3];
static float skin_buf[CTL];
static uint32_t skin_pos = CTL;
void pt_skin_quiet(void)
{
    memset(skin, 0, sizeof skin);
    memset(skin_buf, 0, sizeof skin_buf);
    skin_pos = CTL;
}
void pt_skin_hit(int lane, int velocity, int tone, int decay)
{
    static const uint8_t role[3] = {DV_KICK, DV_SNARE, DV_TOM};
    dv_param_t p;
    uint32_t type;
    if ((unsigned)lane >= 3u) return;
    type = DV_KTYPE(DV_NKIT, role[lane]);
    dv_default(&p, type);
    p.tone = (uint8_t)clamp(tone * 127 / 100, 0, 127);
    p.decay = (uint8_t)clamp(decay * 127 / 100, 0, 127);
    dv_setup(&skin[lane].coef, &p);
    skin[lane].gain = (float)clamp(velocity, 1, 127) * (1.0f / (127.0f * 32768.0f));
    dv_trigger(&skin[lane].voice);
}
int pt_skin_render(float *out, uint32_t n)
{
    uint32_t j=0, i, l;
    int any=0;
    while (j<n) {
        if (skin_pos >= CTL) {
            int32_t y[CTL];
            for (i=0; i<CTL; i++) skin_buf[i]=0.0f;
            for (l=0; l<3; l++) {
                if (!skin[l].voice.live && !skin[l].voice.trig) continue;
                dv_run(&skin[l].coef, &skin[l].voice, 0, y, CTL);
                for (i=0; i<CTL; i++) skin_buf[i]+=(float)y[i]*skin[l].gain;
            }
            skin_pos=0;
        }
        out[j]=skin_buf[skin_pos++];
        if (out[j] != 0.0f) any=1;
        j++;
    }
    return any;
}
