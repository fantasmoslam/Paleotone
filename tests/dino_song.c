/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX demo song, "TITAN RISING": composed in the synth itself through the firmware's own code. Four project slots
 * (A..D) hold the sections' sequences; the SONG chains them, each track on its own slot per section (lanes), with
 * repeats; the sounds stay the current ones, as on the FM-1. 70 BPM, D minor: a stomping low brass riff, a choir
 * singing the hook, brachiosaurs, raptors and a tyrant on one SAURUS track, the TRIBE kit in half time.
 *   T1 FM6 MAMMOTH      the riff                   T2 VOICE EPIC CHOIR  the hook, sung in chords
 *   T3 SAURUS           the creatures (automation)  T4 DRUM WAR DRUMS    the beat, half time
 *   build/host/dino_song OUT.wav [FRAMES.rgb]   (the screen at 30 fps, 240x240 RGB24) Not a test. */
#define UI_RENDER_NO_MAIN
#include "ui_render.c"

#define T_ 255u
#define MAXS 64u
#define ACC 0x100u                                     /* (in a step's notes list: this step accented) */
#define R2 0x200u                                      /* .. ratcheted x2, x3 */
#define R3 0x400u
typedef struct { uint16_t n[MAXS][4]; } steps_t;

static uint32_t preset_of(uint32_t e, const char *name)
{
    uint32_t i;
    for (i = 0; i < ENGINES[e]->npresets; i++)
        if (!strcmp(ENGINES[e]->presets[i].name, name))
            return i;
    fprintf(stderr, "no preset %s\n", name);
    exit(1);
}
/* a track's steps: LEN steps of DIV, notes (T_ a tie, 0 a rest), the first note's high bits the step's flags */
static void steps(uint32_t k, uint32_t div, uint32_t len, const steps_t *st)
{
    track_t *t = &trk[k];
    uint32_t i, n;
    memset(t->step, 0, sizeof t->step);
    for (i = 0; i < NSTEP; i++)
        t->step[i].time = ST_REST;
    t->p[P_SDIV] = (int16_t)div;
    t->p[P_SLEN] = (int16_t)len;
    t->p[P_SGATE] = 127;
    for (i = 0; st && i < len; i++) {
        uint8_t nt[4] = {0};
        uint16_t f = st->n[i][0] & 0xFF00u;
        for (n = 0; n < 4u && (st->n[i][n] & 0xFFu) && (st->n[i][n] & 0xFFu) != T_; n++)
            nt[n] = (uint8_t)st->n[i][n];
        if ((st->n[i][0] & 0xFFu) == T_)
            put_step(t, i, 0, nt, ST_TIE, 0);
        else
            put_step(t, i, n, nt, n ? ST_NOTE : ST_REST,
                     ((f & ACC) ? SF_ACCENT : 0u) | ((f & R2) ? 1u << SF_RATCH_SH : 0u) | ((f & R3) ? 2u << SF_RATCH_SH : 0u));
        if (n && (f & ACC))
            t->step[i].vel = 127;
    }
}

/* ---------------------------------------------------------------- the music, D minor */
#define TIE {T_}
/* T1 MAMMOTH, 1/8, 16 steps (2 bars): the stomping riff */
static const steps_t R_A = {{{38 | ACC}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, {34 | ACC}, TIE, TIE, TIE, {33 | ACC}, TIE, TIE, TIE}};
static const steps_t R_B = {{{38 | ACC}, {0}, {38}, {38}, {41 | ACC}, {0}, {40}, {0},
                             {38 | ACC}, {0}, {38}, {38}, {33 | ACC}, {0}, {36}, {34}}};
static const steps_t R_C = {{{38 | ACC, 45}, {0}, {38, 45}, {38, 45}, {41 | ACC, 48}, {0}, {40, 47}, {0},
                             {38 | ACC, 45}, {0}, {38, 45}, {38, 45}, {36 | ACC, 43}, {36, 43}, {34, 41}, {33, 40}}};
static const steps_t R_D = {{{38}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE}};
/* T4 TRIBAL KIT, 1/8, 16 steps (2 bars), half time: the boom on 1 and the "and" of 2, the skin on 3, the log drum
 * answering; GM notes: 36 BOOM, 38 SKIN, 39 CLAP, 42 SEEDS, 46 RATTL, 45 LOG, 37 BONE, 56 ROCK */
