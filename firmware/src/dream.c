/* SPDX-License-Identifier: GPL-3.0-only
 * PALEOTONE SCAPE MUSIC: a bounded composer sharing SCAPE's four instrument tracks.
 * Ordinary 64-step patterns, four chords, one phrase per chord. No extra sequencer or audio voices.
 * The old global IDs and project representation stay valid. */
enum { DR_DRONE, DR_BASS, DR_PAD, DR_MELODY, DR_PERC };
#define DR_LEN 64u
#define DR_SPAN 16u
static const uint8_t DR_MOOD_SCL[6] = {2, 3, 8, 7, 1, 4};
static const uint8_t DR_PROG[4][4] = {{0,5,6,0},{0,3,0,4},{0,6,5,4},{0,2,5,0}};
static struct {
    uint8_t valid, asked, root, scl, deg[4], role[NTRK], passes, last;
} dream;

static void dream_forget(void) { dream.valid = 0; }
static uint32_t dr_rnd(uint32_t n) { return n ? rng() % n : 0; }
static int dr_has(const char *s, const char *w)
{
    uint32_t i, j;
    for (i = 0; s[i]; i++) {
        for (j = 0; w[j] > ' ' && s[i+j] == w[j]; j++);
        if (w[j] <= ' ') return 1;
    }
    return 0;
}
static uint32_t dr_role(const track_t *t)
{
    uint32_t e = t->eng_req % NENGINES;
    const engine_t *en = ENGINES[e];
    const char *name = !t->user && en->npresets ? en->presets[t->preset % en->npresets].name : "";
    if (e == ENGI_DRUM) return DR_PERC;
    if (dr_has(name, "BASS") || dr_has(name, "TUBA")) return DR_BASS;
    if (e == ENGI_SAURUS || en == &ENG_NOISE || e == ENGI_GURDY) return DR_DRONE;
    if (en == &ENG_FORMANT || en == &ENG_WHEEL || en == &ENG_GRAIN || t->p[P_ATK] >= 50) return DR_PAD;
    return DR_MELODY;
}
static uint32_t dr_note(int32_t degree, int32_t oct)
{
    uint8_t pc[12];
    uint32_t i, n = 0, mask = SCALE_MASK[dream.scl % NELEM(SCALE_MASK)];
    int32_t o;
    for (i = 0; i < 12; i++) if ((mask >> i) & 1u) pc[n++] = (uint8_t)i;
    o = degree >= 0 ? degree / (int32_t)n : -(((int32_t)n - 1 - degree) / (int32_t)n);
    return (uint32_t)clamp(12 * (oct + 1 + o) + dream.root + pc[degree - o * (int32_t)n], 24, 100);
}
static void dr_put(track_t *t, uint32_t at, int32_t degree, uint32_t oct, uint32_t voices, uint32_t hold, uint32_t chance)
{
    uint32_t i;
    step_t *s = &t->step[at];
    step_clear(s);
    s->time = ST_NOTE;
    s->vel = (uint8_t)(72u + dr_rnd(24));
    step_set_chance(s, chance);
    s->n = (uint8_t)voices;
    for (i = 0; i < voices; i++) s->note[i] = (uint8_t)dr_note(degree + (int32_t)(2u * i), (int32_t)oct);
    for (i = 1; i < hold && at + i < DR_LEN; i++) {
        step_clear(&t->step[at+i]);
        t->step[at+i].time = ST_TIE;
    }
}
static void dr_roles(void)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) dream.role[i] = (uint8_t)dr_role(&trk[i]);
}
static void dr_span(track_t *t, uint32_t span, int unused)
{
    uint32_t a = span * DR_SPAN, i, ti = trk_index(t), r = dream.role[ti];
    uint32_t dens = (uint32_t)clamp(song.g[G_DDENS], 1, 8), spacing = dens >= 6 ? 2u : dens >= 3 ? 4u : 8u;
    int32_t degree = dream.deg[span];
    (void)unused;
    if (r == DR_DRONE && span) return;
    for (i = 0; i < (r == DR_DRONE ? DR_LEN : DR_SPAN); i++) step_clear(&t->step[a+i]);
    if (r == DR_DRONE) {
        dr_put(t, 0, 0, 3, 1, DR_LEN, 100);
    } else if (r == DR_BASS || r == DR_PAD) {
        dr_put(t, a, degree, r == DR_BASS ? 2 : 3, r == DR_PAD ? 3 : 1, DR_SPAN, 100);
    } else if (r == DR_PERC) {
        for (i = 0; i < DR_SPAN; i += spacing) {
            if (t->eng_req % NENGINES == ENGI_DRUM) {
                grid_hit(t, a+i, !i ? (span ? DV_TOM : DV_KICK) : DV_TOM, 1);
                t->step[a+i].vel = (uint8_t)(!i ? 96 : 65 + dr_rnd(20));
                step_set_chance(&t->step[a+i], !i ? 100 : 45 + dens * 5);
            } else dr_put(t, a+i, degree, 3, 1, 1, !i ? 100 : 70);
        }
    } else {
        /* Repeated rhythm, sparse scale-degree movement, a chord tone at each phrase's start. */
        int32_t d = degree;
        for (i = 0; i < DR_SPAN; i += spacing) {
            dr_put(t, a+i, d, ti & 1u ? 4 : 3, 1, spacing > 2 ? spacing - 1 : 1, !i ? 100 : 65 + dens * 4);
            d += (int32_t)dr_rnd(3) - 1;
            d = clamp(d, degree - 2, degree + 4);
        }
    }
}
static void dream_compose(uint32_t root)
{
    uint32_t i, s;
    dream.root = (uint8_t)(root % 12);
    dream.scl = DR_MOOD_SCL[clamp(song.g[G_DMOOD], 0, 5)];
    memcpy(dream.deg, DR_PROG[dr_rnd(NELEM(DR_PROG))], sizeof dream.deg);
    dr_roles();
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        motion_reset(t);
        for (s = 0; s < NSTEP; s++) step_clear(&t->step[s]);
        t->p[P_SLEN] = DR_LEN; t->p[P_SDIV] = 1;
        t->p[P_ROOT] = dream.root; t->p[P_SCALE] = dream.scl;
        t->p[P_CHRD] = 0; t->p[P_AMODE] = 0;
        if (t->eng_req % NENGINES == ENGI_GURDY) t->p[P_E0] = dream.root;
        for (s = 0; s < 4; s++) dr_span(t, s, 0);
    }
    dream.valid = 1; dream.passes = 0;
    if (song.playing) transport_req = 3;
    else if (!chain_busy()) transport_req = 1;
}
static void dr_rebuild(void)
{
    /* Following a project/preset load, keep its key; only later evolved phrases are regenerated. */
    dream.root = (uint8_t)(trk[0].p[P_ROOT] % 12);
    dream.scl = (uint8_t)trk[0].p[P_SCALE];
    memcpy(dream.deg, DR_PROG[0], sizeof dream.deg);
    dr_roles();
    dream.valid = 1;
}
static void dream_evolve(void)
{
    uint32_t ti = dr_rnd(NTRK), span = 1 + dr_rnd(3);
    if (!dream.valid) dr_rebuild();
    /* Preserve the opening motif, sustained creatures, mutes and the user's other tracks. */
    if (dream.role[ti] != DR_DRONE) dr_span(&trk[ti], span, 0);
}
static void dream_go(uint32_t root)
{
    char b[16];
    if (chain_busy()) { ui_message("STOP THE SONG FIRST"); return; }
    dream_compose(root);
    str_cpy(b, N_GURDY_DRN[root % 12], 4);
    str_cpy(b + str_len(b), " ", 2);
    str_cpy(b + str_len(b), N_DMOOD[clamp(song.g[G_DMOOD], 0, 5)], 6);
    ui_say("SCAPE IN ", b);
}
static void dream_keys(uint32_t notes)
{
    uint32_t k, i, j, any = 0;
    for (k = 0; k < 27 && !((notes >> k) & 1u); k++);
    if (k == 27) return;
    for (i = 0; i < NTRK && !any; i++)
        for (j = 0; j < NSTEP && !any; j++) any = trk[i].step[j].n || trk[i].step[j].hit;
    if (any && !dream.asked && !chain_busy()) confirm_open(CF_DREAM, (53+k) % 12);
    else dream_go((53+k) % 12);
}
static void dream_tick(void)
{
    uint32_t idx = trk[0].seq_idx, every = 9u - (uint32_t)clamp(song.g[G_DDRFT], 1, 8);
    if (!song.g[G_DREAM] || !song.playing || chain_busy()) { dream.last = dream.passes = 0; return; }
    if (idx < dream.last && ++dream.passes >= every) { dream.passes = 0; dream_evolve(); }
    dream.last = (uint8_t)idx;
}
