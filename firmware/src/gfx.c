/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Small-canvas renderer (no full framebuffer). Draw text/lines into
 * an off-screen strip, then blit it. RAPTOR: a pixel is a byte, an index into the canvas's own colour table
 * (cv_lut, up to 256 colours a canvas, filled as colours are drawn); cv_blit turns the rows into the panel's
 * byte-swapped RGB565 (big-endian) in two small line buffers, one going out by DMA while the next is filled.
 * Half the RAM of a 16-bit strip (29 KB given to the delay line); the same pixels on the screen. */
/* UI_SMALL: the screens' and the buttons' functions (panel.c, ui*.c, icons.c; never in the audio
 * path) are compiled for size (clang minsize, as -Oz). 2.7: the drums' SOUND page and the favourites
 * made the image overflow the flash (XIP); this gives it back with nothing taken out. */
#ifndef UI_SMALL
#if defined(__clang__)
#define UI_SMALL __attribute__((minsize))
#else
#define UI_SMALL
#endif
#endif
typedef struct {               /* proportional, see tools/gen_font.py */
    uint8_t h;
    uint8_t pad;               /* bitmap starts this many pixels left of the pen */
    uint8_t first, last;
    uint8_t sh;                /* 1: the bitmaps are another font's, drawn at 2x (L = S at 2x, no copy) */
    const uint8_t *adv;        /* advance per glyph */
    const uint8_t *bw;         /* bitmap width per glyph (starts FONT_PAD left of the pen) */
    const uint16_t *off;       /* byte offset of each glyph */
    const uint8_t *data;
    uint8_t bits;              /* SLOOP 2.6: 1 = 1 bit a pixel (a pixel font: glyph rows one after the other, high
                                * bit first, each glyph from a whole byte); 0 = 2 pixels a byte, alpha 0..15 */
} felucca_font_t;
#include "felucca_font.h"
/* PALEOTONE (FM-1): the large text in Clayborn (tools/gen_font.py): FONT_L is its 24 px cut, text_k's k = 3 and 4
 * its 36 px cut and the 24 px one at 2x; the small text stays Terminus. (PALEO_TERMINUS_L: the font tests' old L) */
#if !(defined(BOARD_SMK37) && BOARD_SMK37) && !defined(PALEO_TERMINUS_L)
#define PALEO_CLAYBORN 1
#define FONT_L FONT_C2
#define PALEO_SWING_LAB "swng"              /* (a dial's label short enough for M) */
#else
#define PALEO_SWING_LAB "swing"
#endif

#define CV_MAX (240u * 124u)      /* the graph strip is 240 x 124 */
static uint8_t cv_px[CV_MAX] __attribute__((section(".pool")));   /* colour indices (cv_lut) */
static uint32_t cv_w, cv_h;
#define CV_NC 256u                /* colours a canvas */
#define CV_HN 512u                /* their hash table */
#define CV_LB 960u                /* pixels a line buffer (4 rows of 240) */
static uint16_t cv_lut[CV_NC] __attribute__((section(".lowram")));   /* index -> RGB565, byte-swapped for the panel */
static uint16_t cv_hk[CV_HN] __attribute__((section(".lowram")));    /* colour -> index + 1 (0: free) */
static uint32_t cv_nc;            /* colours in cv_lut */
static uint32_t cv_last = 1u << 16, cv_last_i;   /* (the last colour asked for: lines, icons; none: past 16 bits) */
static uint16_t cv_lb[2][CV_LB];  /* the panel's rows, one going out while the other is filled */
static uint32_t cv_lbi;
static int32_t cv_oy;            /* y offset for graph drawing */

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static uint16_t pal_bg;                  /* SLOOP 2.7: the background, black unless a theme says (palette_set) */
#define C_BLACK pal_bg                   /* the background (and dark text on a light tile) */
static uint16_t pal_hi = 0xFFFFu;        /* the accent: white unless a theme says (palette_set) */
#define C_WHITE pal_hi               /* accent only: what is being touched / where we are */
/* The screen is five steps of one colour, darkest to brightest, plus white.
 * Palettes are picked in the HOME-hold menu (COLOR). */