#define AT(i, ...) [i] = {__VA_ARGS__}
static const steps_t D_A = {{AT(0, 36 | ACC), AT(8, 36), AT(12, 45), AT(14, 45)}};
static const steps_t D_B = {{AT(0, 36 | ACC, 56), AT(3, 36), AT(4, 38 | ACC), AT(6, 45), AT(8, 36 | ACC), AT(11, 36),
                             AT(12, 38 | ACC), AT(14, 45), AT(15, 45 | R2)}};
static const steps_t D_C = {{AT(0, 36 | ACC, 56), AT(2, 46), AT(3, 36), AT(4, 38 | ACC, 39), AT(6, 45, 46), AT(8, 36 | ACC),
                             AT(10, 46), AT(11, 36), AT(12, 38 | ACC, 39), AT(13, 45), AT(14, 45 | R2), AT(15, 38 | ACC | R3)}};
static const steps_t D_D = {{AT(0, 36 | ACC, 56), AT(12, 45), AT(14, 45 | R2)}};

/* T2 EPIC CHOIR: the hook sung, chords under its top voice (the two chord tones just below the melody), 1/8, 32 steps
 * (4 bars), a chord a bar; A and D sustained chords */
static const uint8_t CH_DM[3] = {2, 5, 9}, CH_BB[3] = {10, 2, 5}, CH_C[3] = {0, 4, 7}, CH_A[3] = {9, 1, 4}, CH_GM[3] = {7, 10, 2};
static const uint8_t *const BAR_B[4] = {CH_DM, CH_BB, CH_C, CH_A}, *const BAR_C[4] = {CH_DM, CH_GM, CH_BB, CH_A};
static const uint8_t HOOK_B[32] = {69, T_, T_, 74, T_, 72, T_, 69,   70, T_, 69, 67, T_, 65, T_, T_,
                                   69, T_, T_, 74, T_, 76, T_, 77,   76, T_, 74, 73, T_, 74, T_, T_};
static const uint8_t HOOK_C[32] = {74, T_, 72, 74, T_, 77, T_, 76,   74, T_, 72, 70, T_, 69, T_, T_,
                                   74, T_, 77, T_, 79, T_, 81, T_,   79, T_, 77, 76, T_, 74, T_, T_};
static void choir_hook(const uint8_t *mel, const uint8_t *const *bar)
{
    track_t *t = &trk[1];
    uint32_t i, j, c;
    memset(t->step, 0, sizeof t->step);
    for (i = 0; i < NSTEP; i++)
        t->step[i].time = ST_REST;
    t->p[P_SDIV] = 1;
    t->p[P_SLEN] = 32;
    t->p[P_SGATE] = 127;
    for (i = 0; i < 32u; i++) {
        uint8_t nt[3];
        if (mel[i] == T_) {
            put_step(t, i, 0, nt, ST_TIE, 0);
            continue;
        }
        nt[0] = mel[i];
        for (j = 1, c = mel[i] - 1u; j < 3u && c > 40u; c--) {   /* the next chord tones down */
            const uint8_t *ch = bar[i / 8u];
            if (c % 12u == ch[0] || c % 12u == ch[1] || c % 12u == ch[2])
                nt[j++] = (uint8_t)c;
        }
        put_step(t, i, 3, nt, ST_NOTE, i % 8u ? 0u : SF_ACCENT);
    }
}
#define DM {50, 53, 57}
#define BB {50, 53, 58}
#define AM {49, 52, 57}
static const steps_t C_A = {{DM, TIE, TIE, TIE, TIE, TIE, TIE, TIE, BB, TIE, TIE, TIE, AM, TIE, TIE, TIE}};
static const steps_t C_D = {{{50, 57, 62}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE,
                             {49, 57, 64}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE}};

/* T3 SAURUS: the creatures. One track, its call changed by automation on the step it sounds (CALL SIZE SWOOP LEN GROWL
 * BRTH MOUTH WILD and the envelope, from the factory creatures): the brachiosaur's bellow, the raptors' barks
 * answering the riff, the tyrant's roar */
