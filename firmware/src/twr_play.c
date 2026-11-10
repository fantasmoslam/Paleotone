/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS: the conductor (the audio side). A preset is a small world: a key and a mode, a tempo, five of Paleotone's
 * sounds on the five synth parts and the ROAR rhythms on the drum track (the STAMPEDE kit, the skin drum of ROAR
 * without its hurdy-gurdy drone). The conductor plays it by itself, in the arc of a dinosynth piece, and you play the
 * LEAD over it on the keys (white keys: the mode, from C4 = the root) or from MIDI.
 *
 *   part 1 DRONE      the chord's root and fifth, held; the chord moves every four bars (the preset's PROG)
 *   part 2 OSTINATO   a figure of OLEN[0] sixteenths, made from the chord's tones
 *   part 3 BASS       a second figure of OLEN[1] sixteenths, low: the two loops drift against each other
 *   part 4 LEAD       yours (keys, MIDI in); the conductor never plays it (step 3: it will answer you)
 *   part 5 CALLS      a creature now and then, more often in the HUNT
 *   drums             five patterns from the preset's rhythms: INTRO, A, B, FILL, BREAK
 *   atmosphere        ambience.c (AMB, CRKL): wind and crackle under everything, no voice
 *
 * The arc, in bars: EMERGE 8 (the drone, the drums late and sparse), WALK 12 or 16 (the ostinato, the bass in its
 * second half, drums A: the high drum in its second half), HUNT 16 or 24 (everything, drums B with the talking drum,
 * ghost strokes and more notes as it builds, the calls; its end now and then an octave up), BRIDGE 4 or 8 (the
 * ostinato thin, the drums a BREAK; half the time hollow: the drone alone, or nothing), now and then skipped; then WALK
 * again. A section begins on the deep drum and a far thunder. Each pass the figures change a little more (one step,
 * up to four), and the chords too: now and then a substitute a third up, the last chord hanging on, or the drone on a
 * fourth (suspended).
 *
 * Every note the conductor plays goes to MIDI OUT as the sequencer's do (GLO > SYSTEM > OUT = SEQ), on each
 * track's channel (GLO > MIDI): seq_out_on / seq_out_off. Included by seq.c before events_block, which calls
 * twr_block once a block. Nothing here reads flash or the screen. */

enum { TW_DRONE, TW_OST, TW_BASS, TW_LEAD, TW_CALLS, TW_LAYERS };
enum { TW_EMERGE, TW_WALK, TW_HUNT, TW_BRIDGE, TW_SECTIONS };
enum { TD_NONE, TD_INTRO, TD_A, TD_B, TD_BREAK };
static const uint8_t TW_SEC_BARS[TW_SECTIONS] = {8, 16, 16, 8};
static const char *const TW_SEC_NAME[TW_SECTIONS] = {"EMERGE", "WALK", "HUNT", "BRIDGE"};

/* the ROAR rhythms (eng_gurdy.c GURDY_RHY), by index */
enum { R_GALOP, R_TRES, R_WAR, R_RAIN, R_QUAKE, R_WAR2, R_TALK, R_SEVEN, R_PENTA, R_SPIRL, R_CHAOS, R_N };
_Static_assert(sizeof GURDY_RHY / sizeof GURDY_RHY[0] == R_N, "the ROAR rhythms");
enum { SC_MIN = 2, SC_DOR = 3, SC_HARM = 7, SC_PHRY = 8, SC_LOC = 10 };   /* seq.c SCALE_MASK */
enum { N_C, N_CS, N_D, N_DS, N_E, N_F, N_FS, N_G, N_GS, N_A, N_AS, N_B };

typedef struct {
    const char *name;
    const char *snd[TW_LAYERS];        /* Paleotone's sounds, by name (ui.c BANK): drone, ostinato, bass, lead, calls */
    uint8_t root, scale, bpm;
    uint8_t olen[2];                   /* the ostinato's and the bass's loops, in sixteenths (unequal: they drift) */
    uint8_t rhy[3];                    /* drums: A, B and the fill's rhythm */
    uint8_t amb, crkl;                 /* the atmosphere (ambience.c) */
    int8_t prog[4];                    /* the chord's root, a scale degree, four bars each */
} twr_preset_t;

