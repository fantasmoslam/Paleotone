/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca UI drawing: status bar (top), columns + gauges, graphs, focus readout,
 * footer (steps + engine / preset / page). */
static void draw_menu(void);
static uint32_t str_hash(uint32_t h, const char *s);

/* --------------------------------------------------------- drawing --- */
UI_SMALL static int is_eng_name(const char *s)                 /* one of the ENGINES[]->name strings */
{
    uint32_t e;
    for (e = 0; e < NENGINES; e++)
        if (s == ENGINES[e]->name)
            return 1;
    return 0;
}

/* at most 5 characters, and no wider than maxw */
UI_SMALL static void fit(char *d, const char *src, const felucca_font_t *f, int32_t maxw)
{
    str_cpy(d, src, is_eng_name(src) ? 8 : 6);           /* engine names are kept whole */
    while (d[0] && text_w(f, d) > maxw)
        d[str_len(d) - 1u] = 0;
}

UI_SMALL static int32_t batt_level(void)                         /* ADC ch3 thresholds */
{
    return song.batt_raw >= 591 ? 3 : song.batt_raw >= 561 ? 2 : song.batt_raw >= 531 ? 1 : 0;
}
/* charging (a USB host powers us; there is no charger status line): bars fill 1, 2, 3
 * every 0.6 s, so the header is redrawn twice a second at most; else the level */
UI_SMALL static int32_t batt_shown(void)
{
    if (usb.config && !usb.suspended)
        return 1 + (int32_t)((fm1_ms / 600u) % 3u);
    return batt_level();
}

/* top bar: transport, BPM, octave | USB, battery, CPU; messages replace it */
/* FONT_S drawn k times as big (whole pixels, no blend: Terminus is 1-bit); returns the end x */
#ifdef PALEO_CLAYBORN
/* PALEOTONE: k = 2..4 in Clayborn's own cuts (gfx.c), else Terminus k times */
static const felucca_font_t *font_k(uint32_t k)
{
    return k == 2u ? &FONT_C2 : k == 3u ? &FONT_C3 : k == 4u ? &FONT_C4 : &FONT_S;
}
#endif
UI_SMALL static int32_t text_k(int32_t x, int32_t y, const char *s, uint16_t c, uint32_t k)
{
    const felucca_font_t *f = &FONT_S;
#ifdef PALEO_CLAYBORN
    if (k >= 2u && k <= 4u)
        return cv_text(x, y, font_k(k), s, c);
#endif
    uint8_t ci = (uint8_t)cv_ci(c);
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), w = f->bw[gi], gx, gy, a, b;
        const uint8_t *gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t i = gy * w + gx;
                if ((gd[i >> 3] >> (7u - (i & 7u))) & 1u)
                    for (a = 0; a < k; a++)
                        for (b = 0; b < k; b++)
                            cv_pseti(x + ((int32_t)gx - (int32_t)f->pad) * (int32_t)k + (int32_t)b,
                                     y + (int32_t)(gy * k + a), ci);
            }
        x += (int32_t)(f->adv[gi] * k);
    }
    return x;
}
UI_SMALL static void draw_head(void)
{
    char b[16];
    uint32_t i;
    int32_t x;
    uint32_t rec = (song.rec >> song.sel) & 1u ? 2u : song.rec != 0u;   /* 2 the selected track armed, 1 another */
    uint32_t sig = (uint32_t)song.playing * 3u + rec * 5u + (uint32_t)(song.octave + 8) * 11u + song.sel * 13131u +
                   (ui.msg_t ? str_hash(7u, ui.msg) : 0u) + (uint32_t)song.g[G_BPM] * 101u + (ui.bpm_t != 0) * 31u +
                   (uint32_t)batt_shown() * 7777u + (usb.config && !usb.suspended) * 99991u + mlu.on * 7919u;
    if (!ui.force && sig == ui.head_sig)
        return;
    ui.head_sig = sig;
    cv_begin(240, H_TOP, C_BLACK);
    /* RAPTOR (the owner: everything larger): the header's text twice as big, in H_TOP rows */
    if (ui.msg_t) {                                   /* a message: 2x if it fits, else as before */
        if (text_w(&FONT_S, ui.msg) * 2 <= 232)
            text_k(4, -2, ui.msg, C_HI, 2u);
        else
            cv_text(4, 6, &FONT_S, ui.msg, C_HI);
        cv_blit(0, Y_HEAD);
        return;
    }
    if (song.playing) {                               /* > play, square stop: 16 px */
        for (i = 0; i < 8u; i++)
            cv_rect(4 + (int32_t)i * 2, 6 + (int32_t)i, 2, 16 - 2 * (int32_t)i, C_WHITE);
    } else {
        cv_rect(4, 7, 14, 14, C_HI);
    }
    if (rec)                                          /* recording armed: red = this track, gray = another */
        te_disc(28, 14, 6, rec == 2u ? TE_RED : C_GRAY);
    fmt_int(b, song.g[G_BPM]);
    x = text_k(40, -2, b, ui.bpm_t ? C_WHITE : C_HI, 2u);   /* the BPM (white while SELECT turns it) */
    if (mlu.on) {                                     /* MIDI LEARN on (2.6, GLO + F#3): in the octave's place */
        cv_text(x + 8, 6, &FONT_S, "LEARN", C_AMB);
    } else if (song.octave) {
        str_cpy(b, song.octave > 0 ? "+" : "", 4);
        fmt_int(b + str_len(b), song.octave);
        cv_text(x + 8, 9, &FONT_S, "OCT", C_GRAY);
        text_k(x + 36, -2, b, C_HI, 2u);
    }
    b[0] = (char)('1' + song.sel);                    /* the selected track: its number in its colour */
    b[1] = 0;
    cv_rect(170, 2, 24, 24, TE_COL[TSLOT(song.sel)]);
    text_k(174, -2, b, C_BLACK, 2u);
    {   /* battery, 3 bars, 2x; USB when a host is there: a dot */
        int32_t lvl = batt_shown(), k;
        int32_t bx = 236 - 30;
        cv_rect(bx, 7, 27, 2, C_GRAY);
        cv_rect(bx, 19, 27, 2, C_GRAY);
        cv_rect(bx, 7, 2, 14, C_GRAY);
        cv_rect(bx + 25, 7, 2, 14, C_GRAY);
        cv_rect(bx + 27, 11, 3, 6, C_GRAY);
        for (k = 0; k < lvl; k++)
            cv_rect(bx + 4 + k * 7, 11, 5, 6, lvl == 1 && batt_level() <= 1 ? C_WHITE : C_HI);
        if (usb.config && !usb.suspended)
            cv_rect(bx - 8, 12, 4, 4, C_DIM);
    }
    cv_blit(0, Y_HEAD);
}
/* full redraw: the strips (head, columns, graph, foot) cover the rest, so only
 * the space between them is cleared, then the rules are drawn */
UI_SMALL static void draw_frame(void)
{
    uint32_t i;
    lcd_fill(0, H_TOP, 240, Y_LABEL - H_TOP, C_BLACK);
    lcd_fill(0, Y_SEP_END, 240, Y_GRAPH - Y_SEP_END, C_BLACK);
    lcd_fill(0, Y_GRAPH + H_GRAPH, 240, Y_FOOT - Y_GRAPH - H_GRAPH, C_BLACK);
    for (i = 0; i < 4u; i++)                            /* left inset of each column */
        lcd_fill(i * 60u, Y_LABEL, 4, Y_SEP_END - Y_LABEL, C_BLACK);
    lcd_fill(0, H_TOP, 240, 1, C_LINE);
    lcd_fill(0, Y_FOOT - 2, 240, 1, C_LINE);
    for (i = 1; i < 4u; i++)
        lcd_fill(i * 60u - 1u, H_TOP + 4, 1, Y_SEP_END - H_TOP - 4, C_LINE);
}

/* one column: [icon] LABEL / value unit / gauge, redrawn only when it changed.
 * ratio: 0..1000 for the gauge, -1 = no gauge. icon: ICON_* (icons.c), ICON_AUTO = by label */
