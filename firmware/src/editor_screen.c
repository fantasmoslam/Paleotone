/* SPDX-License-Identifier: GPL-3.0-only */
/* Second screen (read-only), EDITOR_PROTOCOL.md "Second screen (90, 91)".
 * ED_SCREEN (90): reply and WATCH push, ED_SCR_N bytes of what the screen shows (all 0..127).
 * ED_PAGE_DESC (91): [index] -> index, NPAGES, title, fam, scope, graph, 4 param ids (127 = empty).
 * Nothing here changes the device: it only reads ui and PAGES. */
enum { ED_SCREEN = 90, ED_PAGE_DESC };
#define ED_SCR_N 12
static uint8_t ed_scr_sh[ED_SCR_N];                      /* what the host was last told */

static void ed_scr_get(uint8_t *o)
{
    o[0] = ui.home;      o[1] = ui.page;     o[2] = ui.menu;     o[3] = ui.menu_sel;
    o[4] = ui.confirm;   o[5] = ui.layer;    o[6] = ui.bank;     o[7] = ui.cursor;
    o[8] = ui.lane;      o[9] = ui.song_row; o[10] = ui.song_trk; o[11] = ui.uslot;
    o[3] = 127;                                           /* MENU: the row as the editor's MENU_DESC index */
    for (uint32_t m = 0; m < ED_MENU_N; m++)
        if (ED_MENU[m][0] == ui.menu_sel)
            o[3] = (uint8_t)m;
    for (uint32_t i = 0; i < ED_SCR_N; i++)
        o[i] &= 0x7Fu;
}

static void ed_screen_push(void)                         /* from ed_sync, while WATCHing */
{
    uint8_t o[ED_SCR_N];
    uint32_t i;
    ed_scr_get(o);
    for (i = 0; i < ED_SCR_N && o[i] == ed_scr_sh[i]; i++)
        ;
    if (i == ED_SCR_N || !ed_room())
        return;
    ed_begin(ED_SCREEN);
    for (i = 0; i < ED_SCR_N; i++) {
        ed_scr_sh[i] = o[i];
        ed_b(o[i]);
    }
    ed_send();
}

static int ed_screen_handle(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    (void)n;
    if (cmd == ED_SCREEN) {
        uint8_t o[ED_SCR_N];
        ed_scr_get(o);
        for (uint32_t i = 0; i < ED_SCR_N; i++)
            ed_b(o[i]);
        return 1;
    }
    if (cmd == ED_PAGE_DESC) {
        uint32_t k = a[0];
        ed_b(k);
        ed_b(NPAGES);
        if (k < NPAGES) {
            const page_t *pg = &PAGES[k];
            ed_str(pg->title, 12);
            ed_b(pg->fam); ed_b(pg->scope); ed_b(pg->graph);
            for (uint32_t i = 0; i < 4u; i++)
                ed_b(pg->id[i] == 0xFFu ? 127u : pg->id[i]);
        }
        return 1;
    }
    return 0;
}