static const twr_preset_t TWR_PRESETS[] = {
    {"EXTINCTION", {"TEMPLE ORGAN", "TUSK TINES", "SUB BASS", "MAMMOTH", "TYRANT ROAR"},
     N_D, SC_PHRY, 52, {16, 15}, {R_QUAKE, R_WAR, R_SPIRL}, 70, 20, {0, 1, 0, -2}},
    {"STAMPEDE", {"EPIC STRINGS", "VALLEY HARP", "STOMP BASS", "TITAN BRASS", "HERD MOAN"},
     N_A, SC_HARM, 92, {16, 15}, {R_GALOP, R_TRES, R_WAR2}, 50, 10, {0, 5, 3, 4}},
    {"RAPTOR NIGHT", {"NIGHT WAIL", "KALIMBA", "SWAMP BASS", "FOSSIL LEAD", "RAPTOR CHAT"},
     N_CS, SC_PHRY, 84, {16, 15}, {R_TALK, R_SEVEN, R_SPIRL}, 60, 60, {0, -2, 0, 1}},
};
#define TWR_NPRESETS (sizeof TWR_PRESETS / sizeof TWR_PRESETS[0])
/* the mix a preset starts with (P_LEVEL, 104 = 0 dB): the drone under everything, the lead on top */
static const uint8_t TW_LEVEL[TW_LAYERS] = {80, 100, 98, 104, 94};
#define TW_DRUM_LEVEL 84                           /* G_DRLVL */

#define TW_OMAX 16u
#define TW_NOFF 24u
static struct {
    const twr_preset_t *p;             /* the world playing (0: the conductor is off) */
    volatile uint8_t req;              /* the UI: preset req - 1 from the next block (0 none) */
    volatile uint8_t k[4];             /* the UI's knobs: DENSITY, DRUMS, CALLS (AMB is the song's) */
    volatile uint8_t sec, sbar;        /* where the arc is (the screen reads it) */
    volatile uint8_t chord;            /* the chord's place in the PROG now */
    int8_t cdeg;                       /* .. the degree it plays (a substitute now and then) */
    uint8_t sus;                       /* the drone's upper note a fourth, not a fifth: the chord hangs */
    volatile uint8_t slen;             /* the section's length this time, in bars */
    uint8_t hollow;                    /* this BRIDGE thins out to the drone alone */
    uint8_t thunder;                   /* a section just began: the deep drums on its downbeat */
    volatile uint8_t pat;              /* the drum pattern now (TD_*), 5 = the fill */
    uint8_t mask_cnt, mask[12];        /* the mode's notes, from the root */
    int8_t ost[2][TW_OMAX];            /* the figures: a chord tone index 0..5 (+ 8: accent), -1 a rest */
    uint8_t drone_on[2], drone_n;      /* the drone's notes sounding */
    uint32_t s16;                      /* sixteenths since PLAY */
    uint32_t base16;                   /* the clock's sixteenth at PLAY */
    uint32_t bar;                      /* bars since PLAY */
    uint32_t pass;                     /* WALKs so far (the figures change each one) */
    uint32_t seed;
    uint8_t was_playing;
    struct { uint8_t t, note; uint32_t left; } off[TW_NOFF];
} twr = {.k = {64, 100, 64, 0}, .seed = 0x5EED};

static uint32_t tw_rand(uint32_t salt) { return gurdy_hash(twr.seed ^ (twr.s16 * 2654435761u) ^ salt); }

/* the note of scale degree d (any, negative too) above the root at octave base (a MIDI note of C) */
static uint32_t tw_degree(int32_t d, int32_t base)
{
    int32_t n = (int32_t)twr.mask_cnt ? (int32_t)twr.mask_cnt : 7, o = d >= 0 ? d / n : -((-d + n - 1) / n);
    int32_t i = d - o * n;
    return (uint32_t)clamp(base + (int32_t)twr.p->root + 12 * o + twr.mask[i], 0, 127);
}

static uint32_t twr_cnt[NTRK];                     /* notes played, each track (the screen's lights, the host test) */
static void tw_on(uint32_t ti, uint32_t note, uint32_t vel, uint32_t len)
{
    track_t *t = &trk[ti];
    uint32_t i;
    if (!vel)
        return;
    twr_cnt[ti % NTRK]++;
    for (i = 0; i < TW_NOFF; i++)                   /* (the same note still on: off first) */
        if (twr.off[i].left && twr.off[i].t == ti && twr.off[i].note == note) {
            twr.off[i].left = 0;
            if (!is_drum(t))
                trk_note_off(t, note);
            seq_out_off(t, note);
        }
    trk_note_on(t, note, vel);
    seq_out_on(t, note, vel);
    for (i = 0; i < TW_NOFF; i++)
        if (!twr.off[i].left) {
            twr.off[i].t = (uint8_t)ti;
            twr.off[i].note = (uint8_t)note;
            twr.off[i].left = len ? len : 1u;
            return;
        }
    if (!is_drum(t))                                /* (no room: a short note) */
        trk_note_off(t, note);
    seq_out_off(t, note);
}

