/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
static uint8_t snapshot[RS_MAX];
static uint16_t expected[240 * 240];
int main(int argc, char **argv)
{
    web_nor_erase(); web_boot();
    ui_prefs |= PREF_ANIM_OFF;
    ui.home = 0; ui.page = (uint8_t)page_first(FAM_ENV);
    ui.menu = ui.confirm = ui.layer = ui.msg_t = ui.hot_t = 0;
    ui.force = 1;
    trk[0].p[P_ATK] = 41; trk[0].p[P_SUS] = 72;
    assert(rs_capture() && rs_len < RS_MAX);
    printf("display snapshot: %u bytes / %u capacity\n", rs_len, RS_MAX);
    uint32_t n = rs_len;
    memcpy(snapshot, rs_buf, n);
    ui_draw_page(seq_counting());
    memcpy(expected, web_fb, sizeof expected);
    song.sel = 3; trk[0].p[P_ATK] = 0; ui.home = 1;
    memcpy(rs_buf, snapshot, n);
    assert(web_remote_apply(n));
    assert(song.sel == 0 && trk[0].p[P_ATK] == 41 && ui.home == 0);
    assert(!memcmp(expected, web_fb, sizeof expected));
    web_remote_expand();
    for (uint32_t y = 0; y < 240u; y++)
        assert(!memcmp(remote_wide + y * 480u, web_fb + y * 240u, 480));
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb"); assert(f);
        fwrite(&n, 4, 1, f); fwrite(snapshot, 1, n, f);
        fwrite(expected, sizeof expected, 1, f);
        fwrite(remote_wide, sizeof remote_wide, 1, f); fclose(f);
    }
    rs_ack();
    assert(rs_base == rs_rev);
    assert(rs_capture());
    uint32_t dirty = 0;
    for (uint32_t i = 0; i < sizeof rs_dirty; i++) dirty += rs_dirty[i] != 0;
    assert(dirty < sizeof rs_dirty); /* catalogs/steps stay reusable */
    assert(!web_remote_apply(1));
    puts("remote renderer tests passed");
    return 0;
}
