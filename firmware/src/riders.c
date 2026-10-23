/* SPDX-License-Identifier: GPL-3.0-only */
/* DINOBOX: the demo song RIDERS OF THE VALLEY (MENU > SYSTEM > SCENE RIDERS). The scene loads the song's four sounds
 * and leaves its theme in the tracks; PLAY (not on the SONG page) plays the whole song while those patterns are as the
 * scene left them, from the tables in build/gen/riders_song.h (tools/gen_riders_song.py): its four sections' steps,
 * automation and chain go straight into the song's sources, as chain_prepare would put saved slots there. The saved
 * projects (slots A..D) and the user's SONG chain are never read or written. Included by ui.c. */
#include "riders_song.h"

static uint8_t riders_on;                  /* the RIDERS scene is loaded (cleared by any other scene) */
static uint32_t riders_sig[NTRK];          /* steps_sig of the theme it left in each track */

static uint32_t riders_preset(uint32_t e, const char *name)     /* a raw preset index by name (NPRESET: none) */
{
    uint32_t i;
    for (i = 0; i < ENGINES[e]->npresets; i++)
        if (str_eq(ENGINES[e]->presets[i].name, name))
            return i;
    return 0xFFFFu;
}

/* slot s's steps of track k (a cleared NSTEP array) */
static void riders_steps(uint32_t s, uint32_t k, step_t *st)
{
    uint32_t i, j;
    for (i = 0; i < NSTEP; i++) {
        memset(&st[i], 0, sizeof st[i]);
        st[i].time = ST_REST;
    }
    for (i = 0; i < RS_NNOTE; i++) {
        const uint8_t *n = RS_NOTE[i];
        step_t *p = &st[n[1] % NSTEP];
        if ((uint32_t)(n[0] >> 6) != s || (uint32_t)(n[0] >> 4 & 3u) != k)
            continue;
        if (p->time == ST_NOTE && p->n && p->n < 4u) {           /* another note on the step: a kit hit, a chord */
            p->note[p->n++] = n[2];
            continue;
        }
        p->time = ST_NOTE;
        p->n = 1;
        p->note[0] = n[2];
        p->vel = n[4];
        p->flags = (uint8_t)((n[0] & 3u) << SF_RATCH_SH);
        for (j = 1; j < n[3] && n[1] + j < NSTEP; j++) {         /* held: ties */
            step_t *q = &st[n[1] + j];
            memset(q, 0, sizeof *q);
            q->time = ST_TIE;
        }
    }
}

/* slot s's automation (the ROAR rhythm changes) into m, for the tracks whose engine is the song's */
static void riders_motion(uint32_t s, motion_store_t *m)
{
    uint32_t i, j, n = 0;
    memset(m, 0, sizeof *m);
    for (i = 0; i < RS_NRHYTHM; i++) {
        uint32_t k = RS_RHYTHM[i].track % NTRK, e = RS_SOUND[k].engine, pi;
        const preset_t *p;
        if (RS_RHYTHM[i].slot != s || trk[k].eng_req != e || (pi = riders_preset(e, RS_RHYTHM[i].preset)) == 0xFFFFu)
            continue;
        p = &ENGINES[e]->presets[pi];
        for (j = 0; j < 13u && n < MOTION_MAX; j++) {
            uint32_t id = j < 8u ? P_E0 + j : j < 12u ? P_ATK + (j - 8u) : P_DIST;
            int32_t v = j < 8u ? p->e[j] : j < 12u ? p->env[j - 8u] : (p->fx[0] ? p->fx[0] - 1 : 0);
            if (!motion_param(id))
                continue;
            m->event[n].place = (uint8_t)(k << 6 | RS_RHYTHM[i].step);
            m->event[n].param = (uint8_t)id;
            m->event[n].value = (int8_t)v;
            n++;
            m->on |= (uint8_t)(1u << k);
        }
    }
    m->count = (uint8_t)n;
}

/* after gb_scene_load(GB_SCENE_RIDERS) put the sounds: the song's levels, dry (as the approved mix), its theme */
static void riders_scene(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        t->p[P_LEVEL] = RS_SOUND[k].level;
        t->p[P_CHOR] = t->p[P_DLY] = t->p[P_REV] = 0;
        riders_steps(RS_THEME_SLOT, k, t->step);
        t->p[P_SDIV] = RS_TIMING[RS_THEME_SLOT][k][0];
        t->p[P_SLEN] = RS_TIMING[RS_THEME_SLOT][k][1];
        t->p[P_SGATE] = RS_TIMING[RS_THEME_SLOT][k][2];
        pat_sig[k] = riders_sig[k] = steps_sig(t);
        pat_last[k] = 0;
    }
    for (k = 0; k < RS_NPARAM; k++)
        trk[RS_PARAM[k].track % NTRK].p[RS_PARAM[k].id] = RS_PARAM[k].value;
    song.g[G_BPM] = RS_BPM;
    riders_on = 1;
}

/* PLAY: the song, if the RIDERS scene's theme is still in the tracks. 1: started */
static int riders_play(void)
{
    uint32_t s, k, j;
    if (!riders_on || transport_busy())
        return 0;
    for (k = 0; k < NTRK; k++)
        if (steps_sig(&trk[k]) != riders_sig[k])
            return 0;
    for (s = 0; s < 4u; s++) {
        for (k = 0; k < NTRK; k++) {
            riders_steps(s, k, chain.source[s].step[k]);
            chain.source[s].qnt[k] = 1;
            for (j = 0; j < 4u; j++)                            /* LEN DIV SWING GATE (core.h P_SLEN..) */
                chain.source[s].timing[k][j] = 0;
            chain.source[s].timing[k][P_SLEN - P_SLEN] = RS_TIMING[s][k][1];
            chain.source[s].timing[k][P_SDIV - P_SLEN] = RS_TIMING[s][k][0];
            chain.source[s].timing[k][P_SGATE - P_SLEN] = RS_TIMING[s][k][2];
        }
        riders_motion(s, &chain.source[s].motion);
    }
    memset(&chain.config, 0, sizeof chain.config);
    chain.config.count = RS_NROW;
    for (s = 0; s < RS_NROW; s++) {
        memcpy(chain.config.row[s].slot, RS_ROW[s], NTRK);
        chain.config.row[s].repeat = RS_ROW[s][NTRK];
    }
    RING_PUBLISH();
    chain.armed = 1;
    transport_req = 1;
    return 1;
}