static void tw_offs(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < TW_NOFF; i++) {
        if (!twr.off[i].left)
            continue;
        if (twr.off[i].left > n) {
            twr.off[i].left -= n;
            continue;
        }
        twr.off[i].left = 0;
        if (!is_drum(&trk[twr.off[i].t]))
            trk_note_off(&trk[twr.off[i].t], twr.off[i].note);
        seq_out_off(&trk[twr.off[i].t], twr.off[i].note);
    }
}

static void tw_drone_off(void)
{
    uint32_t i;
    for (i = 0; i < twr.drone_n; i++) {
        trk_note_off(&trk[TW_DRONE], twr.drone_on[i]);
        seq_out_off(&trk[TW_DRONE], twr.drone_on[i]);
    }
    twr.drone_n = 0;
}

static void tw_all_off(void)
{
    uint32_t i;
    tw_offs(0xFFFFFFFFu);
    tw_drone_off();
    for (i = 0; i < TW_LAYERS; i++)
        if (i != TW_LEAD)
            seq_out_track_off(&trk[i]);
}

/* a figure: chord tones (0 root, 1 third, 2 fifth, 3 the octave, 4 the octave's third, 5 the second), rests;
 * the first step always the root, accented */
static void tw_figure(uint32_t f, uint32_t salt)
{
    uint32_t i, len = twr.p->olen[f] % (TW_OMAX + 1u);
    static const uint8_t TONE_W[2][6] = {{4, 2, 3, 3, 1, 1}, {6, 0, 4, 2, 0, 0}};   /* how likely each tone */
    for (i = 0; i < len; i++) {
        uint32_t h = gurdy_hash(salt * 131u + i * 7919u + f), w = 0, k, pick;
        if (i == 0u) {
            twr.ost[f][0] = 8;
            continue;
        }
        if ((h & 255u) < (f ? 150u : 96u)) {        /* rests: the bass more of them */
            twr.ost[f][i] = -1;
            continue;
        }
        for (k = 0; k < 6u; k++)
            w += TONE_W[f][k];
        pick = (h >> 8) % w;
        for (k = 0; k < 5u && pick >= TONE_W[f][k]; k++)
            pick -= TONE_W[f][k];
        twr.ost[f][i] = (int8_t)(k | ((h >> 20) % 5u == 0u ? 8u : 0u));
    }
}

/* a figure changes a little: one step (two in the HUNT's pass), never the first */
static void tw_mutate(uint32_t f)
{
    uint32_t len = twr.p->olen[f], h = tw_rand(f + 77u), s = 1u + (h % (len - 1u));
    int8_t v = twr.ost[f][s];
    twr.ost[f][s] = v < 0 ? (int8_t)((h >> 12) % 4u) : (h >> 16) & 1u ? -1 : (int8_t)((v & 7) == 2 ? 3 : 2);
}

static void tw_mutates(uint32_t f, uint32_t n)   /* n steps change (the later passes more) */
{
    while (n--) {
        twr.s16 += 7u;                              /* (another hash each) */
        tw_mutate(f);
        twr.s16 -= 7u;
    }
}

static void tw_start_preset(uint32_t i)
{
    uint32_t k, m;
    tw_all_off();
    twr.p = &TWR_PRESETS[i % TWR_NPRESETS];
    m = SCALE_MASK[twr.p->scale % NSCALES];
    twr.mask_cnt = 0;
    for (k = 0; k < 12u; k++)
        if ((m >> k) & 1u)
            twr.mask[twr.mask_cnt++] = (uint8_t)k;
    twr.seed = gurdy_hash(i * 0x9E37u + 1u);
    tw_figure(0, twr.seed);
    tw_figure(1, twr.seed ^ 0xB055u);
    twr.s16 = 0;
    twr.bar = 0;
    twr.pass = 0;
    twr.sec = TW_EMERGE;
    twr.sbar = 0;
    twr.slen = TW_SEC_BARS[TW_EMERGE];
    twr.chord = 0;
    twr.cdeg = twr.p->prog[0];
    twr.sus = 0;
}