#define LABEL_X (FELUCCA_ICONS ? ICON_CELL + ICON_GAP : 0)
UI_SMALL static int big_page(const page_t *pg);           /* (below) */
UI_SMALL static void draw_column(uint32_t c, const char *label, const char *val, const char *unit, uint16_t vc,
                        int32_t ratio, uint32_t icon)
{
    char l[8], v[8], u[8], key[32];
    int32_t x, gw = 52, fx;
    if (icon == ICON_AUTO)
        icon = icon_for_label(label);
    fit(l, label, &FONT_S, 54 - LABEL_X);
    fit(v, val, &FONT_S, is_eng_name(val) ? 56 : 40);   /* engine names whole */
    fit(u, unit, &FONT_S, 54 - text_w(&FONT_S, v) - 3);
    str_cpy(key, l, 8);                                 /* cache key: texts + colour + gauge */
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), v, 8);
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), u, 8);
    {
        uint32_t n = str_len(key);
        key[n] = (char)('A' + (vc == C_WHITE) + (vc == C_DIM) * 2);
        key[n + 1] = (char)(' ' + (ratio < 0 ? 0 : 1 + ratio / 20));
        key[n + 2] = (char)(icon == ICON_NONE ? '~' : '!' + icon % 90u);   /* same label, other icon */
        key[n + 3] = 0;
    }
    if (c < 4u) {                                       /* (the big values: their own fit) */
        fit(ui.big_l[c], label, &FONT_L, 112);         /* (its own fit: the cell, not the small column) */
        fit(ui.big_v[c], val, &FONT_L, 112 - (unit[0] ? 4 : 0));   /* (graph_big scales it to its cell) */
        str_cpy(ui.big_u[c], unit, 8);
        ui.big_c[c] = vc;
        ui.big_r[c] = (int16_t)ratio;
    }
    if (c == ui.hot_col) {
        str_cpy(ui.focus_l, l, 8);
        str_cpy(ui.focus_v, v, 8);
        str_cpy(ui.focus_u, u, 8);
    }
    if (!ui.home && big_page(cur_page())) {             /* RAPTOR: a page without a graph shows its values once, large
                                                         * (graph_big, over this strip too): nothing drawn here */
        ui.col[c][0] = 0;                               /* (the next page's strip is drawn again) */
        return;
    }
    if (!ui.force && str_eq(key, ui.col[c]))
        return;
    str_cpy(ui.col[c], key, sizeof ui.col[c]);
    cv_begin(55, Y_SEP_END - Y_LABEL, C_BLACK);         /* x 4..58: the rule at 59 stays */
#ifdef PALEO_CLAYBORN
    if (label[0] && text_w(&FONT_M, label) <= 55)       /* PALEOTONE: the label in M (no icon) where it fits */
        cv_text(0, -3, &FONT_M, label, C_GRAY);
    else
#endif
    {
    if (FELUCCA_ICONS && icon != ICON_NONE && l[0])
        cv_icon(0, 1, icon, KN_COL[c & 3u]);            /* icon rows 1..10 = the label's cap height */
    cv_text(l[0] ? LABEL_X : 0, 0, &FONT_S, l, C_GRAY);
    }
    if (text_w(&FONT_L, v) <= 54) {                     /* RAPTOR: the value large when it fits the column (the unit
                                                         * small beside it, if there is room) */
        x = cv_text(0, 12, &FONT_L, v, vc);
        if (x + 2 + text_w(&FONT_S, u) <= 55)
            cv_text(x + 2, 20, &FONT_S, u, C_DIM);
    } else {
        x = cv_text(0, Y_VALUE - Y_LABEL, &FONT_S, v, vc);
        cv_text(x + 3, Y_VALUE - Y_LABEL, &FONT_S, u, C_DIM);
    }
    if (ratio >= 0) {                                   /* gauge: track, fill, 1 px end line */
        int32_t gy = Y_GAUGE - Y_LABEL;
        fx = ratio * gw / 1000;
        cv_rect(0, gy + 1, gw, 1, C_LINE);
        cv_rect(0, gy, fx, 3, vc == C_DIM ? C_DIM : KN_DIM[c & 3u]);
        cv_rect(fx, gy - 1, 1, 5, vc == C_DIM ? C_HI : vc);
    }
    cv_blit(c * 60u + 4u, Y_LABEL);
}

/* Matches voice.c: attack is linear, decay and release are exponential
 * (env += (target - env) * k each tick, ~99 % after the set time). Time
 * axis is the parameter value (the times themselves are exponential). */
UI_SMALL static void graph_adsr(const track_t *t, uint16_t c)
{
    int32_t a = 4 + t->p[P_ATK] * 50 / 127, d = 6 + t->p[P_DEC] * 50 / 127, r = 6 + t->p[P_REL] * 60 / 127;
    int32_t top = 8, bot = 90, sus = t->p[P_SUS] * 1000 / 127;          /* 0..1000 */
    int32_t x0 = 6, x1 = x0 + a, x3 = 232 - r, i, px, py;
    int32_t e = 32768;                                                  /* exp(-4.6 u), Q15 */
#define EGY(lvl) (bot - (lvl) * (bot - top) / 1000)
    cv_line(x0, bot, x1, top, c);                                       /* attack: linear */
    px = x1;
    py = top;
    for (i = 1; i <= d; i++) {                                          /* decay: exponential to SUS */
        int32_t lvl;
        e = (e * (32768 - 150733 / d)) >> 15;           /* k^d = exp(-4.6) */
        lvl = sus + ((1000 - sus) * e >> 15);
        cv_line(px, py, x1 + i, EGY(lvl), c);
        px = x1 + i;
        py = EGY(lvl);
    }
    cv_line(px, py, x3, EGY(sus), c);                                    /* sustain */
    px = x3;
    py = EGY(sus);
    e = 32768;
    for (i = 1; i <= r; i++) {                                          /* release: exponential to 0 */
        e = (e * (32768 - 150733 / r)) >> 15;
        cv_line(px, py, x3 + i, EGY(sus * e >> 15), c);
        px = x3 + i;
        py = EGY(sus * e >> 15);
    }
    cv_line(0, bot + 1, 239, bot + 1, C_LINE);
#undef EGY
}

UI_SMALL static void graph_lfo(const track_t *t, uint16_t c)
{
    int32_t x, py = 50;
    uint32_t ph = (uint32_t)t->p[P_LPHASE] << 25;
    for (x = 0; x < 240; x++) {                      /* (lfo_wave only reads the track) */
        int32_t y = 50 - lfo_wave((track_t *)t, ph + (uint32_t)x * (0xFFFFFFFFu / 120u)) * 38 / 32768;
        if (t->p[P_LWAVE] == 4)
            y = 50 - ((int32_t)((x / 20 * 2654435761u) >> 16) - 32768) * 38 / 32768;
        if (x)
            cv_line(x - 1, py, x, y, c);
        py = y;
    }
    cv_line(0, 50, 239, 50, C_LINE);
}

UI_SMALL static void graph_steps(const track_t *t, uint16_t c)
{
    uint32_t i, len = (uint32_t)t->p[P_SLEN];
    for (i = 0; i < NSTEP; i++) {                    /* 4 rows of 16 thin bars */
        int32_t x = 6 + (int32_t)(i % 16u) * 14 + (int32_t)(i % 16u) / 4 * 4, y = 6 + (int32_t)(i / 16u) * 24;
        const step_t *st = &t->step[i];
        if (i >= len)
            continue;
        if (step_on(st))
            cv_rect(x, y, 1, 14, c);
        else if (st->time == ST_TIE)
            cv_rect(x, y + 6, 1, 8, C_DIM);
        else
            cv_rect(x, y + 13, 1, 1, C_DIM);
        if (st->flags & SF_ACCENT)
            cv_rect(x - 1, y - 2, 3, 1, c);
        if ((song.playing && i == t->seq_idx) || i == ui.cursor)
            cv_rect(x - 1, y + 16, 3, 3, C_WHITE);
    }
}
/* STEP page: the cursor's 16-step bank as a little piano roll */
/* (2.7.2) SEQ > NOTES: the pattern as a piano roll (cv_oy 0, the whole strip). Each note a bar as long as its
 * step and TIEs, its FORCE as its fill (GHOST dark, SOFT hollow, NORM / HARD full, ACC a white cap), a SLIDE's line to
 * the next; the note under edit outlined in white, its name over it; the C rows named on the left, the beats ruled;
 * the playhead a line. The rows follow the note under edit when the pattern spans more than 18 semitones */