typedef struct {
    const char *name;
    uint16_t c[5];             /* the five steps, faintest .. strongest (RGB565) */
    uint16_t bg, hi;           /* PALEOTONE: the background and the accent (SLOOP's built-ins: black and white) */
    uint16_t trk[6];           /* PALEOTONE: the tracks' colours by colour slot (TSLOT: synths 0..2, drums 3, SFX 4, CALLS 5) */
} palette_t;
/* PALEOTONE: the FM-1 Paleotone's own colour schemes (tools/gen_ui_palettes.py on main: BG, TEXT, THEME, ACCENT and the
 * track colours of each). The five steps are LINE (18 % of the way from BG to TEXT), DIM (42 %), MID (70 %), TEXT and
 * THEME; the accent is ACCENT. TERMINAL (a 1993 park-operations console: phosphor green on black, amber for what
 * needs attention) is a fresh device's; JUNGLE and VOLCANO beside it; GHOULBOX's CRYPT; HI-CON for bright light. */
static const palette_t PALETTES[] = {
    {"TERMINAL", {0x21A4, 0x536A, 0x8D92, 0xC7D9, 0x474E}, 0x0040, 0xFD84, {0xFD84, 0x569E, 0xFBD9, 0xACBF, 0xFCA7, 0x6D5F}},
    {"JUNGLE", {0x31C5, 0x6B6B, 0xAD32, 0xEF39, 0xAEAA}, 0x08A1, 0xFBC6, {0xFBC6, 0xFBD9, 0x6D5F, 0x6736, 0x569E, 0xACBF}},
    {"VOLCANO", {0x39A5, 0x734B, 0xB533, 0xF75A, 0xFB27}, 0x1061, 0xFEAA, {0xFEAA, 0xFBD9, 0x6D5F, 0x6736, 0x569E, 0xACBF}},
    {"CRYPT", {0x3165, 0x6B0A, 0xACF1, 0xEEF8, 0xECC5}, 0x0841, 0xEA68, {0xEA68, 0x569E, 0xACBF, 0xBF4A, 0xFBD9, 0x6D5F}},
    {"HI-CON", {0x2965, 0x6B4D, 0xB576, 0xFFDF, 0xFF40}, 0x0000, 0x073F, {0x073F, 0xFBD9, 0xACBF, 0x6D5F, 0xBF4A, 0xFCA7}},
};
#define PAL_TERMINAL 0u          /* PALEOTONE: a fresh device's palette */
#define NPALETTES (sizeof(PALETTES) / sizeof(PALETTES[0]))
static uint16_t pal[5];
#define C_LINE pal[0]                /* 1 rules, separators */
#define C_DIM pal[1]                 /* 2 inactive, empty steps, units */
#define C_GRAY pal[2]                /* 3 labels */
#define C_AMB pal[3]                 /* 4 secondary text */
#define C_HI pal[4]                  /* 5 values, curves */

/* SLOOP 2.7: your own theme, made in the web editor (its Theme page): the five steps of the screen and the four
 * tracks' colours, with a name. MENU > COLOR shows it after MONO (theme_on: panel.c lights_word bit 30; the
 * palette number stays a built-in one, the one an older SLOOP shows). Kept in the settings object's side record
 * (project.c, storage.c st_side): an older SLOOP never reads it, and forgets it when it saves its settings. */
#define THEME_MAGIC 0x324D4854u                  /* "THM2" (2.7 beta 2: with the accent) */
#define THEME_MAGIC1 0x314D4854u                 /* "THM1": the first beta's, 36 bytes (its CRC where hi is now),
                                                  * taken with a white accent (black on a light background) */