/* the chord's tone k (tw_figure) at octave base */
static uint32_t tw_tone(uint32_t k, int32_t base)
{
    static const int8_t DEG[6] = {0, 2, 4, 7, 9, 1};
    return tw_degree((int32_t)twr.cdeg + DEG[k % 6u], base);
}

static void tw_chord(void)                         /* the bar's chord: the drone follows */
{
    track_t *t = &trk[TW_DRONE];
    uint32_t c = (twr.bar / 4u) % 4u, r, f, h;
    if (twr.drone_n && c == twr.chord)
        return;
    twr.chord = (uint8_t)c;
    h = tw_rand(0xC0u + c);
    twr.cdeg = twr.p->prog[c];
    twr.sus = 0;
    if (twr.pass && c && (h & 7u) == 0u)
        twr.cdeg = (int8_t)(twr.cdeg + 2);          /* now and then a substitute: the chord a third up (shares two notes) */
    else if (twr.pass && (h & 7u) == 1u)
        twr.cdeg = twr.p->prog[(c + 3u) % 4u];      /* .. or the last chord hangs on */
    if (twr.sec != TW_HUNT && ((h >> 8) & 7u) == 0u)
        twr.sus = 1;                                /* .. or it hangs on a fourth, unresolved */
    tw_drone_off();
    if (twr.sec == TW_BRIDGE && twr.hollow && (h >> 12) & 1u)
        return;                                     /* (a hollow BRIDGE: sometimes not even the drone) */
    r = tw_tone(0, 36);
    if (r < 43u)
        r += 12u;                                   /* (G2 .. F#3: low, not mud) */
    f = twr.sus ? tw_degree((int32_t)twr.cdeg + 3, 36) : tw_tone(2, 36);   /* the fifth (the fourth), just above */
    while (f <= r)
        f += 12u;
    while (f > r + 12u)
        f -= 12u;
    trk_note_on(t, r, 84);
    seq_out_on(t, r, 84);
    twr_cnt[TW_DRONE]++;
    twr.drone_on[twr.drone_n++] = (uint8_t)r;
    if (t->p[P_VOICE] == V_POLY) {                  /* (a mono sound: the root alone) */
        trk_note_on(t, f, 70);
        seq_out_on(t, f, 70);
        twr.drone_on[twr.drone_n++] = (uint8_t)f;
    }
}

/* a section's length this time: EMERGE 8; WALK 12 or 16; HUNT 16, sometimes 24 once it has been heard; BRIDGE 4 or 8 */
static uint32_t tw_sec_len(uint32_t k)
{
    uint32_t h = tw_rand(0xA4Cu + k);
    switch (k) {
    case TW_EMERGE: return 8u;
    case TW_WALK: return (h & 1u) ? 12u : 16u;
    case TW_HUNT: return twr.pass >= 2u && (h & 3u) == 0u ? 24u : 16u;
    default: return (h & 1u) ? 4u : 8u;
    }
}

static void tw_bar(void)                           /* a new bar: the arc */
{
    if (twr.bar == 0u) {
        twr.sec = TW_EMERGE;
        twr.sbar = 0;
        twr.slen = (uint8_t)tw_sec_len(TW_EMERGE);
        twr.hollow = 0;
        twr.thunder = 0;
    } else if (++twr.sbar >= twr.slen) {            /* the next section */
        uint32_t k = twr.sec == TW_BRIDGE || twr.sec == TW_EMERGE ? TW_WALK : twr.sec + 1u, h = tw_rand(0xB1u);
        if (k == TW_BRIDGE && twr.pass >= 1u && (h & 3u) == 0u)
            k = TW_WALK;                            /* (now and then no BRIDGE: the HUNT turns straight back) */
        if (k == TW_WALK) {                         /* a new pass: the figures change, the later ones more */
            if (twr.sec != TW_EMERGE)
                twr.pass++;
            tw_mutates(0, 1u + (twr.pass < 3u ? twr.pass : 3u));
            tw_mutates(1, (twr.pass + 1u) / 2u < 2u ? (twr.pass + 1u) / 2u : 2u);
        }
        twr.hollow = (uint8_t)(k == TW_BRIDGE && ((h >> 8) & 1u));
        twr.thunder = (uint8_t)(k == TW_HUNT || k == TW_WALK);
        twr.sec = (uint8_t)k;
        twr.sbar = 0;
        twr.slen = (uint8_t)tw_sec_len(k);
    }
    if (twr.sec == TW_HUNT && twr.sbar == twr.slen / 2u)
        tw_mutate(0);                               /* (the HUNT's second half: the figure turns) */
    tw_chord();
}