UI_SMALL static void graph_roll(const track_t *t, uint16_t c)
{
    uint32_t len = trk_len(t), nv = NV_ZOOM[clamp(1 + (int8_t)ui.zoom, 0, 3)], v0, i, j;
    int32_t lo = 127, hi = 0, x0 = 22, w = 240 - 22 - 2, top = 14, bot = H_GRAPH - 4 - (ui.sheet ? 18 : 0), rh, sel_note = last_note, cx;
    const step_t *cs = &t->step[ui.cursor];
    char b[12];
    if (nv > len)
        nv = len;
    v0 = ui.cursor / nv * nv;
    for (i = 0; i < len; i++)
        for (j = 0; j < t->step[i].n; j++) {
            if (t->step[i].note[j] < lo) lo = t->step[i].note[j];
            if (t->step[i].note[j] > hi) hi = t->step[i].note[j];
        }
    if (step_on(cs))
        sel_note = cs->note[ui.note_k % cs->n];
    if (lo > hi)
        lo = hi = sel_note;
    if (hi - lo < 11) {                                 /* (an octave at least: rows of a sane height) */
        lo = clamp((lo + hi) / 2 - 6, 0, 115);
        hi = lo + 11;
    }
    if (hi - lo > 17) {                                 /* (a wide pattern: 18 rows around the note under edit) */
        lo = clamp(sel_note - 8, lo, hi - 17);
        hi = lo + 17;
    }
    lo = clamp(lo - 1, 0, 127), hi = clamp(hi + 1, 0, 127);
    rh = (bot - top) / (hi - lo + 1);
    fmt_int(b, (int32_t)v0 + 1);
    str_cpy(b + str_len(b), "-", 2);
    fmt_int(b + str_len(b), (int32_t)(v0 + nv > len ? len : v0 + nv));
    cv_text(2, 0, &FONT_S, b, C_GRAY);
    str_cpy(b, "ZOOM ", 12);
    fmt_int(b + 5, (int32_t)nv);
    cv_text(238 - text_w(&FONT_S, b), 0, &FONT_S, b, C_DIM);
    for (i = (uint32_t)lo; i <= (uint32_t)hi; i++) {    /* rows: the black keys' bands, the C's ruled and named */
        int32_t y = bot - ((int32_t)i - lo + 1) * rh;
        uint32_t pc = i % 12u;
        if ((0x54Au >> pc) & 1u)
            cv_rect(x0, y, w, rh, TE_G1);
        if (!pc) {
            note_name(b, i);
            cv_text(0, y + rh / 2 - 6, &FONT_S, b, C_DIM);
            cv_rect(x0, y + rh - 1, w, 1, C_GRAY);
        }
    }
    for (i = 0; i <= nv; i++)                           /* the beats (4 steps), a tick on each step */
        if (i % 4u == 0u)
            cv_rect(x0 + (int32_t)i * w / (int32_t)nv, top, 1, bot - top, i % 16u ? C_DIM : C_GRAY);
        else if (nv <= 32u)
            cv_rect(x0 + (int32_t)i * w / (int32_t)nv, bot + 1, 1, 2, C_DIM);
    for (i = 0; i < len; i++) {                         /* the notes (one starting before the view: its part in it) */
        const step_t *st = &t->step[i];
        uint32_t n = note_len(t, i), f;
        int32_t xa, xb, y;
        if (!step_on(st) || i + n <= v0 || i >= v0 + nv)
            continue;
        xa = x0 + ((int32_t)i - (int32_t)v0) * w / (int32_t)nv + 1;
        xb = x0 + ((int32_t)(i + n) - (int32_t)v0) * w / (int32_t)nv - 1;
        xa = xa < x0 ? x0 : xa;
        xb = xb > x0 + w ? x0 + w : xb;
        for (j = 0; j < st->n; j++) {
            if (st->note[j] < lo || st->note[j] > hi)
                continue;
            y = bot - ((int32_t)st->note[j] - lo + 1) * rh + 1;
            f = note_force(st, j);
            cv_rect(xa, y, xb - xa, rh - 1, f ? c : TE_DIM[TSLOT(song.sel)]);
            if (f == 1u && rh > 4)                      /* SOFT: hollow */
                cv_rect(xa + 1, y + 1, xb - xa - 2, rh - 3, C_BLACK);
            if (f == 4u)                                /* ACC: a white cap */
                cv_rect(xa, y, xb - xa, 2, C_WHITE);
            if ((st->flags & SF_SLIDE) && j == 0u)
                cv_rect(xb, y + rh / 2 - 1, 4, 2, c);
            if (i == ui.cursor && j == ui.note_k % st->n) {   /* the note under edit */
                cv_rect(xa - 2, y - 2, xb - xa + 4, 1, C_WHITE), cv_rect(xa - 2, y + rh - 1, xb - xa + 4, 1, C_WHITE);
                cv_rect(xa - 2, y - 2, 1, rh + 2, C_WHITE), cv_rect(xb + 1, y - 2, 1, rh + 2, C_WHITE);
                note_name(b, st->note[j]);
                cv_text(xa, y - 15 < top ? y + rh + 1 : y - 15, &FONT_S, b, C_WHITE);
            }
        }
    }
    cx = x0 + ((int32_t)ui.cursor - (int32_t)v0) * w / (int32_t)nv;
    if (!step_on(cs) && sel_note >= lo && sel_note <= hi) {   /* an empty step: a frame where a note would go */
        int32_t y = bot - (sel_note - lo + 1) * rh + 1, xw = w / (int32_t)nv - 2;
        cv_rect(cx + 1, y - 1, xw, 1, C_WHITE), cv_rect(cx + 1, y + rh - 1, xw, 1, C_WHITE);
        cv_rect(cx + 1, y - 1, 1, rh, C_WHITE), cv_rect(cx + xw, y - 1, 1, rh, C_WHITE);
    }
    if (song.playing && t->seq_idx >= v0 && t->seq_idx < v0 + nv) {   /* the playhead */
        int32_t x = x0 + ((int32_t)t->seq_idx - (int32_t)v0) * w / (int32_t)nv;
        cv_rect(x, top, 1, bot - top, C_HI);
        cv_rect(x - 2, top, 5, 2, C_HI);
    }
    if (ui.sheet) {                                     /* OCT+ held: the note's sheet */
        char nu[8];
        const char *val[6] = {cs->flags & SF_SLIDE ? "ON" : "OFF", nu, FILL_N[step_fill(t, ui.cursor) % 3u], "", "", ""};
        fmt_int(nu, t->micro[ui.cursor]);
        if (t->micro[ui.cursor] > 0) { nu[0] = '+'; fmt_int(nu + 1, t->micro[ui.cursor]); }
        sheet_bar(H_GRAPH - 16, SHEET_N, val, ui.sheet - 1u, TE_DIM[TSLOT(song.sel)]);
    }
}
UI_SMALL static void graph_scale(const track_t *t, uint16_t c)
{
    static const uint8_t BLACK[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    uint32_t i, mask = scale_mask(t);
    for (i = 0; i < 12u; i++) {
        uint32_t deg = (i + 12u - (uint32_t)t->p[P_ROOT]) % 12u;
        int32_t x = 6 + (int32_t)i * 19;
        uint16_t col = (mask >> deg) & 1u ? (deg == 0u ? C_WHITE : c) : C_DIM;
        cv_rect(x, BLACK[i] ? 10 : 40, 16, 1, col);
        cv_rect(x + 7, BLACK[i] ? 10 : 40, 1, 40, col);
    }
}

UI_SMALL static void graph_fx(const track_t *t, uint16_t c)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t h = t->p[P_DIST + i] * 80 / 127, x = (int32_t)i * 60 + 28;
        cv_rect(x, 10, 1, 80, C_LINE);
        cv_rect(x, 90 - h, 1, h, c);
        cv_rect(x - 3, 90 - h, 7, 1, c);
    }
}
/* SLICER page: the pattern's 16 steps, a 'x' step a full bar; a '.' step: GATE a bar as high as
 * it stays open (DEPTH), STUT hatched (it repeats the last 'x'); the step playing underlined.
 * Grey when the SLICER is OFF. */