enum { BRACHIO, RAPTOR, TYRANT };
static const int8_t CREATURE[3][12] = {
    {1, 120, 42, 108, 20, 35, 90, 25, 40, 90, 55, 90},     /* BRACHIO CALL (held softer: far off) */
    {3, 35, 55, 82, 80, 60, 120, 50, 0, 70, 127, 40},      /* RAPTOR BARK */
    {2, 105, 58, 102, 100, 45, 127, 30, 4, 90, 90, 80},    /* TYRANT ROAR (its body under the raptors' level) */
};
typedef struct { uint8_t step, who, note, len; } cry_t;
static void creatures(const cry_t *c, uint32_t n)
{
    track_t *t = &trk[2];
    uint32_t i, j;
    memset(t->step, 0, sizeof t->step);
    for (i = 0; i < NSTEP; i++)
        t->step[i].time = ST_REST;
    t->p[P_SDIV] = 1;
    t->p[P_SLEN] = 32;
    t->p[P_SGATE] = 127;
    motion_clear(t);
    for (i = 0; i < n; i++) {
        uint8_t nt = c[i].note;
        put_step(t, c[i].step, 1, &nt, ST_NOTE, SF_ACCENT);
        t->step[c[i].step].vel = 127;
        for (j = 1; j < c[i].len; j++)
            put_step(t, c[i].step + j, 0, &nt, ST_TIE, 0);
        for (j = 0; j < 8u; j++)
            motion_set_event(t, c[i].step, P_E0 + j, CREATURE[c[i].who][j]);
        for (j = 0; j < 4u; j++)
            motion_set_event(t, c[i].step, P_ATK + j, CREATURE[c[i].who][8 + j]);
    }
}
/* A: two brachiosaurs far across the valley; B: raptors bark in the riff's gaps; C: the tyrant roars at the top,
 * raptors after it; D: a brachiosaur, then the tyrant roars into the return */
static const cry_t K_A[] = {{0, BRACHIO, 45, 12}, {18, BRACHIO, 41, 12}};
static const cry_t K_B[] = {{7, RAPTOR, 64, 1}, {15, RAPTOR, 62, 1}, {23, RAPTOR, 65, 1}, {30, RAPTOR, 64, 1}, {31, RAPTOR, 67, 1}};
static const cry_t K_C[] = {{0, TYRANT, 45, 10}, {13, RAPTOR, 66, 1}, {14, RAPTOR, 64, 1}, {27, RAPTOR, 67, 1},
                            {29, RAPTOR, 65, 1}, {30, RAPTOR, 64, 1}};
static const cry_t K_D[] = {{0, BRACHIO, 45, 14}, {22, TYRANT, 43, 10}};

