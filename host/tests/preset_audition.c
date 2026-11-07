/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the actual firmware unit, including its voice and key ownership. */
#include "../../web/emu/felucca_web.c"
#include <assert.h>

static void settle(void) { for (unsigned i = 0; i < 12; ++i) web_render(1024); }
static unsigned gates(track_t *t) {
    unsigned n = 0;
    for (unsigned i = 0; i < NVOICE; ++i) n += t->v[i].active && t->v[i].gate;
    return n;
}
static void require(int ok, const char *why) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", why); exit(1); }
}
int main(void) {
    web_nor_erase(); web_boot(); settle(); settle(); settle();
    song.sel = 0;
    set_engine(0); settle();
    web_keys(1u << 7); settle();
    require(gates(TSEL) > 0, "initial held note plays");
    preset_step(1); settle();
    require(gates(TSEL) > 0, "held note starts on next preset without lifting finger");
    web_keys(0); settle();
    require(gates(TSEL) == 0, "release after preset change ends the note");
    for (unsigned pass = 0; pass < 12; ++pass) {
        web_keys((1u << 7) | (1u << 11)); settle();
        preset_step(pass & 1 ? -1 : 1); settle();
        require(gates(TSEL) > 0, "held chord survives repeated forward/backward preset browsing");
        web_keys(0); settle();
        require(gates(TSEL) == 0 && !TSEL->xp_n && !TSEL->nmono,
                "chord release leaves no gated, pending or mono notes");
    }
    web_keys(1u << 7); settle();
    set_engine(ENGI_FM6); settle();
    require(TSEL->engine == ENGI_FM6 && gates(TSEL) > 0,
            "held note is queued through cross-engine fade");
    web_keys(0); settle();
    require(gates(TSEL) == 0, "cross-engine audition releases");
    web_keys(1u << 7); settle();
    preset_step(1); web_keys(0); settle();
    require(gates(TSEL) == 0 && !TSEL->xp_n, "release before load is processed never replays a stale key");
    web_keys(1u << 7); settle();
    panic_req |= 1u; settle();
    require(gates(TSEL) == 0, "ordinary panic does not restart held keys");
    web_keys(0); settle();
    set_engine(0); settle();
    TSEL->p[P_CHRD] = CH_DIA3; TSEL->p[P_SCALE] = 1;
    web_keys((1u << 7) | (1u << 11)); settle();
    preset_step(1); settle();
    unsigned owned = 0;
    for (unsigned i = 0; i < 128; ++i) owned += midi_local_held(TSEL, i);
    require(owned && gates(TSEL) == owned, "overlapping chords replay shared tones once");
    web_keys(0); settle();
    require(gates(TSEL) == 0, "overlapping chords release completely");
    TSEL->p[P_CHRD] = 0;
    web_keys(1u << 7); settle();
    step_t before[NSTEP]; memcpy(before, TSEL->step, sizeof before);
    song.playing = 1; song.rec = 1;
    preset_step(1); events_block(0);
    require(!memcmp(before, TSEL->step, sizeof before), "preset audition does not record a phantom performance");
    song.playing = song.rec = 0; web_keys(0); settle();
    require(up_store(0, "AUDITION") == 0, "store user sound");
    web_keys(1u << 7); settle();
    require(up_load(0) == 0, "load user sound"); settle();
    require(gates(TSEL) > 0, "user preset also auditions a held note");
    web_keys(0); settle();
    require(gates(TSEL) == 0, "user preset key release works");
    puts("PASS: preset browsing, chords, cross-engine fades, releases, panic, recording and user sounds");
    return 0;
}