UI_SMALL static void graph_slicer(const track_t *t, uint16_t c)
{
    uint32_t i, pat = sl_pattern(t), mode = (uint32_t)t->p[P_SLCR], cur = sl[t - trk].idx;
    int32_t open = 70 - t->p[P_SLDEPTH] * 70 / 127;      /* px a closed GATE step keeps */
    uint16_t col = mode == SL_OFF ? C_DIM : c;
    for (i = 0; i < 16u; i++) {
        int32_t x = 4 + (int32_t)i * 14 + (int32_t)(i / 4u) * 2, y;
        if ((pat >> i) & 1u) {
            cv_rect(x, 10, 11, 70, col);
        } else if (mode == SL_STUT) {
            for (y = 10; y < 80; y += 4)
                cv_rect(x, y, 11, 1, col);
        } else {
            cv_rect(x, 79, 11, 1, C_LINE);
            if (open)
                cv_rect(x, 80 - open, 11, open, C_DIM);
        }
        if (mode != SL_OFF && i == cur)
            cv_rect(x, 85, 11, 3, C_WHITE);
    }
}
UI_SMALL static uint32_t steps_hash(const track_t *t)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < NSTEP; i++) {
        const step_t *st = &t->step[i];
        h = (h ^ (st->note[0] + st->n * 128u + st->time * 1024u + st->flags * 4096u + st->note[1] * 65536u)) *
            16777619u;
    }
    return h;
}

UI_SMALL static uint32_t str_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

/* a page drawn with its values large (graph_big) */
UI_SMALL static int big_page(const page_t *pg)            /* (2.6: the INSERT pages too: their middle stayed empty) */
{
    return (pg->graph == GR_NONE || pg->graph == GR_INS || pg->graph == GR_ADSR || pg->graph == GR_LFO ||
            pg->graph == GR_FX || pg->graph == GR_SLCR || pg->graph == GR_SCALE) && pg->scope != SC_SONG && pg->scope != SC_DRUM;
            /* RAPTOR: the curves and bars give way to the values, large (the owner: readability) */
}
UI_SMALL static uint32_t graph_signature(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint32_t h = 2166136261u, i;
    if (ui.hot_t && settings.zoom)
        h = str_hash(str_hash(str_hash(h ^ 0x5555u, ui.focus_v), ui.focus_l), ui.focus_u);
    if (!ui.home && big_page(pg))                    /* the big values: as the columns show them */
        for (i = 0; i < 4u; i++)
            h = str_hash(str_hash(str_hash(h ^ ui.big_c[i] * 31u, ui.big_v[i]), ui.big_l[i]), ui.big_u[i]);
    if (ui.home)
        return h ^ (ui.frame / 2u);                  /* scope: redraw every other frame */
    h ^= (uint32_t)pg->graph * 131u + TSEL->eng_req + song.sel * 7777u;
    for (i = 0; i < P_COUNT; i++)
        h = (h ^ (uint32_t)t->p[i]) * 16777619u;
    h ^= (uint32_t)TSEL->preset * 7u + (uint32_t)song.g[G_SLOT] * 13u + TSEL->user * 257u + up_gen * 7919u + ui.uslot * 104729u;
    {   /* (2.7) the favourites: the PRESETS list and its FAV column */
        uint32_t q;
        for (q = 0; q < FAV_N; q++) h = h * 31u + fav[q];
    }
    if (pg->graph == GR_SLCR && t->p[P_SLCR])        /* the SLICER's step playing */
        h ^= (sl[song.sel].idx + 1u) * 2654435761u;
    if (pg->graph == GR_SLOTS)                       /* (a checksum over each slot) */
        for (i = 0; i < 4u; i++)
            h ^= (uint32_t)project_used(i) << (20u + i);
    if (pg->graph == GR_STEPS || pg->graph == GR_ROLL) {
        uint32_t ph = song.playing ? t->seq_idx : 0xFFFFu;
        if (pg->graph == GR_ROLL)                    /* (2.7.2 NOTES: the note under edit, the zoom, the sheet) */
            h ^= ui.note_k * 104729u + ui.zoom * 7u + ui.sheet * 977u + (uint32_t)t->micro[ui.cursor] * 31337u +
                 step_fill(t, ui.cursor) * 3u + last_note * 65537u;
        h ^= steps_hash(t) + ph * 31u + ui.cursor * 7919u;
    }
    return h;
}
/* preset browser: the list by kind (ui.c BANK), the current one in white */
UI_SMALL static void graph_browse(void)
{
    uint32_t total, cur = preset_pos(&total), e, k;
    int32_t row;
    if (!total)
        return;
    for (row = -2; row <= 2; row++) {               /* RAPTOR: five rows, the selected one twice as big */
        static const int8_t RY[5] = {2, 20, 44, 82, 100};
        int32_t y = RY[row + 2];
        char tag[5], nm[13];
        int sel = row == 0;
        uint32_t n = (cur + total * 4u + (uint32_t)row) % total;
        e = preset_at(n, &k);
        if (e == NENGINES) {                             /* user preset: "U07" and its name */
            up_slot_label(tag, k);
            up_name(k, nm);
        } else {                                         /* its kind: BASS, KEYS, PAD... */
            str_cpy(tag, preset_kind(n), sizeof tag);
            str_cpy(nm, ENGINES[e]->presets[k].name, sizeof nm);
        }
        if (sel) {
            cv_text(4, y + 12, &FONT_S, tag, C_GRAY);
            text_k(44, y, nm, C_WHITE, 2u);
            continue;
        }
        cv_text(14, y, &FONT_S, tag, C_DIM);
        cv_text(54, y, &FONT_S, nm, C_GRAY);
    }
}

/* user preset slots around the selected one: "U07  NAME" / EMPTY, the selected one in white */
UI_SMALL static void graph_user(void)
{
    int32_t row, first = clamp((int32_t)ui.uslot - 3, 0, UP_SLOTS - 7);
    for (row = 0; row < 7; row++) {
        uint32_t k = (uint32_t)(first + row);
        int32_t y = 4 + row * 17;
        char tag[4], nm[13];
        int sel = k == ui.uslot, used = up_used(k);
        up_slot_label(tag, k);
        if (used)
            up_name(k, nm);
        else
            str_cpy(nm, "EMPTY", sizeof nm);
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_WHITE : C_GRAY);
        cv_text(54, y, &FONT_S, nm, used ? (sel ? C_WHITE : C_HI) : C_DIM);
    }
}

/* the sections A..D (the four slots): used / empty, the selected one in white */
UI_SMALL static void graph_slots(void)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t y = 8 + (int32_t)i * 26;
        char b[4];
        int sel = (int32_t)i + 1 == song.g[G_SLOT];
        b[0] = (char)('A' + i);
        b[1] = 0;
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, b, sel ? C_WHITE : C_GRAY);
        cv_text(40, y, &FONT_S, project_used(i) ? "USED" : "EMPTY", project_used(i) ? (sel ? C_WHITE : C_HI) : C_DIM);
    }
}

/* TRACKS page: six channel strips (RAPTOR; SLOOP: four) (mixer style), one under each column: number +
 * REC / ARM / MUTE, the sound's short name, then a level fader with the output meter
 * (note activity), the pattern over its LEN (time running down; bar width = notes in
 * the step) and the play head. The selected track is drawn bright. Every part is
 * its own small canvas with its own signature: while the transport runs only the
 * head markers and the meters move. (No ZOOM focus here.) */