static uint32_t tw_s16_len(void) { return BEAT_U / 4u / (uint32_t)(song.g[G_BPM] ? song.g[G_BPM] : 1); }   /* samples */

/* the drums this sixteenth: the pattern of the section, the fill in the last beat of every fourth bar */
static void tw_drums(uint32_t s)
{
    static const uint8_t NOTE[5] = {0, 35, 36, 48, 63};          /* soft low, loud low, the fifth up, the talking drum */
    static const uint8_t VEL[5] = {0, 66, 112, 92, 100};
    uint32_t pat, r, v, len, pos, h = tw_rand(0xD5u), lvl = twr.k[1], bar16 = s % 16u, fill;
    if (!lvl)
        return;
    switch (twr.sec) {
    case TW_EMERGE: pat = twr.sbar >= 4u ? TD_INTRO : TD_NONE; break;
    case TW_WALK: pat = TD_A; break;
    case TW_HUNT: pat = TD_B; break;
    default: pat = TD_BREAK; break;
    }
    fill = pat >= TD_A && (twr.sbar & 3u) == 3u && bar16 >= 12u;
    twr.pat = (uint8_t)(fill ? 5u : pat);
    if (twr.thunder && bar16 == 0u) {               /* a section begins: the deep drum and the far thunder */
        twr.thunder = 0;
        tw_on(TRK_DRUM, 43, (uint32_t)clamp(118 * (int32_t)lvl / 100, 1, 127), tw_s16_len() * 8u);
        tw_on(TRK_DRUM, 49, (uint32_t)clamp(96 * (int32_t)lvl / 100, 1, 127), tw_s16_len() * 16u);
    }
    if (pat == TD_NONE)
        return;
    r = fill ? twr.p->rhy[2] : pat == TD_B ? twr.p->rhy[1] : twr.p->rhy[0];
    len = GURDY_RHY[r].len;
    pos = fill ? (GURDY_RHY[r].len - 16u + bar16) % len : s % len;   /* (the fill: its rhythm's last beat) */
    v = GURDY_RHY[r].v[pos];
    if (r == R_CHAOS && !v && (h >> 28) < 6u)        /* CHAOS: the rest by chance (as ROAR plays it) */
        v = (h >> 28) < 3u ? 1u : (h >> 28) < 5u ? 3u : 4u;
    if (pat == TD_INTRO && v != 2u)
        v = 0;                                       /* the intro: the low drum's loud strokes only */
    if (pat == TD_BREAK) {                           /* the break: the soft strokes, every other bar, far */
        if ((twr.sbar & 1u) || v == 0u)
            return;
        v = 1u;
    }
    if (pat == TD_A && v == 3u && twr.sbar < twr.slen / 2u)
        v = 1u;                                      /* A: the high drum only in the WALK's second half */
    if (pat == TD_A && v == 4u)
        v = 2u;                                      /* .. the talking drum is the HUNT's */
    if (pat == TD_B && !v && twr.sbar >= twr.slen / 2u && ((h >> 4) & 3u) == 0u)
        v = 1u;                                      /* B, its second half: ghost strokes between (the build) */
    if (fill && !v && (h & 3u) == 0u)
        v = 3u;                                      /* (a fill fills) */
    if (!v || v > 4u)
        return;
    {
        int32_t vel = VEL[v] + (int32_t)((h >> 8) & 15u) - 8;
        if (bar16 == 0u && v == 2u)
            vel += 10;                               /* the downbeat */
        if (pat == TD_INTRO || pat == TD_BREAK)
            vel -= 24;
        if (pat == TD_B && v == 1u && !GURDY_RHY[r].v[pos])
            vel -= 22;                               /* (the ghosts: soft) */
        vel = vel * (int32_t)lvl / 100;
        tw_on(TRK_DRUM, NOTE[v], (uint32_t)clamp(vel, 1, 127), tw_s16_len());
        if (v == 2u && pat == TD_B && bar16 == 0u && (twr.sbar & 1u) == 0u)
            tw_on(TRK_DRUM, 37, (uint32_t)clamp(vel - 30, 1, 127), tw_s16_len());   /* the hunt: a slap on it */
    }
}