typedef struct {
    uint32_t magic;
    char name[8];                                /* ASCII, 0-padded (not terminated when 8 long) */
    uint16_t c[5];                               /* the screen, faintest .. strongest (as a palette), RGB565 */
    uint16_t trk[4];                             /* the tracks' colours (knobs, tiles, meters), RGB565 */
    uint16_t bg;                                 /* the background, RGB565 (0 black: as before) */
    uint16_t hi;                                 /* the accent (what is touched, the cursor, the play head) */
    uint16_t rsv;                                /* 0 */
    uint32_t crc;                                /* CRC-32 (storage.c st_crc32) of the 36 bytes before it */
} theme_t;
_Static_assert(sizeof(theme_t) == 40u, "theme layout (the editor's Theme page writes these bytes)");
static theme_t theme;                            /* the theme there is (theme_valid: project.c theme_check passed) */
static uint8_t theme_valid, theme_on;
/* the tracks' colours, by colour slot: SLOOP's four (0..2 the synth tracks, 3 the drums: a theme sets them), then
 * RAPTOR's SFX (4) and CALLS (5). TSLOT(k): track k's slot (core.h). PALEOTONE: each palette has its own (palette_set);
 * full, two thirds, one third. The defaults: TERMINAL's. */
#define TE_COL_DEF (PALETTES[PAL_TERMINAL].trk)
static uint16_t TE_COL[6] = {0xFD84, 0x569E, 0xFBD9, 0xACBF, 0xFCA7, 0x6D5F};
static uint16_t TE_MID[6], TE_DIM[6];
/* PALEOTONE: the four knobs' colours (labels, gauges, pies) and their one third: the palette's THEME, one colour for the
 * four, as Paleotone's phosphor screens (the track colours stay the tracks'). The SMK-37 (RAPTOR): SLOOP's, a track
 * colour a knob */
static uint16_t KN_COL[4], KN_DIM[4];
#if defined(BOARD_SMK37) && BOARD_SMK37
#define C_BIGV C_WHITE               /* the big values of a page: the accent (RAPTOR) */
#else
#define C_BIGV C_AMB                 /* PALEOTONE: the palette's TEXT (TERMINAL: pale phosphor), the accent for the touched */
#endif
static uint16_t rgb565_scale(uint32_t c, uint32_t num, uint32_t den)
{
    uint32_t r = ((c >> 11) & 31u) * num / den, g = ((c >> 5) & 63u) * num / den, b = (c & 31u) * num / den;
    return (uint16_t)(r << 11 | g << 5 | b);
}
static uint32_t st_crc32(const void *p, uint32_t n);   /* storage.c (zlib CRC-32, as the editor's) */
/* a channel of an RGB565 colour on 0..255 (as the editor reads it: x * 8, x * 4, x * 8) */
static int32_t rgb565_ch(uint32_t c, uint32_t k)
{
    return k == 0u ? (int32_t)((c >> 11) & 31u) * 8 : k == 1u ? (int32_t)((c >> 5) & 63u) * 4 : (int32_t)(c & 31u) * 8;
}
/* one colour stands out from the background: one channel 96 or more apart */
static int theme_apart(uint32_t c, uint32_t bg)
{
    uint32_t k;
    for (k = 0; k < 3u; k++) {
        int32_t d = rgb565_ch(c, k) - rgb565_ch(bg, k);
        if (d >= 96 || d <= -96)
            return 1;
    }
    return 0;
}
/* a theme as the editor sends it and flash keeps it: its magic, its CRC, a name of printable ASCII, and the strongest
 * step and the accent standing out from the background (a screen whose values or menu cursor cannot be seen would hide
 * the way back to COLOR). A first-beta THM1 (36 bytes) is turned into a THM2 in *t */