#define TS_HEAD_Y (Y_GRAPH + 2)
#define TS_NAME_Y (Y_GRAPH + 20)
#define TS_Y (Y_GRAPH + 38)
#define TS_H 84                                      /* body: rows Y_GRAPH + 38 .. + 121 */
static struct {
    uint32_t head[NTRK], name[NTRK], fader[NTRK], ov[NTRK], mark[NTRK];
    uint8_t meter[NTRK];
} ts;

UI_SMALL static int32_t meter_px(int32_t a)                   /* |sample| (Q15) -> px: 6 dB = TS_H / 10 */
{
    int32_t lg = 0, v;
    if (a < 64)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);             /* 8 log2(a): 48 (-54 dB) .. 128 (+6 dB) */
    return clamp((v - 48) * (TS_H - 2) / 80, 0, TS_H - 2);
}

#define TS_CW (240u / NTRK)                         /* RAPTOR: the TRACKS page's column (six of 40 px; SLOOP's four: 60) */
UI_SMALL static uint32_t trk_level(uint32_t c)                /* LEVEL 0..127 (the drum track: GLO > DRUMS LEVEL) */
{
    return (uint32_t)(c == TRK_DRUM ? song.g[G_DRLVL] : trk[c].p[P_LEVEL]) & 127u;
}

UI_SMALL static void trk_short_name(uint32_t c, char *b)      /* the track's sound, b holds 13 */
{
    const track_t *t = &trk[c];
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    if (c == TRK_DRUM)
        str_cpy(b, "DRUM", 13);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        str_cpy(b, e->presets[t->preset % e->npresets].name, 13);
    else
        str_cpy(b, e->name, 13);
}

UI_SMALL static void draw_tracks(void)
{
    uint32_t c;
    if (ui.force) {
        lcd_fill(0, Y_GRAPH, 240, H_GRAPH, C_BLACK);
        for (c = 0; c < NTRK; c++)
            ts.meter[c] = 0;
    }
    for (c = 0; c < NTRK; c++) {
        track_t *t = &trk[c];
        uint32_t x0 = c * TS_CW + 2u, sel = c == song.sel, lvl = trk_level(c), mute = !lvl || t->p[P_MUTE];
        uint32_t arm = (song.rec >> c) & 1u, st = arm ? (song.playing ? 1u : 2u) : mute ? 3u : 0u, sig;
        uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u, row = 0xFFFFu;
        int32_t pk, m;
        char b[16];
        if (c == TRK_DRUM) {
            pk = drums.peak;
            drums.peak = 0;
        } else {
            pk = t->peak;
            t->peak = 0;
        }
        /* head: number (white = selected), REC / ARM / MUTE */
        sig = 1u + sel + st * 2u;
        if (ui.force || sig != ts.head[c]) {
            ts.head[c] = sig;
            b[0] = (char)('1' + c);
            b[1] = 0;
            cv_begin(TS_CW - 2u, 16, C_BLACK);
            cv_text(0, 0, &FONT_S, b, sel ? C_WHITE : C_GRAY);
            if (sel)
                cv_rect(0, 15, 8, 1, C_WHITE);
            if (st == 1u || st == 2u) {                 /* (RAPTOR: six columns of 40 px: a REC / ARM square, MUTE an M) */
                cv_rect(14, 5, 6, 6, st == 1u ? C_WHITE : C_AMB);
            } else if (st) {
                cv_text(14, 0, &FONT_S, "M", C_DIM);
            }
            cv_blit(x0, TS_HEAD_Y);
        }
        /* the sound: preset name (user preset, DRUM), cut to the column */
        trk_short_name(c, b);
        while (b[0] && text_w(&FONT_S, b) > (int32_t)TS_CW - 4)
            b[str_len(b) - 1u] = 0;
        sig = str_hash(2u + sel, b);
        if (ui.force || sig != ts.name[c]) {
            ts.name[c] = sig;
            cv_begin(TS_CW - 2u, 16, C_BLACK);
            cv_text(0, 0, &FONT_S, b, sel ? C_HI : C_DIM);
            cv_blit(x0, TS_NAME_Y);
        }
        /* fader (LEVEL) + meter of the output */
        m = mute ? 0 : meter_px(pk);
        if (m < ts.meter[c] - 3)
            m = ts.meter[c] - 3;                     /* falls ~20 dB/s */
        ts.meter[c] = (uint8_t)(m < 0 ? 0 : m);
        sig = 1u + lvl + ts.meter[c] * 128u + sel * 65536u + (sel && ui.hot_t && ui.hot_col == 1u) * 131072u +
              (t->p[P_MUTE] != 0) * 262144u;
        if (ui.force || sig != ts.fader[c]) {
            int32_t fy = (int32_t)(TS_H - 2u) - (int32_t)lvl * (TS_H - 4) / 127;
            ts.fader[c] = sig;
            cv_begin(12, TS_H, C_BLACK);
            cv_rect(3, 0, 1, TS_H, C_LINE);
            if (lvl) {
                cv_rect(2, fy, 3, TS_H - fy, C_DIM);
                cv_rect(0, fy - 1, 7, 2, (sig & 131072u) ? C_WHITE : sel ? C_HI : C_GRAY);
            } else {
                cv_rect(0, TS_H - 2, 7, 2, C_DIM);
            }
            if (ts.meter[c])
                cv_rect(9, TS_H - ts.meter[c], 3, ts.meter[c], C_AMB);
            cv_blit(x0, TS_Y);
        }
        /* the pattern: one band per step over LEN, a note step as wide as its notes */
        sig = steps_hash(t) * 31u + len * 7u + sel;
        if (ui.force || sig != ts.ov[c]) {
            uint32_t i;
            ts.ov[c] = sig;
            cv_begin(22, TS_H, C_BLACK);
            cv_rect(11, 0, 1, TS_H, C_LINE);
            for (i = 0; i < len; i++) {
                const step_t *s = &t->step[i];
                int32_t r0 = (int32_t)(i * TS_H / len), r1 = (int32_t)((i + 1u) * TS_H / len), h = r1 - r0;
                if (h > 2)
                    h--;                             /* a gap between steps when there is room */
                if (h < 1)
                    h = 1;
                if (step_on(s)) {
                    int32_t w = 2 + 4 * (int32_t)s->n;
                    cv_rect(11 - w / 2, r0, w, h, (s->flags & SF_ACCENT) && sel ? C_WHITE : sel ? C_HI : C_GRAY);
                } else if (s->time == ST_TIE) {
                    cv_rect(10, r0, 3, r1 - r0, C_DIM);
                }
            }
            cv_blit(x0 + 12u, TS_Y);
        }
        /* play head: a small arrow right of the pattern (white while recording) */
        if (song.playing)
            row = ((t->seq_idx % len) * 2u + 1u) * TS_H / (2u * len);
        sig = 1u + row + (st == 1u) * 65536u;
        if (ui.force || sig != ts.mark[c]) {
            int32_t i;
            ts.mark[c] = sig;
            cv_begin(5, TS_H, C_BLACK);
            if (row != 0xFFFFu)
                for (i = -2; i <= 2; i++) {
                    int32_t w = 5 - 2 * (i < 0 ? -i : i);
                    cv_rect(5 - w, (int32_t)row + i, w, 1, st == 1u ? C_WHITE : C_AMB);
                }
            cv_blit(x0 + 33u, TS_Y);
        }
    }
}

/* oscilloscope of the output, triggered on a rising zero crossing */
#include "scratch.h"                                /* sloop-lab: the snapshot in the main loop's shared scratch */
_Static_assert(sizeof main_scratch.snap == SCOPE_N * sizeof(int16_t), "the scratch holds the snapshot");
UI_SMALL static void graph_scope(uint16_t c)
{
    int16_t *snap = main_scratch.snap;
    uint32_t w = scope_w, i, trig = 0;
    int32_t py = 50, x, peak = 1500;
    for (i = 0; i < SCOPE_N; i++) {
        snap[i] = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (snap[i] > peak)
            peak = snap[i];
        else if (-snap[i] > peak)
            peak = -snap[i];
    }
    for (i = 1; i < SCOPE_N - 240u; i++)
        if (snap[i - 1] < 0 && snap[i] >= 0) {
            trig = i;
            break;
        }
    cv_line(0, 50, 239, 50, C_LINE);
    for (x = 0; x < 240; x++) {
        int32_t y = 50 - snap[trig + (uint32_t)x] * 44 / peak;   /* auto-scaled */
        if (x)
            cv_line(x - 1, py, x, y, c);
        py = y;
    }
}