/* the sounds (one set for the whole song), their mix and effects: a giant's valley */
static void sounds(void)
{
    host_preset(&trk[0], ENGI_FM6, preset_of(ENGI_FM6, "MAMMOTH"));
    host_preset(&trk[1], 5, preset_of(5, "EPIC CHOIR"));
    host_preset(&trk[2], ENGI_SAURUS, preset_of(ENGI_SAURUS, "RAPTOR BARK"));
    host_preset(&trk[3], ENGI_DRUM, preset_of(ENGI_DRUM, "WAR DRUMS"));
    trk[1].p[P_ATK] = 45;                                   /* the choir sings the hook: a quicker attack */
    trk[1].p[P_REL] = 90;
    trk[2].p[P_VOICE] = V_POLY;                             /* (a call rings on under the next) */
    /* levels (balanced by meter: the riff and the choir lead, the drums under them, the creatures on top), pan */
    trk[0].p[P_LEVEL] = 116; trk[0].p[P_PAN] = 0;
    trk[1].p[P_LEVEL] = 79;  trk[1].p[P_PAN] = 0;   trk[1].p[P_SPRD] = 90;
    trk[2].p[P_LEVEL] = 102; trk[2].p[P_PAN] = 10;
    trk[3].p[P_LEVEL] = 96;  trk[3].p[P_PAN] = 0;
    /* sends: DIST CHORUS DELAY REVERB */
    trk[0].p[P_DIST] = 18; trk[0].p[P_CHOR] = 0;  trk[0].p[P_DLY] = 0;  trk[0].p[P_REV] = 55;
    trk[1].p[P_DIST] = 0;  trk[1].p[P_CHOR] = 70; trk[1].p[P_DLY] = 18; trk[1].p[P_REV] = 72;
    trk[2].p[P_DIST] = 10; trk[2].p[P_CHOR] = 0;  trk[2].p[P_DLY] = 45; trk[2].p[P_REV] = 110;
    trk[3].p[P_DIST] = 12; trk[3].p[P_CHOR] = 0;  trk[3].p[P_DLY] = 0;  trk[3].p[P_REV] = 80;
#ifdef DS_SOLO
    song.master_q12 = 1024;                                 /* (under the limiter: a track's own level) */
    for (uint32_t k = 0; k < NTRK; k++)
        if (k != DS_SOLO)
            trk[k].p[P_LEVEL] = 0;
#endif
}
static void slot(uint32_t s, const steps_t *r, const steps_t *c, const uint8_t *hook, const uint8_t *const *bars,
                 const cry_t *k, uint32_t nk, const steps_t *d)
{
    steps(0, 1, 16, r);
    if (hook)
        choir_hook(hook, bars);
    else
        steps(1, 1, c == &C_A ? 16u : 32u, c);
    creatures(k, nk);
    steps(3, 1, 16, d);
    if (project_save(s))
        fprintf(stderr, "slot %u not saved\n", s);
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "build/dino_song.wav";
    FILE *f, *v = argc > 2 ? fopen(argv[2], "wb") : 0;
    uint32_t i, t, frames, next_frame = 0, rows;
    int32_t o[2 * CTL];
    /* the song: {T1 T2 T3 T4 slots (4: silent)}, repeats */
    static const struct { uint8_t s[4], rep; } ROW[] = {
        {{0, 0, 0, 0}, 2},                             /* awakening: long riff notes, the choir, brachiosaurs, a heartbeat */
        {{1, 0, 4, 1}, 2},                             /* the riff stomps in with the drums */
        {{1, 1, 1, 1}, 2},                             /* the theme: the choir sings the hook, raptors answer */
        {{2, 2, 2, 2}, 2},                             /* rampage: the tyrant roars, power fifths, the hook higher */
        {{3, 3, 3, 3}, 1},                             /* breakdown: a held note, a brachiosaur; the tyrant roars into .. */
        {{1, 2, 1, 1}, 2},                             /* .. the theme with the high hook */
        {{2, 2, 2, 2}, 2},                             /* rampage */
        {{0, 3, 0, 3}, 1},                             /* the giants walk away */
    };
    rep = fopen("/dev/null", "w");
    ui_power_on();
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    song.g[G_BPM] = 70;
    song.g[G_RTYPE] = 2;                               /* HALL */
    fx.rtype = 2;
    song.g[G_RSIZE] = 127;                             /* the biggest hall: a valley */
    song.g[G_RDAMP] = 55;
    song.g[G_DTIME] = 0;                               /* echoes a quarter apart, off the cliffs */
    song.g[G_DFDBK] = 50;
    song.g[G_DCOLOR] = 45;
    song.g[G_TAPE] = 25;
    song.master_q12 = 1300;                            /* (MASTER at ~3/4: the mix under the limiter, its dynamics kept) */
    song.batt_raw = 600;
    usb.config = 0;
    rev_clear();
    hall_clear();
    sounds();
    slot(0, &R_A, &C_A, 0, 0, K_A, 2, &D_A);
    slot(1, &R_B, 0, HOOK_B, BAR_B, K_B, 5, &D_B);
    slot(2, &R_C, 0, HOOK_C, BAR_C, K_C, 6, &D_C);
    slot(3, &R_D, &C_D, 0, 0, K_D, 2, &D_D);
    rows = sizeof ROW / sizeof ROW[0];
    chain_defaults(&chain_config);
    chain_config.count = (uint8_t)rows;
    for (i = 0; i < rows; i++) {
        memcpy(chain_config.row[i].slot, ROW[i].s, 4);
        chain_config.row[i].repeat = ROW[i].rep;
    }
    if (chain_prepare()) {
        fprintf(stderr, "the song did not start\n");
        return 1;
    }
    ui.home = 1;
    {
        uint32_t bars = 0;
        for (i = 0; i < rows; i++)
            bars += 2u * ROW[i].rep;
        frames = (uint32_t)((bars * 4.0 * 60.0 / 70.0 + 6.0) * FS);
    }
    if (!(f = fopen(path, "wb")))
        return 1;
    wav_hdr(f, frames);
    for (t = 0; t < frames; t += CTL) {
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++) {
            wav_put(f, o[2 * i], o[2 * i + 1]);
            scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)clamp(o[2 * i], -32768, 32767);
        }
        if (v && t + CTL > next_frame) {
            static uint8_t rgb[240u * 240u * 3u];
            fm1_ms = (uint32_t)((uint64_t)t * 1000u / FS);
            ui.force = 1;
            ui_draw();
            for (i = 0; i < 240u * 240u; i++) {
                uint16_t c = swap16(host_screen[i]);
                rgb[3 * i] = (uint8_t)((c >> 11) * 255u / 31u);
                rgb[3 * i + 1] = (uint8_t)(((c >> 5) & 63u) * 255u / 63u);
                rgb[3 * i + 2] = (uint8_t)((c & 31u) * 255u / 31u);
            }
            fwrite(rgb, 1, sizeof rgb, v);
            next_frame += FS / 30u;
        }
    }
    fclose(f);
    if (v)
        fclose(v);
    printf("%s: %.1f s, %u sections, voices given up %u\n", path, frames / (double)FS, rows, voice_kills);
    return 0;
}