static int theme_check(theme_t *t)
{
    uint32_t k;
    if (t->magic == THEME_MAGIC1) {
        if (((uint32_t)t->hi | (uint32_t)t->rsv << 16) != st_crc32(t, 32u))
            return 0;
        t->hi = theme_apart(0xFFFFu, t->bg) ? 0xFFFFu : 0u;
        t->rsv = 0;
        t->magic = THEME_MAGIC;
        t->crc = st_crc32(t, __builtin_offsetof(theme_t, crc));
    }
    if (t->magic != THEME_MAGIC || t->crc != st_crc32(t, __builtin_offsetof(theme_t, crc)))
        return 0;
    for (k = 0; k < sizeof t->name; k++)
        if (t->name[k] && (t->name[k] < 0x20 || t->name[k] > 0x7E))
            return 0;
    return theme_apart(t->c[4], t->bg) && theme_apart(t->hi, t->bg);
}
/* colour c at a / n over the background (antialiased text and icons) */
static uint16_t rgb565_over(uint32_t c, uint32_t a, uint32_t n)
{
    uint32_t b = pal_bg, r, g, bl;
    r = (((b >> 11) & 31u) * (n - a) + ((c >> 11) & 31u) * a) / n;
    g = (((b >> 5) & 63u) * (n - a) + ((c >> 5) & 63u) * a) / n;
    bl = ((b & 31u) * (n - a) + (c & 31u) * a) / n;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
static int theme_shown(void) { return theme_on && theme_valid; }
/* (2.7) the greys of the TE-style pages (tiles, empty steps, labels, secondary text) are drawn for black: on a theme's
 * background, each the same share of the way from it toward white (a dark one) or black (a light one). On black: as
 * ever. te_gray() does it for any grey */
static const uint16_t TE_G_DEF[4] = {RGB(26, 26, 30), RGB(54, 54, 60), RGB(118, 118, 126), RGB(196, 196, 204)};
static uint16_t te_g[4] = {RGB(26, 26, 30), RGB(54, 54, 60), RGB(118, 118, 126), RGB(196, 196, 204)};
#define TE_G1 te_g[0]                    /* tiles */
#define TE_G2 te_g[1]                    /* empty steps, dial tracks */
#define TE_G3 te_g[2]                    /* labels */
#define TE_G4 te_g[3]                    /* secondary text */
static uint16_t te_gray(uint32_t c)
{
    uint32_t bg = pal_bg, k, out = 0;
    static const uint8_t SH[3] = {11, 5, 0}, MX[3] = {31, 63, 31};
    int light;
    if (!bg)
        return (uint16_t)c;
    light = ((bg >> 11) & 31u) * 2u * 299u + ((bg >> 5) & 63u) * 587u + (bg & 31u) * 2u * 114u > 31500u;
    for (k = 0; k < 3u; k++) {
        int32_t m = MX[k], b = (int32_t)((bg >> SH[k]) & (uint32_t)m), x = (int32_t)((c >> SH[k]) & (uint32_t)m);
        out |= (uint32_t)(b + ((light ? 0 : m) - b) * x / m) << SH[k];
    }
    return (uint16_t)out;
}

static void palette_set(uint32_t i)              /* built-in palette i, or the theme when it is chosen (2.7) */
{
    uint32_t k;
    int cu = theme_shown();
    const palette_t *pp = &PALETTES[i % NPALETTES];
    for (k = 0; k < 5u; k++)
        pal[k] = cu ? theme.c[k] : pp->c[k];
    pal_bg = cu ? theme.bg : pp->bg;
    pal_hi = cu ? theme.hi : pp->hi;
    for (k = 0; k < 4u; k++)
        te_g[k] = te_gray(TE_G_DEF[k]);
    for (k = 0; k < 6u; k++) {                    /* (a theme has four: SFX and CALLS keep the palette's) */
        TE_COL[k] = cu && k < 4u ? theme.trk[k] : pp->trk[k];
        TE_MID[k] = rgb565_scale(TE_COL[k], 2u, 3u);
        TE_DIM[k] = rgb565_scale(TE_COL[k], 1u, 3u);
    }
#if !(defined(BOARD_SMK37) && BOARD_SMK37)
    te_g[0] = rgb565_scale(pal[0], 4u, 5u);           /* PALEOTONE: the greys in the palette's own tint (TERMINAL: */
    te_g[1] = rgb565_scale(pal[1], 2u, 3u);           /* green-grey), not SLOOP's neutral ones */
    te_g[2] = rgb565_scale(pal[2], 5u, 6u);
    te_g[3] = rgb565_scale(pal[3], 9u, 10u);
#endif
    for (k = 0; k < 4u; k++) {
#if defined(BOARD_SMK37) && BOARD_SMK37
        KN_COL[k] = TE_COL[k];
#else
        KN_COL[k] = pal[4];
#endif
        KN_DIM[k] = rgb565_scale(KN_COL[k], 1u, 3u);
    }
}

static inline uint16_t swap16(uint32_t c) { return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8)); }