/* SLOOP 2.4: a page without a graph (EDIT, VOICE, the DEST pages, GLOBAL, MASTER, SYSTEM...): its values
 * large in the empty space, 2 x 2 as the knobs (KNOB 1 2 / KNOB 3 4), the one turned in white; one value
 * alone (FILTER) across the width. The columns (draw_columns, drawn first) fill ui.big_* */
/* RAPTOR: a page without a graph: its four values, each once, large (the owner reads them from a distance): a 2 x 2
 * grid from under the header to the footer (Y_BIG .. + H_BIG), each cell the label in the knob's colour, the value in
 * FONT_L, its unit, and a thick gauge */
#define Y_BIG (H_TOP + 2)
#define H_BIG (Y_FOOT - 4 - Y_BIG)
#ifdef PALEO_CLAYBORN
#define TEXT_KW(s, k) ((k) >= 2 && (k) <= 4 ? text_w(font_k((uint32_t)(k)), (s)) : text_w(&FONT_S, (s)) * (int32_t)(k))
#else
#define TEXT_KW(s, k) (text_w(&FONT_S, (s)) * (int32_t)(k))
#endif

/* RAPTOR: a page's four values large, a 2 x 2 grid from under the header to the footer (the owner: as big as the
 * screen takes). A cell: the label (2x, the knob's colour), the value (3x where it fits with its unit, else 2x, else
 * 1x), the unit beside it, a 12 px gauge. One value alone: 4x in the middle */
UI_SMALL static void graph_big(void)
{
    uint32_t c, n = 0;
    for (c = 0; c < 4u; c++)
        n += ui.big_l[c][0] != 0;
    if (n > 1u) {
        cv_rect(119, 2, 2, H_BIG - 4, C_LINE);
        cv_rect(2, H_BIG / 2 - 1, 236, 2, C_LINE);
    }
    for (c = 0; c < 4u; c++) {
        const int32_t CW = 110;                         /* a cell's width for text */
        int32_t x0 = c & 1u ? 126 : 4, y0 = c & 2u ? (int32_t)H_BIG / 2 + 2 : 0, x, r = ui.big_r[c];
        int32_t uw = ui.big_u[c][0] ? text_w(&FONT_S, ui.big_u[c]) + 3 : 0, lk, vk, vh;
        uint16_t vc = ui.big_c[c] == C_DIM ? C_DIM : C_BIGV, kc = KN_COL[c & 3u];
        const char *v = ui.big_v[c];
        if (!ui.big_l[c][0])
            continue;
        if (n == 1u) {                                  /* one value: in the middle, 4x (or less) */
            for (vk = 4; vk > 1 && TEXT_KW(v, vk) + uw > 228; vk--)
                ;
            lk = TEXT_KW(ui.big_l[c], 2) <= 228 ? 2 : 1;
#ifdef PALEO_CLAYBORN
            if (TEXT_KW(ui.big_l[c], 3) <= 228)
                lk = 3;                                 /* PALEOTONE: the label bigger too */
#endif
            x0 = (240 - TEXT_KW(ui.big_l[c], lk)) / 2;
            text_k(x0, (int32_t)H_BIG / 2 - 16 * vk / 2 - 16 * lk - 2, ui.big_l[c], kc, (uint32_t)lk);
            x0 = (240 - TEXT_KW(v, vk) - uw) / 2;
            x = text_k(x0, (int32_t)H_BIG / 2 - 16 * vk / 2, v, vc, (uint32_t)vk);
            if (uw)
                cv_text(x + 3, (int32_t)H_BIG / 2 + 16 * vk / 2 - 16 - 2 * vk, &FONT_S, ui.big_u[c], C_GRAY);
            if (r >= 0) {
                cv_rect(20, (int32_t)H_BIG / 2 + 16 * vk / 2 + 4, 200, 12, TE_G1);
                cv_rect(20, (int32_t)H_BIG / 2 + 16 * vk / 2 + 4, (r > 1000 ? 1000 : r) * 200 / 1000, 12,
                        vc == C_DIM ? TE_G3 : kc);
            }
            continue;
        }
#ifdef PALEO_CLAYBORN
        /* PALEOTONE (bigger): the label at 3x where it fits, the value at 4x; Clayborn's capitals sit on row 12k of
         * the 16k cell, 9k tall: the label's from row 2 of the cell, the value's ending on row 70, over the gauge */
        lk = TEXT_KW(ui.big_l[c], 3) <= CW ? 3 : TEXT_KW(ui.big_l[c], 2) <= CW ? 2 : 1;
        text_k(x0, y0 + 2 - 3 * lk, ui.big_l[c], kc, (uint32_t)lk);
        for (vk = 4; vk > 1 && TEXT_KW(v, vk) + uw > CW; vk--)
            ;
        (void)vh;
        x = text_k(x0, y0 + 70 - 12 * vk, v, vc, (uint32_t)vk);
#else
        lk = TEXT_KW(ui.big_l[c], 2) <= CW ? 2 : 1;
        text_k(x0, y0 + (lk == 2 ? -3 : 4), ui.big_l[c], kc, (uint32_t)lk);   /* (the capitals' top rows: 2..) */
        for (vk = 3; vk > 1 && TEXT_KW(v, vk) + uw > CW; vk--)
            ;
        vh = 16 * vk;                                   /* the value's cell: its capitals end ~3/4 down */
        x = text_k(x0, y0 + 70 - 2 - vh * 13 / 16, v, vc, (uint32_t)vk);
#endif
        if (uw && x + uw <= x0 + CW + 4)
            cv_text(x + 3, y0 + 70 - 2 - 13, &FONT_S, ui.big_u[c], C_GRAY);
        if (r >= 0) {                                   /* the gauge: 12 px, the knob's colour */
            cv_rect(x0, y0 + 72, CW, 12, TE_G1);
            cv_rect(x0, y0 + 72, (r > 1000 ? 1000 : r) * CW / 1000, 12, vc == C_DIM ? TE_G3 : kc);
        }
    }
}
#undef TEXT_KW

