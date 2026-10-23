/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX demo song, "TITAN RISING": composed in the synth itself through the firmware's own code. Four project slots
 * (A..D) hold the sections' sequences; the SONG chains them, each track on its own slot per section (lanes), with
 * repeats; the sounds stay the current ones, as on the FM-1. 70 BPM, D minor: a stomping low brass riff, a choir, a
 * soaring horn, the TRIBE kit driving under it (accents, ratchets).
 *   T1 FM6 MAMMOTH      the riff                   T2 VOICE EPIC CHOIR  the chords
 *   T3 ANALOG CANYON HORN the hook (glides)        T4 DRUM TRIBAL KIT   the beat (1/16)
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
#define DM {50, 53, 57}
#define BB {50, 53, 58}
#define CC {48, 52, 55}
#define AM {49, 52, 57}                                /* A major: the harmonic minor's dominant */
#define GM {50, 55, 58}
#define TIE {T_}
/* T1 MAMMOTH, 1/8, 16 steps (2 bars) */
static const steps_t R_A = {{{38 | ACC}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, {34 | ACC}, TIE, TIE, TIE, {33 | ACC}, TIE, TIE, TIE}};
static const steps_t R_B = {{{38 | ACC}, {0}, {38}, {38}, {41 | ACC}, {0}, {40}, {0},
                             {38 | ACC}, {0}, {38}, {38}, {33 | ACC}, {0}, {36}, {34}}};
static const steps_t R_C = {{{38 | ACC, 45}, {0}, {38, 45}, {38, 45}, {41 | ACC, 48}, {0}, {40, 47}, {0},
                             {38 | ACC, 45}, {0}, {38, 45}, {38, 45}, {36 | ACC, 43}, {36, 43}, {34, 41}, {33, 40}}};
static const steps_t R_D = {{{38}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE, TIE}};
/* T2 EPIC CHOIR, 1/8, 16 steps */
static const steps_t C_A = {{DM, TIE, TIE, TIE, TIE, TIE, TIE, TIE, BB, TIE, TIE, TIE, AM, TIE, TIE, TIE}};
static const steps_t C_B = {{DM, TIE, TIE, TIE, BB, TIE, TIE, TIE, CC, TIE, TIE, TIE, AM, TIE, TIE, TIE}};
static const steps_t C_C = {{DM, TIE, TIE, GM, TIE, TIE, BB, TIE, TIE, TIE, CC, TIE, AM, TIE, TIE, TIE}};
static const steps_t C_D = {{{50, 57, 62}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, {50, 57, 61}, TIE, TIE, TIE, TIE, TIE, TIE, TIE}};
/* T3 CANYON HORN, 1/8, 32 steps (4 bars): the hook; C an octave's urgency higher; D a lone call */
static const steps_t H_B = {{{69 | ACC}, TIE, TIE, {74}, TIE, {72}, TIE, {69},
                             {70 | ACC}, TIE, {69}, {67}, TIE, {65}, TIE, TIE,
                             {69 | ACC}, TIE, TIE, {74}, TIE, {76}, TIE, {77},
                             {76 | ACC}, TIE, {74}, {73}, TIE, {74}, TIE, TIE}};
static const steps_t H_C = {{{74 | ACC}, TIE, {72}, {74}, TIE, {77}, TIE, {76},
                             {74 | ACC}, TIE, {72}, {70}, TIE, {69}, TIE, TIE,
                             {74 | ACC}, TIE, {77}, TIE, {79}, TIE, {81}, TIE,
                             {79 | ACC}, TIE, {77}, {76}, TIE, {74}, TIE, TIE}};
static const steps_t H_D = {{{69}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
                             {73}, TIE, TIE, TIE, {74}, TIE, TIE, TIE, TIE, TIE, TIE, TIE, {0}, {0}, {0}, {0}}};
/* T4 TRIBAL KIT, 1/16, 32 steps (2 bars). GM notes: 36 BOOM, 38 SKIN, 39 CLAP, 42 SEEDS, 46 RATTL, 45 LOG, 37 BONE,
 * 56 ROCK */
#define AT(i, ...) [i] = {__VA_ARGS__}
static const steps_t D_A = {{AT(0, 36 | ACC), AT(3, 36), AT(12, 45), AT(16, 36 | ACC), AT(19, 36), AT(28, 45), AT(30, 45)}};
static const steps_t D_B = {{AT(0, 36 | ACC, 42), AT(2, 42), AT(4, 38 | ACC, 42), AT(6, 36, 42), AT(8, 36 | ACC, 42), AT(10, 42),
                             AT(11, 36), AT(12, 38 | ACC, 42), AT(14, 45 | R2), AT(15, 45),
                             AT(16, 36 | ACC, 42), AT(18, 42), AT(20, 38 | ACC, 42), AT(22, 36, 42), AT(24, 36 | ACC, 42),
                             AT(26, 42), AT(27, 36), AT(28, 38 | ACC, 39), AT(30, 46), AT(31, 45 | R3)}};