/* the index of colour c (RGB565) in this canvas's table, added if new; a canvas with more than CV_NC colours
 * (none on any screen: the five steps, white, the tracks' colours and the text's 15 blend steps of each) gets the
 * nearest one */
static uint32_t cv_ci(uint32_t c)
{
    uint32_t h, n, sc = swap16(c), best = 0, bd = 0xFFFFFFFFu, i;
    c &= 0xFFFFu;
    if (c == cv_last)
        return cv_last_i;
    h = ((c * 40503u) >> 7) & (CV_HN - 1u);
    for (n = 0; n < 16u; n++, h = (h + 1u) & (CV_HN - 1u)) {
        uint32_t e = cv_hk[h];
        if (!e)
            break;
        if (cv_lut[e - 1u] == sc) {
            cv_last = c;
            return cv_last_i = e - 1u;
        }
    }
    if (n < 16u && cv_nc < CV_NC) {
        cv_lut[cv_nc] = (uint16_t)sc;
        cv_hk[h] = (uint16_t)++cv_nc;
        cv_last = c;
        return cv_last_i = cv_nc - 1u;
    }
    for (i = 0; i < cv_nc; i++) {                       /* (full: the nearest, by the sum of squares of R, G, B) */
        uint32_t o = swap16(cv_lut[i]);
        int32_t dr = (int32_t)(o >> 11) - (int32_t)(c >> 11), dg = (int32_t)((o >> 5) & 63u) - (int32_t)((c >> 5) & 63u),
                db = (int32_t)(o & 31u) - (int32_t)(c & 31u);
        uint32_t d = (uint32_t)(4 * dr * dr + dg * dg + 4 * db * db);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

static void cv_begin(uint32_t w, uint32_t h, uint16_t bg)
{
    uint32_t n;
    if (w > CV_LB)
        w = CV_LB;
    if (w * h > CV_MAX)
        h = CV_MAX / w;
    cv_w = w;
    cv_h = h;
    n = w * h;
    memset(cv_hk, 0, sizeof cv_hk);                     /* a new table: the background is index 0 */
    cv_nc = 0;
    cv_last = 1u << 16;
    memset(cv_px, (int)cv_ci(bg), n);
}

/* canvas rows r0 .. cv_h-1, to screen row y + r0: converted to RGB565 a few rows at a time */
static void cv_blit_from(uint32_t x, uint32_t y, uint32_t r0)
{
    uint32_t w = cv_w, rows = w ? CV_LB / w : 0, r, i, n;
    for (r = r0; rows && r < cv_h; r += n) {
        uint16_t *d = cv_lb[cv_lbi];                    /* (its last transfer ended: lcd_blit waited for it
                                                         * before starting the other buffer's) */
        const uint8_t *s = cv_px + r * w;
        n = cv_h - r < rows ? cv_h - r : rows;
        for (i = 0; i < n * w; i++)
            d[i] = cv_lut[s[i]];
        lcd_blit(x, y + r, w, n, d);
        cv_lbi ^= 1u;
    }
}

static void cv_blit(uint32_t x, uint32_t y) { cv_blit_from(x, y, 0); }

static inline void cv_pseti(int32_t x, int32_t y, uint32_t ci)   /* a colour index (cv_ci) */
{
    y += cv_oy;
    if ((uint32_t)x < cv_w && (uint32_t)y < cv_h)
        cv_px[(uint32_t)y * cv_w + (uint32_t)x] = (uint8_t)ci;
}
static inline void cv_pset(int32_t x, int32_t y, uint16_t c)
{
    y += cv_oy;
    if ((uint32_t)x < cv_w && (uint32_t)y < cv_h)
        cv_px[(uint32_t)y * cv_w + (uint32_t)x] = (uint8_t)cv_ci(c);
}

static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    int32_t x1 = x + w, y1 = y + h + cv_oy, i;
    uint8_t sc = (uint8_t)cv_ci(c);
    y += cv_oy;                             /* clipped once, then filled row by row */
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x1 > (int32_t)cv_w)
        x1 = (int32_t)cv_w;
    if (y1 > (int32_t)cv_h)
        y1 = (int32_t)cv_h;
    for (; y < y1; y++)
        for (i = x; i < x1; i++)
            cv_px[(uint32_t)y * cv_w + (uint32_t)i] = sc;
}