UI_SMALL static void draw_graph(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint16_t c = TE_COL[TSLOT(song.sel)];               /* LIVE: the curves in the track's colour */
    uint32_t sig, top, drum_note = !ui.home && is_drum(t) && !page_for_drum(pg);
    if (!ui.home && pg->graph == GR_TRK) {
        draw_tracks();
        ui.graph_top = 1;                            /* the next graph draws its top rows again */
        return;
    }
    sig = graph_signature();
    if (!ui.force && sig == ui.graph_sig)
        return;
    ui.graph_sig = sig;
    if (!ui.home && !drum_note && big_page(pg)) {     /* RAPTOR: the values once, large, from under the header */
        uint32_t half;
        for (half = 0; half < 2u; half++) {           /* (in two bands: the canvas holds 120 rows) */
            cv_begin(240, H_BIG / 2, C_BLACK);
            cv_oy = -(int32_t)(half * (H_BIG / 2));
            graph_big();
            cv_oy = 0;
            cv_blit(0, Y_BIG + half * (H_BIG / 2));
        }
        ui.graph_top = 1;
        return;
    }
    cv_begin(240, H_GRAPH, C_BLACK);
    cv_oy = G_OY;
    if (ui.home) {
        graph_scope(c);
    } else if (drum_note) {                          /* a page the drum track has no use for */
        static const char *const L[2] = {"DRUM TRACK", "SEQ  TRACKS  GLO DRUMS"};
        cv_text((240 - text_w(&FONT_S, L[0])) / 2, 26, &FONT_S, L[0], C_HI);
        cv_text((240 - text_w(&FONT_S, L[1])) / 2, 52, &FONT_S, L[1], C_DIM);
    } else {
        switch (pg->graph) {
        case GR_ADSR:
            graph_adsr(t, c);
            break;
        case GR_LFO:
            graph_lfo(t, c);
            break;
        case GR_STEPS:
            graph_steps(t, c);
            break;
        case GR_ROLL:
            cv_oy = 0;
            graph_roll(t, c);
            break;
        case GR_SCALE:
            graph_scale(t, c);
            break;
        case GR_FX:
            graph_fx(t, c);
            break;
        case GR_SLCR:
            graph_slicer(t, c);
            break;
        case GR_BROWSE:
            cv_oy = 0;
            graph_browse();
            break;
        case GR_SLOTS:
            cv_oy = 0;
            graph_slots();
            break;
        case GR_USER:
            cv_oy = 0;
            graph_user();
            break;
        default:
            break;
        }
    }
    top = !ui.home && !drum_note && (pg->graph == GR_BROWSE || pg->graph == GR_SLOTS || pg->graph == GR_USER ||
                                     pg->graph == GR_ROLL || big_page(pg));   /* these draw from the top */
    cv_oy = 0;
    if (ui.hot_t && settings.zoom && !(top && (big_page(pg) || pg->graph == GR_ROLL) && !drum_note)) {   /* (the big values show it already) */                 /* focus (menu ZOOM): the touched value, large and white */
        int32_t x;
        top = 1;
        cv_rect(0, 0, 150, 50, C_BLACK);
        cv_text(4, 0, &FONT_S, ui.focus_l, C_GRAY);
        x = cv_text(4, 16, &FONT_L, ui.focus_v, C_WHITE);
        cv_text(x + 4, 30, &FONT_S, ui.focus_u, C_DIM);
    }
    /* graphs keep out of the top G_OY rows: skip them unless something is (or was) there */
    cv_blit_from(0, Y_GRAPH, top || ui.graph_top || ui.force ? 0u : G_OY);
    ui.graph_top = (uint8_t)top;
}

/* the page shown: its place in its family and how many (*n), as SELECT walks them (2.7: the screens of their own, SONG
 * and DRUMS, are not counted: SEQ is STEP 1/2, PATTERN 2/2; the page shown always counts) */
UI_SMALL static uint32_t page_pos(uint32_t *n)
{
    const page_t *pg = cur_page();
    uint32_t i, k = 0;
    *n = 0;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == pg->fam && (i == ui.page || (page_shown(&PAGES[i]) && PAGES[i].scope != SC_SONG &&
                                                         PAGES[i].scope != SC_DRUM))) {
            (*n)++;
            if (i == ui.page)
                k = *n;
        }
    return k;
}

