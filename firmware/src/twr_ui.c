/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS: the app's screen and controls (the main loop's side; the conductor is twr_play.c, the audio side).
 * T-WRECKS starts on its own screen, the preset playing (PLAY starts it). Everything of Paleotone is still there
 * under it: a sound's pages (ENV, LFO, FX, EDIT ...) edit the selected layer, HOME held the menu (GLO > MIDI: each
 * layer's channel, SYSTEM > OUT = SEQ: everything the conductor plays to MIDI OUT). HOME tapped on Paleotone's pages:
 * back here; tapped here: Paleotone's TRACKS screen (the mix).
 *
 *   ALGORITHM what PRESETS changes: PRESET (the first place), or a layer (DRONE .. DRUMS), whose sound it swaps;
 *             the keys and the sound pages play that layer (the LEAD on PRESET)
 *   PRESETS   on PRESET: the preset (loads at once; playing, the arc starts again); on a layer: its sound, through
 *             all of Paleotone's (the PRESETS list); on DRUMS the kit. The swaps last until another preset loads
 *   REC       SHUFFLE: every layer a new sound of the same kind (a pad for a pad, a bass for a bass)
 *   SELECT    the tempo
 *   KNOB 1..4 DENSITY (the figures' notes), DRUMS (0: none), CALLS (how often), AMB (the atmosphere)
 *   PLAY      start / stop
 *   OCT- OCT+ the keys' octave
 *   keys      the LEAD, white keys in the mode from C4 = the root (MIDI in: GLO > MIDI's channels) */

static uint8_t twr_cur;                             /* the preset loaded */
static uint8_t twr_focus;                           /* 0: PRESETS picks the preset; 1 + i: it swaps layer i's sound */
static uint8_t twr_sndgen;                          /* (a sound swapped: the screen redraws) */
static uint32_t twr_seen[NTRK], twr_lit[NTRK];      /* the layers' lights: the notes seen, the frame of the last */

static const char *const TW_LAYER_NAME[NTRK] = {"DRONE", "OST", "BASS", "LEAD", "CALLS", "DRUMS"};

UI_SMALL static void twr_load(uint32_t n)
{
    const twr_preset_t *p = &TWR_PRESETS[n % TWR_NPRESETS];
    uint32_t i, e, k;
    twr_cur = (uint8_t)(n % TWR_NPRESETS);
    for (i = 0; i < TW_LAYERS; i++) {
        track_t *t = &trk[i];
        if (!twr_find(p->snd[i], &e, &k)) {
            if (e != t->eng_req)
                set_engine_of(t, e);
            apply_preset_to(t, k);
        }
        t->p[P_ROOT] = p->root;
        t->p[P_SCALE] = p->scale;
        t->p[P_LEVEL] = TW_LEVEL[i];
        t->p[P_AMODE] = 0;                          /* (the conductor plays the figures: no arp under them) */
    }
    trk[TW_LEAD].p[P_QUANT] = 2;                    /* WHITE: the white keys walk the mode */
    TDRUM->p[P_E0] = (int16_t)twr_kit();
    song.g[G_DRLVL] = TW_DRUM_LEVEL;
    song.g[G_BPM] = p->bpm;
    song.g[G_AMB] = p->amb;
    song.g[G_CRKL] = p->crkl;
    twr.k[3] = (uint8_t)p->amb;
    if (song.sel != TW_LEAD)
        track_select(TW_LEAD);
    sync_reload = 1;
    twr.req = (uint8_t)(twr_cur + 1u);               /* the conductor takes it at its next block */
    ui.force = 1;
}

/* the preset's key, mode and mix on layer i again (a sound loaded resets them) */
UI_SMALL static void twr_fit(uint32_t i)
{
    const twr_preset_t *p = &TWR_PRESETS[twr_cur % TWR_NPRESETS];
    track_t *t = &trk[i % TW_LAYERS];
    t->p[P_ROOT] = p->root;
    t->p[P_SCALE] = p->scale;
    t->p[P_LEVEL] = TW_LEVEL[i % TW_LAYERS];
    t->p[P_AMODE] = 0;
    if (i == TW_LEAD)
        t->p[P_QUANT] = 2;
}

UI_SMALL static uint32_t twr_bank_of(const track_t *t)   /* the PRESETS list's place of t's sound (NBANK: none) */
{
    uint32_t n;
    if (!bank_ready)
        bank_resolve();
    for (n = 0; n < NBANK; n++)
        if (BANK[n].e == t->eng_req && bank_pi[n] == t->preset)
            return n;
    return NBANK;
}

UI_SMALL static void twr_swap(uint32_t i, int32_t s)      /* PRESETS on a layer: its next / previous sound */
{
    if (i == TRK_DRUM) {
        TDRUM->p[P_E0] = (int16_t)(((int32_t)TDRUM->p[P_E0] + s % (int32_t)DRUM_KITS + (int32_t)DRUM_KITS) % (int32_t)DRUM_KITS);
        sync_reload = 1;
    } else if (i < TW_LAYERS) {
        uint32_t n = twr_bank_of(&trk[i]);
        if (n >= NBANK)
            n = 0;
        n = (uint32_t)(((int32_t)n + s % (int32_t)NBANK + (int32_t)NBANK) % (int32_t)NBANK);
        if (!preset_go_to(&trk[i], n))
            twr_fit(i);
    }
    twr_sndgen++;
    ui.force = 1;
}

UI_SMALL static void twr_shuffle(void)              /* REC: every layer another sound of its kind */
{
    uint32_t i, n, c, cnt, pick, seed = fm1_ms * 2654435761u;
    for (i = 0; i < TW_LAYERS; i++) {
        uint32_t cur = twr_bank_of(&trk[i]), kind;
        if (cur >= NBANK)
            continue;
        kind = BANK[cur].kind;
        for (cnt = 0, n = 0; n < NBANK; n++)
            cnt += BANK[n].kind == kind && n != cur && bank_pi[n] != 0xFFu;
        if (!cnt)
            continue;
        pick = gurdy_hash(seed + i * 0x9E37u) % cnt;
        for (c = 0, n = 0; n < NBANK; n++)
            if (BANK[n].kind == kind && n != cur && bank_pi[n] != 0xFFu && c++ == pick)
                break;
        if (n < NBANK && !preset_go_to(&trk[i], n))
            twr_fit(i);
    }
    twr_sndgen++;
    ui_message("SHUFFLED");
    ui.force = 1;
}

UI_SMALL static void twr_boot(void)                 /* felucca_init: the first preset, stopped */
{
    twr_load(0);
    song.g[G_MIDI] = 1;                             /* SYSTEM > OUT = SEQ: what the conductor plays goes out too */
}

/* the main loop's input while this screen is up (ui_input, before the pages). 1: taken (ui_input stops) */
UI_SMALL static int twr_input(uint32_t *pressed, uint32_t home)
{
    uint32_t id, b, k;
    int32_t s;
    if (!twr_scr) {                                 /* Paleotone's pages: HOME tapped comes back */
        if (home == BT_TAP && !ui.menu && ui.layer == LY_PLAY && !ui.hold_kind) {
            twr_scr = 1;
            vis_on = 0;
            ui.force = 1;
            return 1;
        }
        return 0;
    }
    if (ui.menu || home == BT_HOLD || ui.layer != LY_PLAY || ui.hold_kind)
        return 0;                                   /* the menu, a layer held: Paleotone's */
    if (home == BT_TAP) {                           /* to Paleotone's TRACKS screen (the mix) */
        twr_scr = 0;
        go_home();
        return 1;
    }
    for (id = 0; id < FM1_NBTN; id++) {
        if (!((*pressed >> id) & 1u))
            continue;
        b = panel_btn_of(id);
        switch (b) {
        case B_PLAY:
            if (!ft_owns_press())
                transport_req = song.playing ? 2 : 1;
            break;
        case B_OCTDN:
        case B_OCTUP:
            song.octave += b == B_OCTDN ? (song.octave > -3 ? -1 : 0) : (song.octave < 3 ? 1 : 0);
            ui.force = 1;
            break;
        case B_REC:
            twr_shuffle();
            break;
        case B_HOME:
        case B_SAVE:
            break;
        default:                                    /* a sound's page: Paleotone's, for the selected layer */
            twr_scr = 0;
            ui.force = 1;
            return 0;
        }
    }
    *pressed = 0;
    if ((s = panel_enc(EN_PRESET)) != 0 && twr_focus) {
        twr_swap(twr_focus - 1u, s);
    } else if (s) {
        twr_load((uint32_t)(((int32_t)twr_cur + s % (int32_t)TWR_NPRESETS + (int32_t)TWR_NPRESETS) % (int32_t)TWR_NPRESETS));
        if (song.playing)
            twr.was_playing = 0;                    /* (the arc from the top, at the next block) */
    }
    if ((s = panel_enc(EN_SELECT)) != 0) {
        song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + accel(EN_SELECT, s, 200), GP[G_BPM].min, GP[G_BPM].max);
        ui.force = 1;
    }
    if ((s = panel_enc(EN_ALGO)) != 0) {                /* PRESET, then the layers */
        twr_focus = (uint8_t)clamp((int32_t)twr_focus + s, 0, NTRK);
        track_select(twr_focus ? twr_focus - 1u : TW_LEAD);
        ui.force = 1;
    }
    for (k = 0; k < 4u; k++) {
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        twr.k[k] = (uint8_t)clamp((int32_t)twr.k[k] + accel(EN_K1 + k, s, 127), 0, 127);
        if (k == 3u)
            song.g[G_AMB] = twr.k[3];
        ui.force = 1;
    }
    return 1;
}

UI_SMALL static void twr_text_c(int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(120 - text_w(f, s) / 2, y, f, s, c);
}

/* the screen: the preset and its key, where the arc is, the layers' lights, the knobs */
UI_SMALL static void twr_draw(void)
{
    const twr_preset_t *p = &TWR_PRESETS[twr_cur % TWR_NPRESETS];
    static uint32_t sig_was = 0xFFFFFFFFu;
    uint32_t i, sig, lights = 0;
    char b[32];
    if (!twr_shown_scr) {
        lcd_fill(0, 0, 240, 240, C_BLACK);
        twr_shown_scr = 1;
        ui.force = 1;
    }
    for (i = 0; i < NTRK; i++) {                    /* a light stays ~8 frames after a note */
        if (twr_cnt[i] != twr_seen[i]) {
            twr_seen[i] = twr_cnt[i];
            twr_lit[i] = ui.frame | 1u << 31;
        }
        if ((twr_lit[i] >> 31) && ((ui.frame - twr_lit[i]) & 0x7FFFFFFFu) < 8u)
            lights |= 1u << i;
    }
    sig = (uint32_t)twr_cur | (uint32_t)song.playing << 5 | (uint32_t)twr.sec << 6 | (uint32_t)twr.sbar << 8 |
          lights << 14 | (uint32_t)song.sel << 20 | (uint32_t)twr.pat << 23 | (uint32_t)(song.octave + 3) << 26;
    sig ^= ((uint32_t)twr_focus << 29) ^ ((uint32_t)twr_sndgen * 0x9E3779B9u) ^ (ui.msg_t ? 0x5A5A5A5Au : 0u) ^
           (uint32_t)TDRUM->p[P_E0] << 16;
    if (sig == sig_was && !ui.force)
        return;
    sig_was = sig;

    /* the title, the preset, its key and tempo */
    cv_begin(240, 96, C_BLACK);
    cv_text(6, 4, &FONT_S, "T-WRECKS", C_GRAY);
    str_cpy(b, "", sizeof b);
    fmt_int(b, song.g[G_BPM]);
    str_cpy(b + str_len(b), " BPM", sizeof b - str_len(b));
    cv_text(234 - text_w(&FONT_S, b), 4, &FONT_S, b, C_AMB);
    cv_rect(0, 24, 240, 1, C_LINE);
    twr_text_c(34, &FONT_L, p->name, twr_focus ? C_GRAY : C_WHITE);
    if (ui.msg_t) {                                 /* (SHUFFLED, ...) */
        twr_text_c(72, &FONT_S, ui.msg, C_WHITE);
    } else if (twr_focus) {                         /* the layer PRESETS swaps, and its sound */
        uint32_t li = twr_focus - 1u;
        str_cpy(b, TW_LAYER_NAME[li], sizeof b);
        str_cpy(b + str_len(b), ": ", sizeof b - str_len(b));
        if (li == TRK_DRUM)
            str_cpy(b + str_len(b), DRUM_KIT_NAMES[drum_kit()], sizeof b - str_len(b));
        else
            str_cpy(b + str_len(b), ENGINES[trk[li].eng_req % NENGINES]->presets[trk[li].preset %
                    (ENGINES[trk[li].eng_req % NENGINES]->npresets ? ENGINES[trk[li].eng_req % NENGINES]->npresets : 1u)].name,
                    sizeof b - str_len(b));
        twr_text_c(72, &FONT_S, b, C_WHITE);
    } else {
        str_cpy(b, N_NOTE[p->root % 12u], sizeof b);
        str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
        str_cpy(b + str_len(b), N_SCALE[p->scale % NSCALES], sizeof b - str_len(b));
        str_cpy(b + str_len(b), "   ", sizeof b - str_len(b));
        fmt_int(b + str_len(b), twr_cur + 1);
        str_cpy(b + str_len(b), "/", sizeof b - str_len(b));
        fmt_int(b + str_len(b), (int32_t)TWR_NPRESETS);
        twr_text_c(72, &FONT_S, b, C_AMB);
    }
    cv_blit(0, 0);

    /* the arc: its four sections, the bar in the section */
    cv_begin(240, 52, C_BLACK);
    for (i = 0; i < TW_SECTIONS; i++) {
        int on = song.playing && twr.sec == i;
        int32_t x = 4 + (int32_t)i * 58;
        cv_rect(x, 4, 54, 24, on ? C_HI : C_LINE);
        cv_rect(x + 1, 5, 52, 22, on ? C_HI : C_BLACK);
        cv_text(x + 27 - text_w(&FONT_S, TW_SEC_NAME[i]) / 2, 8, &FONT_S, TW_SEC_NAME[i], on ? C_BLACK : C_GRAY);
        if (on) {
            uint32_t w = 52u * ((uint32_t)twr.sbar + 1u) / TW_SEC_BARS[i];
            cv_rect(x + 1, 32, (int32_t)w, 4, C_AMB);
        }
    }
    if (!song.playing)
        twr_text_c(32, &FONT_S, "PLAY TO START", C_DIM);
    cv_blit(0, 100);

    /* the layers, two rows of three: a light as each one plays; the one the keys play in the accent */
    cv_begin(240, 44, C_BLACK);
    for (i = 0; i < NTRK; i++) {
        int32_t x = 8 + (int32_t)(i % 3u) * 80, y = 2 + (int32_t)(i / 3u) * 22;
        int lit = (lights >> i) & 1u;
        cv_rect(x, y + 3, 10, 10, lit ? C_HI : C_LINE);
        cv_text(x + 16, y, &FONT_S, TW_LAYER_NAME[i], song.sel == i ? C_WHITE : lit ? C_AMB : C_GRAY);
    }
    cv_blit(0, 152);

    /* the knobs */
    cv_begin(240, 44, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    {
        static const char *const KN[4] = {"DENSITY", "DRUMS", "CALLS", "AMB"};
        for (i = 0; i < 4u; i++) {
            int32_t x = (int32_t)i * 60;
            cv_text(x + 30 - text_w(&FONT_S, KN[i]) / 2, 6, &FONT_S, KN[i], C_GRAY);
            str_cpy(b, "", sizeof b);
            fmt_int(b, twr.k[i]);
            cv_text(x + 30 - text_w(&FONT_S, b) / 2, 24, &FONT_S, b, C_HI);
        }
    }
    cv_blit(0, 196);
}