static void cv_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy, guard = 2000;
    while (guard--) {                       /* bounded: a line is never longer than 480 px */
        int32_t e2 = 2 * err;               /* both tests use the same error value */
        cv_pset(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* glyph index of a character: the screen shows capitals only (SLOOP 2.6: bold capitals read better on the
 * small screen), so lower case folds to upper case, Latin-1 too (\xfc -> \xdc; not the division sign, nor sharp
 * s and y diaeresis, which have none); anything missing (and C1 controls) draws as '?' */
static uint32_t glyph(const felucca_font_t *f, uint32_t ch)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 0xE0u && ch <= 0xFEu && ch != 0xF7u))
        ch -= 32u;
    if (ch < f->first || ch > f->last || (ch >= 127u && ch < 160u))
        ch = '?';
    return ch - f->first;
}

/* text, alpha-blended onto the background with colour c; returns the end x */
static int32_t cv_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    uint16_t ramp[16];                                 /* the blend steps' colour indices (0xFFFF: not yet) */
    uint32_t a;
    for (a = 0; a < 16u; a++)
        ramp[a] = 0xFFFFu;                             /* (only the steps a glyph uses take a colour) */
#define CV_RAMP(v) (ramp[v] != 0xFFFFu ? ramp[v] : (ramp[v] = (uint16_t)cv_ci(rgb565_over(c, (v), 15u))))   /* (on black:
                                                                                     * c * v / 15, as before) */
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w, bpr;
        const uint8_t *gd;
        w = f->bw[gi];
        gd = f->data + f->off[gi];
        if (f->bits == 1u) {                           /* on / off pixels: full colour where a bit is set */
            uint32_t sw = w >> f->sh;
            for (gy = 0; gy < f->h; gy++)
                for (gx = 0; gx < w; gx++) {
                    uint32_t i = (gy >> f->sh) * sw + (gx >> f->sh);
                    if ((gd[i >> 3] >> (7u - (i & 7u))) & 1u)
                        cv_pseti(x - f->pad + (int32_t)gx, y + (int32_t)gy, CV_RAMP(15u));
                }
            x += f->adv[gi];
            continue;
        }
        bpr = ((w >> f->sh) + 1u) / 2u;
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t sx = gx >> f->sh, v = gd[(gy >> f->sh) * bpr + sx / 2u];
                v = (sx & 1u) ? (v & 15u) : (v >> 4);
                if (v)
                    cv_pseti(x - f->pad + (int32_t)gx, y + (int32_t)gy, CV_RAMP(v));
            }
        x += f->adv[gi];
    }
#undef CV_RAMP
    return x;
}

static int32_t text_w(const felucca_font_t *f, const char *s)
{
    int32_t w = 0;
    for (; *s; s++)
        w += f->adv[glyph(f, (uint8_t)*s)];
    return w;
}

/* one-shot: text in a box, cleared to black, blitted */
static void draw_text_box(uint32_t x, uint32_t y, uint32_t w, const felucca_font_t *f, const char *s,
                          uint16_t c, int align)
{
    int32_t tw = text_w(f, s), tx = 0;
    cv_begin(w, f->h, C_BLACK);
    if (align == 1)
        tx = ((int32_t)w - tw) / 2;
    else if (align == 2)
        tx = (int32_t)w - tw;
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
    lcd_sync();                     /* one-shots (boot, crash, UBOOT, update) finish here */
}