UI_SMALL static void draw_foot(void)
{
    char s[48], pn[16], en[10], ti[20];
    const track_t *t = TSEL;
    uint32_t sig;
    const page_t *pg = cur_page();
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    const char *ename = is_drum(t) ? "DRUM" : e->name;
    int32_t x;
    pn[0] = 0;
    if (is_drum(t))
        str_cpy(pn, DRUM_KIT_NAMES[drum_kit()], sizeof pn);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), pn);                       /* a user preset */
    else if (e->npresets)
        str_cpy(pn, e->presets[TSEL->preset % e->npresets].name, sizeof pn);
    if (ui.home) {
        str_cpy(ti, "HOME", sizeof ti);
    } else if (mac_on) {
        str_cpy(ti, "MACROS", sizeof ti);               /* RAPTOR: KNOB BANK */
    } else {                                           /* page title + number in its family: "ENV DEST 2/2" */
        uint32_t n, k = page_pos(&n);
        const char *pt = pg->scope == SC_ENGINE ? e->page_title[pg->id[0] != P_E0] : 0;   /* EDIT: the engine's */
        str_cpy(ti, pt ? pt : pg->title, 10);
        if (n > 1) {
            str_cpy(ti + str_len(ti), " ", 4);
            fmt_int(ti + str_len(ti), (int32_t)k);
            str_cpy(ti + str_len(ti), "/", 4);
            fmt_int(ti + str_len(ti), (int32_t)n);
        }
    }
    str_cpy(s, ename, sizeof s);
    s[str_len(s) + 1u] = 0;
    s[str_len(s)] = (char)('1' + song.sel);
    str_cpy(s + str_len(s), pn, 16);
    str_cpy(s + str_len(s), ti, sizeof ti);
    {   /* step markers: the playhead only when it is in the shown bank, the cursor only in SEQ */
        uint32_t ph = song.playing && t->seq_idx / 16u == ui.bank ? t->seq_idx : 0xFFu;
        sig = str_hash(0x9E3779B9u, s) + ph * 97u + (song.seq_mode ? ui.cursor : 0xFFu) * 3001u + steps_hash(t) +
              ui.bank * 7u + (uint32_t)t->p[P_SLEN] * 13u;
    }
    if (!ui.force && sig == ui.foot_sig)
        return;
    ui.foot_sig = sig;
    cv_begin(240, H_FOOT, C_BLACK);
    /* RAPTOR (the owner: everything larger): row 1 the engine (icon, name) and the page, small; row 2 the sound's
     * name twice as big, as wide as the screen. (SLOOP's 16 step bars went: the SEQ pages show the steps) */
    x = 4;
    if (FELUCCA_ICONS) {
        cv_icon(x, 1, engine_icon(ename), C_GRAY);
        x += 14;
    }
    fit(en, ename, &FONT_S, 230 - text_w(&FONT_S, ti) - x);
    cv_text(x, 0, &FONT_S, en, C_GRAY);
    cv_text(236 - text_w(&FONT_S, ti), 0, &FONT_S, ti, C_HI);
    {
        char pf[16];
        str_cpy(pf, pn[0] ? pn : ename, sizeof pf);
        while (pf[0] && text_w(&FONT_L, pf) > 232)     /* (FONT_L: the 2x font, Clayborn on the FM-1) */
            pf[str_len(pf) - 1u] = 0;
        text_k(4, 9, pf, C_AMB, 2u);
    }
    cv_blit(0, Y_FOOT);
}
UI_SMALL static void draw_columns(void)
{
    uint32_t c;
    char val[12];
    const char *unit;
    if (ui.home) {
        for (c = 0; c < 4u; c++) {
            int16_t *vp;
            const param_desc_t *d = home_param(c, &vp);
            param_format(d, *vp, val, &unit);
            draw_column(c, d->label, val, unit, VAL(c), RATIO(d, *vp), param_icon(d, *vp));
        }
        return;
    }
    if (is_drum(TSEL) && !page_for_drum(cur_page())) {   /* "DRUM TRACK" (the graph says so) */
        for (c = 0; c < 4u; c++)
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_TRK) {                 /* TRACK LEVEL LEN PAN of the selected track */
        const track_t *t = TSEL;
        uint32_t lvl = trk_level(song.sel);
        fmt_int(val, (int32_t)song.sel + 1);
        draw_column(0, "TRACK", val, "/6", VAL(0u), (int32_t)song.sel * 1000 / (NTRK - 1), ICON_AUTO);
        if (!lvl || t->p[P_MUTE]) {                    /* (MUTE: a turn of KNOB 2 unmutes, tracks_edit) */
            str_cpy(val, "MUTE", 12);
            unit = "";
        } else if (is_drum(t)) {
            fmt_int(val, (int32_t)lvl);
            unit = "";
        } else {
            param_format(&TP[P_LEVEL], (int32_t)lvl, val, &unit);
        }
        draw_column(1, "LEVEL", val, unit, lvl && !t->p[P_MUTE] ? VAL(1u) : C_DIM, (int32_t)lvl * 1000 / 127, ICON_AUTO);
        param_format(&TP[P_SLEN], t->p[P_SLEN], val, &unit);
        draw_column(2, "LEN", val, unit, VAL(2u), RATIO(&TP[P_SLEN], t->p[P_SLEN]), ICON_AUTO);
        param_format(&TP[P_PAN], t->p[P_PAN], val, &unit);
        draw_column(3, "PAN", val, unit, VAL(3u), RATIO(&TP[P_PAN], t->p[P_PAN]), param_icon(&TP[P_PAN], t->p[P_PAN]));
        return;
    }
    if (cur_page()->graph == GR_BROWSE) {
        uint32_t total, cur = preset_pos(&total);
        char u[8];
        fmt_int(val, (int32_t)cur + 1);
        str_cpy(u, "/", 8);
        fmt_int(u + 1, (int32_t)total);
        draw_column(0, "No.", val, u, VAL(0u), -1, ICON_NONE);
        draw_column(1, "ENG", ENGINES[TSEL->eng_req]->name, "", VAL(1u), -1, engine_icon(ENGINES[TSEL->eng_req]->name));
        draw_column(2, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(3, "FAV", fav_has(fav_key()) ? "ON" : "--", "", VAL(3u), -1, ICON_NONE);   /* (2.7) KNOB 4 */
        return;
    }
    if (cur_page()->graph == GR_USER) {                  /* SLOT, then three GO buttons */
        int used = up_used(ui.uslot);
        up_slot_label(val, ui.uslot);
        draw_column(0, "SLOT", val, "", VAL(0u), (int32_t)ui.uslot * 1000 / (int32_t)(UP_SLOTS - 1u), ICON_AUTO);
        draw_column(1, "LOAD", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(2, "ERASE", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(3, "SAVE", "--", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_STEP) {                 /* (2.7.2) NOTES: NOTE PITCH LEN FORCE of the note under edit */
        const step_t *st = &TSEL->step[ui.cursor];
        char sn[8], sl[8], u[8], ln[8];
        int on = step_on(st);
        uint32_t k = on ? ui.note_k % st->n : 0u;
        fmt_int(sn, (int32_t)ui.cursor + 1);
        str_cpy(sl, "/", 8);
        fmt_int(sl + 1, TSEL->p[P_SLEN]);
        draw_column(0, "NOTE", sn, sl, VAL(0u), -1, ICON_AUTO);
        u[0] = 0;
        if (on) {
            note_name(val, st->note[k]);
            if (st->n > 1u) {                             /* a chord: which of its notes */
                fmt_int(u, (int32_t)k + 1);
                str_cpy(u + str_len(u), "/", 2);
                fmt_int(u + str_len(u), st->n);
            }
        } else {
            str_cpy(val, "--", 12);
        }
        draw_column(1, "PITCH", val, u, on ? VAL(1u) : C_DIM, -1, ICON_AUTO);
        if (on)
            fmt_int(ln, (int32_t)note_span(TSEL, ui.cursor));
        else
            str_cpy(ln, "--", 8);
        draw_column(2, "LEN", ln, on ? "STEP" : "", on ? VAL(2u) : C_DIM, -1, ICON_AUTO);
        draw_column(3, "FORCE", on ? FORCE_N[note_force(st, k)] : "--", "", on ? VAL(3u) : C_DIM, -1, ICON_AUTO);
        return;
    }
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(cur_page(), c, &vp);
        if (cur_page()->graph == GR_SLOTS && c == 1u) {   /* 2.6: ERASE, lit while the slot holds a project */
            draw_column(1, "ERASE", "--", "", project_used((uint32_t)song.g[G_SLOT] - 1u) ? C_HI : C_DIM, -1, ICON_AUTO);
            continue;
        }
        if (!d || !d->label || d->label[0] == '-') {
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL && !usb.config) {   /* not connected: the
                                                       * USB status instead of the CPU (2.4: MIDI is MIDI OUT, ROUT is IN) */
            str_cpy(val, !usb.up ? "OFF" : usb.config ? "MIDI" : usb.setups ? "ENUM" : usb.sof_seen ? "BUS" : "WAIT", 12);
            unit = "USB";
            draw_column(c, "USB", val, unit, C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL) {
            fmt_int(val, (int32_t)(song.cpu_q8 * 100u / 256u));
            unit = "%";
        } else {
            param_format(d, *vp, val, &unit);
        }
        draw_column(c, d->label, val, unit, VAL(c), d->fmt == F_ENUM && d->max < 2 ? -1 : RATIO(d, *vp),
                    param_icon(d, *vp));
    }
}


/* the UI's timers, once a frame whatever the screen (a message, the BPM highlight, an armed GO, the
 * white value of the knob just turned) */
UI_SMALL static void ui_timers(void)
{
    if (ui.msg_t)
        ui.msg_t--;
    if (ui.bpm_t)
        ui.bpm_t--;
    if (ui.arm_t && !--ui.arm_t)
        ui.arm = 0;
    if (ui.hot_t)
        ui.hot_t--;
}

#if BOARD_SMK37
static int fb_shown(void);                      /* ui_fbank.c */
static void fb_draw(void);
static int fb_left(void);
static void fb_set(uint32_t b);
static void fb_close(void);
static void fb_expire(void);
static int daw_screen(void);                    /* ui_daw.c: DAW mode */
static void daw_switch(uint32_t *pressed);
static void daw_input(uint32_t pressed, uint32_t now);
static void daw_leds(void);
static void daw_set(uint32_t on, int splash);
static void daw_edit_open(void);
#endif
static void vis_draw(void);                     /* ui_vis.c */
static int vis_shown(void);
static uint8_t vis_on, vis_shown_last, vis_name_t;   /* the visualiser (ui_vis.c): on, drawn last frame, its name's time */
UI_SMALL static void ui_draw(void)
{
    ui.frame++;
    pads_tick();
#if BOARD_SMK37
    if (daw_screen()) {                                 /* DAW mode, the mode's screen (ui_daw.c) */
        ui_timers();
        ui.force = 0;
        return;
    }
#endif
    if (rec_go) {                                       /* the take started: say so */
        rec_go = 0;
        ui_message("RECORDING");
    }
    if (ft_bars) {                                      /* a free take closed: the loop it made */
        char m[24];
        uint32_t n = ft_bars;
        ft_bars = 0;
        if (n == 0xFFu) {
            ui_message("TAKE DROPPED");
        } else {
            str_cpy(m, n == 1u ? "LOOP 1 BAR " : n == 2u ? "LOOP 2 BARS " : "LOOP 4 BARS ", sizeof m);
            fmt_int(m + str_len(m), song.g[G_BPM]);
            str_cpy(m + str_len(m), " BPM", sizeof m - str_len(m));
            ui_message(m);
        }
    }
    if (er_flash) {                                     /* EDIT + key took something out */
        er_flash = 0;
        if (!ui.msg_t)
            ui_message("ERASED");
    }
#if BOARD_SMK37
    if (fb_shown() || pick_shown()) {                  /* the FADER BANK screen (ui_fbank.c), the pad picker (ui_pick.c) */
        if (fb_shown())
            fb_draw();
        else
            pick_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (fb_left() | (pick_frame && pick_frame + 1u == ui.frame)) {   /* back from them */
        pick_frame = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
#endif
    if (!ui.menu && (ui.layer != LY_PLAY || ui.hold_kind)) {   /* a layer held / a hold to confirm */
        if (ui.hold_kind)
            hold_screen_draw();
        else
            layer_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (layer_shown) {                                  /* back to the page */
        layer_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if ((rec_wait || ft_on) && !ui.menu && !on_song_page()) {   /* armed / free take */
        rec_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (rec_shown) {                                    /* back to the page */
        rec_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
#if FELUCCA_ARRANGER
    if (!ui.menu && on_song_page()) {
        song_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
#endif
    if (ui.menu) {
        draw_menu();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown()) {                                  /* the visualiser over the TRACKS screen */
        vis_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown_last) {                               /* back from it */
        vis_shown_last = 0;
        vis_on = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (!ui.home && cur_page()->scope == SC_TRK) {
        studio_tracks_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (on_drum_page()) {
        drum_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    cursor_fix();
    if (ui.force)
        draw_frame();
    felucca_dbg.stage = 3;
    draw_head();
    felucca_dbg.stage = 4;
    draw_columns();
    felucca_dbg.stage = 5;
    draw_graph();
    ui_timers();
    felucca_dbg.stage = 6;
    draw_foot();
    ui.force = 0;
}