static const steps_t D_C = {{AT(0, 36 | ACC, 56), AT(1, 42), AT(2, 42), AT(3, 36), AT(4, 38 | ACC, 39), AT(5, 42), AT(6, 36, 42),
                             AT(7, 42), AT(8, 36 | ACC, 42), AT(9, 42), AT(10, 36, 42), AT(11, 36), AT(12, 38 | ACC, 39),
                             AT(13, 45), AT(14, 45 | R2), AT(15, 45 | R3),
                             AT(16, 36 | ACC, 46), AT(17, 42), AT(18, 42), AT(19, 36), AT(20, 38 | ACC, 39), AT(21, 42),
                             AT(22, 36, 42), AT(23, 42), AT(24, 36 | ACC, 42), AT(25, 37), AT(26, 36, 42), AT(27, 37),
                             AT(28, 38 | ACC | R2, 39), AT(29, 45 | R2), AT(30, 45 | R3), AT(31, 36 | ACC, 56)}};
static const steps_t D_D = {{AT(0, 36 | ACC, 56), AT(16, 36, 46), AT(24, 45), AT(28, 45 | R2)}};

/* the sounds (one set for the whole song) and the four slots */
static void sounds(void)
{
    host_preset(&trk[0], ENGI_FM6, preset_of(ENGI_FM6, "MAMMOTH"));
    host_preset(&trk[1], 5, preset_of(5, "EPIC CHOIR"));
    host_preset(&trk[2], 0, preset_of(0, "CANYON HORN"));
    host_preset(&trk[3], ENGI_DRUM, preset_of(ENGI_DRUM, "TRIBAL KIT"));
    trk[0].p[P_LEVEL] = 117; trk[0].p[P_PAN] = 0;           /* (balanced by ear and meter: the riff and the drums lead, the hook over them, the choir a bed) */
    trk[1].p[P_LEVEL] = 64;  trk[1].p[P_PAN] = -18;
    trk[2].p[P_LEVEL] = 86;  trk[2].p[P_PAN] = 14;  trk[2].p[P_GLIDE] = 18;   /* the horn glides: Vangelis */
    trk[3].p[P_LEVEL] = 113;
#ifdef DS_SOLO
    for (uint32_t k = 0; k < NTRK; k++)
        if (k != DS_SOLO)
            trk[k].p[P_LEVEL] = 0;
#endif
    trk[2].p[P_REV] = 70; trk[1].p[P_REV] = 90; trk[0].p[P_REV] = 45; trk[3].p[P_REV] = 55;
}
static void slot(uint32_t s, const steps_t *r, const steps_t *c, const steps_t *h, uint32_t hl, const steps_t *d)
{
    steps(0, 1, 16, r);
    steps(1, 1, 16, c);
    steps(2, 1, hl, h);
    steps(3, 2, 32, d);
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
        {{0, 0, 4, 0}, 2},                             /* awakening: the riff's long notes, the choir, a heartbeat */
        {{1, 0, 4, 1}, 2},                             /* the riff stomps in with the beat */
        {{1, 1, 1, 1}, 2},                             /* the theme */
        {{2, 2, 2, 2}, 2},                             /* rampage: power fifths, the hook high, the drums wild */
        {{3, 3, 3, 3}, 1},                             /* breakdown: a held note, the choir alone, a lone call */
        {{1, 1, 2, 1}, 2},                             /* the theme under the high hook */
        {{2, 2, 2, 2}, 2},                             /* rampage */
        {{0, 3, 4, 3}, 1},                             /* the titan walks away */
    };
    rep = fopen("/dev/null", "w");
    ui_power_on();
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    song.g[G_BPM] = 70;
    song.g[G_RTYPE] = 2;                               /* HALL */
    fx.rtype = 2;
    song.g[G_RSIZE] = 110;
    song.g[G_RDAMP] = 60;
    song.g[G_TAPE] = 25;
    song.batt_raw = 600;
    usb.config = 0;
    rev_clear();
    hall_clear();
    sounds();
    slot(0, &R_A, &C_A, 0, 16, &D_A);
    slot(1, &R_B, &C_B, &H_B, 32, &D_B);
    slot(2, &R_C, &C_C, &H_C, 32, &D_C);
    slot(3, &R_D, &C_D, &H_D, 32, &D_D);
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
