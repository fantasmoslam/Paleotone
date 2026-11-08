/* SPDX-License-Identifier: GPL-3.0-only */
/* The user guide's screens (not a test): the UI test's harness, the demo song as a new FM-1 has it, driven by the
 * panel as a player would, each screen written as a 240x240 PPM into DIR/<name>.ppm (web/site/guide_shots.py turns
 * them into the guide's PNGs).   guide_shots DIR */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void shot(const char *name)
{
    ppm(name);
    printf("guide: %s\n", name);
}
static void hold(uint32_t b, uint32_t n) { press(b); frames(n); }
static void let(uint32_t b) { release(b); frames(4); }
static void turn(uint32_t role, int32_t n) { encs[panel.enc[role]] += n * panel.dir[role]; frames(3); }
static void sel_track(uint32_t t) { while (song.sel != t) { turn(EN_ALGO, song.sel < t ? 1 : -1); } frames(2); }
static void page(const char *title, uint32_t fam)
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++) if (!strcmp(PAGES[i].title, title) && PAGES[i].fam == fam) break;
    if (i == NPAGES) { fprintf(stderr, "? page %s\n", title); exit(1); }
    open_family(fam); ui.page = (uint8_t)i; ui.fam_last[fam] = (uint8_t)i; page_entered(); ui.force = 1; frames(3);
}
static void home(void) { go_home(); ui.msg_t = 0; ui.force = 1; frames(3); }
int main(int argc, char **argv)
{
    uint32_t i;
    outdir = argc > 1 ? argv[1] : "build/guide";
    panel = PANEL_DEFAULT;
    settings.palette = PAL_TERMINAL;
    palette_set(PAL_TERMINAL);
    for (i = 0; i < G_COUNT; i++) song.g[i] = GP[i].def;
    fm6_init();
    for (i = 0; i < NTRK; i++) {
        track_defaults(&trk[i]);
        if (i < NPART) { set_engine_of(&trk[i], TRK_DEF[i][0]); apply_preset_to(&trk[i], TRK_DEF[i][1]); trk[i].engine = trk[i].eng_req; }
        track_defaults_steps(&trk[i]);
    }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    song.master_q12 = 2048;
    demo_fill(1, 3);
    layers_init();
    sloop_splash(); shot("splash");
    home(); shot("tracks");
    tap(B_PLAY); frames(90); shot("tracks-playing");
    tap(B_PLAY); frames(10);
    sel_track(4); home(); turn(EN_PRESET, -1); frames(2); shot("presets-bar");
    turn(EN_PRESET, 1); frames(60); home();
    sel_track(0); home();
    hold(B_FX, 40); shot("layer-fx"); let(B_FX); home();
    hold(B_EDIT, 40); shot("layer-edit"); let(B_EDIT); home();
    hold(B_ARP, 40); shot("layer-arp"); let(B_ARP); home();
    hold(B_SEQ, 40); shot("layer-seq"); let(B_SEQ); home();
    hold(B_SCL, 40); shot("layer-sel"); let(B_SCL); home();
    hold(B_GLO, 40); shot("layer-mix"); let(B_GLO); home();
    hold(B_SAVE, 40); shot("layer-song"); let(B_SAVE); home();
    home();
    page("ENV", FAM_ENV); shot("page-env");
    page("LFO", FAM_LFO); shot("page-lfo");
    page("FX", FAM_FX); shot("page-fx");
    page("TAPE", FAM_FX); shot("page-tape");
    page("EDIT 1", FAM_EDIT); shot("page-edit");
    page("VOICE", FAM_EDIT); shot("page-voice");
    page("SEL", FAM_SCL); shot("page-sel");
    page("ARP", FAM_ARP); shot("page-arp");
    page("PATTERN", FAM_SEQ); shot("page-pattern");
    page("NOTES", FAM_SEQ); shot("page-notes");
    page("GLOBAL", FAM_GLO); shot("page-global");
    page("MIDI", FAM_GLO); shot("page-midi");
    page("TOOLS", FAM_SAVE); shot("page-tools");
    sel_track(4); page("EDIT 1", FAM_EDIT); shot("page-edit-saurus");
    page("EDIT 2", FAM_EDIT); shot("page-edit-saurus2");
    {   /* a ROAR sound on track 1: its RING page */
        uint32_t e, k;
        for (e = 0; e < NENGINES && ENGINES[e]->name[0] != 'R'; e++) ;
        sel_track(0);
        for (k = 0; k < ENGINES[e]->npresets && strcmp(ENGINES[e]->presets[k].name, "PRIMEVAL"); k++) ;
        set_engine_of(&trk[0], e); apply_preset_to(&trk[0], k % ENGINES[e]->npresets); trk[0].engine = trk[0].eng_req;
        page("EDIT 1", FAM_EDIT); shot("page-edit-roar");
        page("RING", FAM_EDIT); shot("page-ring");
        set_engine_of(&trk[0], TRK_DEF[0][0]); apply_preset_to(&trk[0], TRK_DEF[0][1]); trk[0].engine = trk[0].eng_req;
    }
    home();
    sel_track(TRK_DRUM); home(); tap(B_EDIT); frames(3); shot("drums-grid");
    tap(B_EDIT); frames(3); shot("drums-kit");
    tap(B_EDIT); frames(3); shot("drums-sound");
    home(); sel_track(0); home();
    tap(B_SAVE); frames(3); shot("song");
    home(); tap(B_REC); frames(6); shot("rec-ready"); tap(B_REC); frames(6); home();
    hold(B_HOME, 60); let(B_HOME); frames(3); shot("menu");
    hold(B_HOME, 60); let(B_HOME); home();
    return 0;
}