static void tw_step(void)                          /* one sixteenth */
{
    uint32_t s = twr.s16, q = tw_s16_len(), dens = twr.k[0], h;
    int32_t obase = 60;
    if (s % 16u == 0u) {
        twr.bar = s / 16u;
        tw_bar();
    }
    tw_drums(s);
    if (twr.sec == TW_HUNT)
        dens += twr.sbar * 48u / (twr.slen ? twr.slen : 16u);   /* the HUNT builds */
    if (twr.sec == TW_HUNT && twr.sbar >= twr.slen - twr.slen / 4u && (twr.pass & 1u))
        obase = 72;                                  /* .. its last quarter, every other pass, an octave up */
    else if (twr.sec == TW_BRIDGE && (twr.pass & 2u))
        obase = 48;                                  /* (a BRIDGE now and then low) */
    /* the ostinato: WALK, HUNT; the BRIDGE every other note; EMERGE's last two bars come in soft */
    if ((twr.sec != TW_EMERGE || twr.sbar >= 6u) && !(twr.sec == TW_BRIDGE && twr.hollow)) {
        int8_t v = twr.ost[0][s % twr.p->olen[0]];
        h = tw_rand(0x05u);
        if (v >= 0 && (h & 127u) < dens + 40u && !(twr.sec == TW_BRIDGE && (s & 1u)) &&
            !(twr.sec == TW_EMERGE && (s & 3u))) {
            uint32_t vel = (v & 8) ? 96u : 72u;
            if (twr.sec == TW_EMERGE || twr.sec == TW_BRIDGE)
                vel -= 20u;
            tw_on(TW_OST, tw_tone((uint32_t)v & 7u, obase), vel + ((h >> 8) & 7u), q * 3u / 2u);
        }
    }
    /* the bass: the WALK's second half, the HUNT */
    if ((twr.sec == TW_WALK && twr.sbar >= 8u) || twr.sec == TW_HUNT) {
        int8_t v = twr.ost[1][s % twr.p->olen[1]];
        h = tw_rand(0x0Bu);
        if (v >= 0 && (h & 127u) < dens + 50u)
            tw_on(TW_BASS, tw_tone((uint32_t)v & 7u, 36), (v & 8) ? 104u : 84u, q * 2u);
    }
    /* the calls: a creature on an offbeat now and then; the HUNT more */
    if ((s & 3u) == 2u) {
        uint32_t odds = twr.k[2] * (twr.sec == TW_HUNT ? 3u : twr.sec == TW_EMERGE ? 1u : 2u) / 3u;   /* /1024 */
        h = tw_rand(0xCA11u);
        if ((h & 1023u) < odds)
            tw_on(TW_CALLS, tw_tone((h >> 12) % 3u * 2u % 6u, 48), 70u + ((h >> 20) & 31u), q * 8u);
    }
    twr.s16++;
}

static void twr_block(uint32_t n)                  /* events_block, once a block */
{
    uint32_t r = twr.req;
    if (r) {
        twr.req = 0;
        tw_start_preset(r - 1u);
    }
    if (!twr.p)
        return;
    tw_offs(n);
    if (!song.playing) {
        if (twr.was_playing)
            tw_all_off();                            /* STOP: everything the conductor started ends */
        twr.was_playing = 0;
        return;
    }
    {   /* the sixteenths the clock has passed since PLAY */
        uint32_t now = clk_beat * 4u + (uint32_t)((uint64_t)clk_pos * 4u / BEAT_U), guard = 4u;
        if (!twr.was_playing) {                      /* PLAY: the arc from the top */
            twr.was_playing = 1;
            twr.base16 = now;
            twr.s16 = 0;
            twr.pass = 0;
            twr.drone_n = 0;
        }
        while (twr.s16 <= now - twr.base16 && guard--)
            tw_step();
    }
}

/* a sound by its name (ui.c BANK's names): engine *e, preset *k; 0 found */
static int twr_find(const char *name, uint32_t *e, uint32_t *k)
{
    uint32_t i, j;
    for (i = 0; i < NENGINES; i++)
        for (j = 0; j < ENGINES[i]->npresets; j++)
            if (str_eq(ENGINES[i]->presets[j].name, name)) {
                *e = i;
                *k = j;
                return 0;
            }
    return 1;
}
static uint32_t twr_kit(void)                      /* the STAMPEDE kit (tools/gen_drumkits.py), the drum track's P_E0 */
{
    uint32_t i;
    for (i = 0; i < DS_NKITS; i++)
        if (str_eq(DS_KITS[i].name, "STAMPEDE"))
            return DRUM_SAMPLED + i;
    return DRUM_DEFAULT_KIT;
}
