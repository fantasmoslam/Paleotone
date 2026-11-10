/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS: Paleotone's SAMPLE engine is not built here (its recordings, 205 KB, do not fit beside NACHTFAHRT's).
 * Engine 4 keeps its number and plays nothing; no sound in pt.c's lists uses it. What drums.c and params.c read of
 * the sample sets is here, empty. */
static const char *const N_ONOFF_E[] = {"OFF", "ON"};
typedef struct {                 /* (eng_sample.c's types, as Paleotone's) */
    uint32_t off, n, ls, le;
    uint32_t rate;
    int16_t root16;
    int16_t pred;
    uint8_t idx, lo, hi, looped;
} smp_zone_t;
#define SMP_LOOP_HELD 2u
typedef struct {
    const char *name;
    uint16_t z0, nz;
} smp_set_t;
#define SMP_NSETS 0
#define SMP_USER_SLOTS 4               /* (the drum kits' names count them; never loaded: usr_nz stays 0) */
#define SMP_NALL 4
static const smp_zone_t SMP_ZONES[1];
static const smp_set_t SMP_SETS[1];
static const uint8_t SMP_DATA[1];
static smp_zone_t usr_zone[SMP_USER_SLOTS][1];
static uint8_t usr_nz[SMP_USER_SLOTS];
static const char *const SMP_ALL_NAMES[SMP_NALL] = {"USR1", "USR2", "USR3", "USR4"};
static void silent_note_on(struct track *t, voice_t *v) { (void)t, (void)v; }
static void silent_render(struct track *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    (void)t, (void)out, (void)n, (void)m;
    v->active = 0;
}
static const preset_t SILENT_PRESETS[1] = {{"-", {0}, {0, 0, 127, 0}, 0, 0}};
static const engine_t ENG_SAMPLE = {
    "SAMPLE", {"-", "-"}, {{"-", F_INT, 0, 0, 0, 0, 0}}, SILENT_PRESETS, 1, -1, silent_note_on, silent_render,
    0xFFFF, {P_E0, P_E1, P_ATK, P_REL},
};
static void sample_spread(track_t *t, voice_t *v, uint32_t r) { (void)t, (void)v, (void)r; }   /* (voice.c's UNISON) */
/* (drums.c's sampled drums: never played here, no sample set; pow2_q16 as eng_sample.c's) */
static uint32_t pow2_q16(int32_t d16)
{
    uint32_t u = (uint32_t)(d16 + 192 * 16), oct = u / 192u;
    uint32_t r = pitch_inc(1600 + u % 192u) / (pitch_inc(1600) >> 16);
    return oct >= 16u ? r << (oct - 16u) : r >> (16u - oct);
}
static inline int32_t sample_next(const smp_zone_t *z, voice_t *v, int loop)
{
    (void)z, (void)loop;
    v->active = 0;
    return 0;
}
