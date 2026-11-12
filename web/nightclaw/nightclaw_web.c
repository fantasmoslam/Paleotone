/* SPDX-License-Identifier: GPL-3.0-only */
/* Browser-only adapter. The firmware UI, project and DSP remain authoritative. */
#include "../emu/amb_web.c"
#define NC_API __attribute__((used, visibility("default")))
static int32_t nc_st[32];
static char nc_text_buf[96];
NC_API int nc_scene_count(void) { return NPIECES; }
NC_API const char *nc_scene_name(int i) { return piece_name(i >= 0 && i < NPIECES ? i : 0); }
NC_API const char *nc_track_name(int i) { return HOME_NAME[i >= 0 && i < HOME_TRACKS ? i : 0]; }
static int nc_voice_param(void) {
    if (ui.view == V_MIX && !ui.perf) return home_voice();
    if (ui.view == V_CALLS) return P_CLVOICE;
    if (atm_view(ui.view)) return P_NCHAR;
    return view_voice(ui.view);
}
static int nc_is_beat(void) { return !ui.perf && ((ui.view == V_MIX && ui.track == 5) || ui.view == V_SKIN); }
NC_API int nc_voice_count(void) {
    int p = nc_voice_param();
    return nc_is_beat() ? EN_NBEAT : p >= 0 ? en_param_info(p)->hi + 1 : 0;
}
NC_API const char *nc_voice_name(int i) {
    int p = nc_voice_param();
    if (i < 0 || i >= nc_voice_count()) return "";
    if (nc_is_beat()) return en_beat_name(i);
    en_param_text(p, i, nc_text_buf); return nc_text_buf;
}
NC_API const char *nc_control_name(int col) {
    if (col < 0 || col > 3) return "";
    return ui.view == V_MIX && !ui.perf ? track_label(col) : KNOB_LABEL[ui.view][col];
}
NC_API const char *nc_control_text(int col) {
    if (col < 0 || col > 3) return "";
    knob_text(control_knob(ui.view, col), nc_text_buf); return nc_text_buf;
}
NC_API const char *nc_page_name(void) { return ui.view == V_MIX && !ui.perf ? "Home" : VIEW_NAME[ui.view]; }
NC_API int32_t *nc_state(void) {
    int col, p = nc_voice_param();
    nc_st[0] = proj.piece; nc_st[1] = ui.view; nc_st[2] = ui.track;
    nc_st[3] = en_playing; nc_st[4] = en_frozen; nc_st[5] = ui.dirty;
    nc_st[6] = en_call_auto;
    nc_st[7] = nc_is_beat() ? project_drum_pattern() : p >= 0 ? proj.par[p] : -1;
    nc_st[8] = nc_voice_count(); nc_st[9] = calls_context() ? 1 : ui.perf ? 2 : ui.view == V_KEY || ui.view == V_CHORDS || ui.view == V_PAT ? 3 : 0;
    for (col = 0; col < 4; col++) {
        int k = control_knob(ui.view, col), lo, hi; knob_range(k, &lo, &hi);
        nc_st[12 + col * 3] = knob_get(k); nc_st[13 + col * 3] = lo; nc_st[14 + col * 3] = hi;
    }
    return nc_st;
}
NC_API void nc_scene(int i) { if (i >= 0 && i < NPIECES) set_piece(i); }
NC_API void nc_track(int i) {
    if (i < 0 || i >= HOME_TRACKS) return;
    keys_release_mode(); perf_set(0); ui.track = (uint8_t)i; set_view(V_MIX);
}
NC_API void nc_page(int i) {
    /* Only ordinary sound/performance pages; never update/recovery modes. */
    if (i < 0 || i >= NVIEWS) return;
    keys_release_mode();
    if (i == V_PERF) { perf_set(1); return; }
    perf_set(0); set_view(i);
}
NC_API void nc_control(int col, int v) {
    if (col >= 0 && col < 4) { int k = control_knob(ui.view, col); turn(EN_K1 + col, k, v - knob_get(k)); }
}
NC_API void nc_voice(int i) {
    int p = nc_voice_param();
    if (i < 0 || i >= nc_voice_count()) return;
    if (nc_is_beat()) { project_drum_pattern_set(i); en_drum_pattern(project_drum_pattern()); mark_dirty(); }
    else if (p >= 0) { if (p == P_CLVOICE) knob_set(p, i); else set_voice(p, i); }
    ui.sig[0] = ui.sig[1] = ui.sig[2] = 0;
}
NC_API void nc_save(void) { do_save(); }
NC_API void nc_play(int on) { if (!!on != !!en_playing) button(B_PLAY); }
NC_API void nc_hold(int on) { en_freeze(!!on); }
NC_API void nc_calls_auto(int on) { en_calls_auto(!!on); }

NC_API void nc_octave(int n) {
    if (ui.view == V_MIX && ui.track < NLAYER && LAYER_OCT[ui.track] != NO_OCT) {
        int k = LAYER_OCT[ui.track]; knob_set(k, knob_get(k) + (n > 0 ? 1 : -1));
    } else { oct_step(n > 0 ? B_OCTUP : B_OCTDN); }
}

NC_API int nc_page_count(void) { return NVIEWS; }
NC_API const char *nc_page_title(int i) { return i == V_MIX ? "Home" : i > 0 && i < NVIEWS ? VIEW_NAME[i] : "Scene motion"; }
