/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
static uint8_t ed_test_out[664];
static uint32_t ed_test_n;
static void ed_b(uint32_t v) { assert(ed_test_n < sizeof ed_test_out); ed_test_out[ed_test_n++] = v & 127u; }
#include "../firmware/src/editor_remote.c"
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
    /* The protocol validates sizes before a[0]/a[1] are read, and controls
     * merge with (rather than overwrite) the physical scanner state. */
    uint8_t bad[3] = {0};
    assert(!ed_remote_args(ED_REMOTE, bad, 0));
    assert(!ed_remote_args(ED_REMOTE_INPUT, bad, 2));
    uint32_t physical = fm1_in.buttons;
    uint8_t press[3] = {0, 1u << B_ENV, 0};
    ed_test_n = 0; assert(ed_remote_handle(ED_REMOTE_INPUT, press, 3));
    assert(fm1_in.buttons == physical);
    assert(panel_buttons() & (1u << panel.btn[B_ENV]));
    assert(panel_remote_edges() & (1u << panel.btn[B_ENV]));
    assert(!panel_remote_edges());
    assert(rs_capture()); n = rs_len;
    memcpy(snapshot, rs_buf, n); panel_remote_clear(); fm1_in.buttons = 0;
    memcpy(rs_buf, snapshot, n); assert(web_remote_apply(n));
    assert(fm1_in.buttons & (1u << panel.btn[B_ENV]));
    fm1_in.buttons = physical;
    panel_remote_turn(EN_K1, 1); assert(panel_enc(EN_K1) == 1);
    fm1_ms += 1501; panel_remote_tick(); assert(!remote_panel.held);
    uint8_t block[4] = {rs_rev & 127u, rs_rev >> 7, 0, 0};
    ed_test_n = 0; assert(ed_remote_handle(ED_REMOTE_BLOCK, block, 4));
    assert(ed_test_n + 6u <= 192u); /* no blocking send for a snapshot block */
    uint8_t burst[7]={rs_rev&127u,rs_rev>>7,4,0,1,2,3}, args[4];
    assert(ed_remote_args(ED_REMOTE_BATCH,burst,sizeof burst));
    ed_test_n=0;assert(ed_remote_handle(ED_REMOTE_BATCH,burst,sizeof burst));
    assert(ed_test_n==4 && ed_test_out[3]==0);
    for(uint32_t used=64;used>4;used--)assert(!ed_stream_take(args,used,usb.resets,1));
    for(uint32_t j=0;j<4;j++){
        assert(ed_stream_take(args,4,usb.resets,1) && args[2]==j);
        ed_test_n=0;assert(ed_remote_handle(ED_REMOTE_BLOCK,args,4));
        assert(ed_test_n+6<=180 && !ed_test_out[4]);
    }
    assert(!ed_stream_take(args,0,usb.resets,1));
    burst[4]=0;assert(!ed_remote_args(ED_REMOTE_BATCH,burst,sizeof burst));burst[4]=1;
    ed_test_n=0;ed_remote_handle(ED_REMOTE_BATCH,burst,sizeof burst);
    assert(!ed_stream_take(args,0,usb.resets+1,1) && !ed_stream.count);
    ed_test_n=0;ed_remote_handle(ED_REMOTE_BATCH,burst,sizeof burst);
    assert(!ed_stream_take(args,0,usb.resets,0) && !ed_stream.count);
    /* The wire compressor must round-trip every native snapshot block.
     * Also feed these exact C payloads to the JS decoder in CI. */
    FILE *compact_file=NULL;
    if(argc>1){char path[1024];snprintf(path,sizeof path,"%s.compact",argv[1]);compact_file=fopen(path,"wb");assert(compact_file);}
    for(uint32_t off=0;off<rs_len;off+=RS_BLOCK){
        uint8_t a[4]={rs_rev&127u,rs_rev>>7,(uint8_t)(off/RS_BLOCK),64};
        ed_test_n=0;assert(ed_remote_handle(ED_REMOTE_BLOCK,a,4));
        assert(ed_test_n+6<=180 && !ed_test_out[4]);
        if(compact_file){uint32_t len=ed_test_n;fwrite(&len,4,1,compact_file);fwrite(ed_test_out,1,len,compact_file);uint32_t raw=rs_len-off;if(raw>128)raw=128;fwrite(rs_buf+off,1,raw,compact_file);}
    }
    if(compact_file)fclose(compact_file);
    rs_ack();
    assert(rs_base == rs_rev);
    assert(rs_capture());
    uint32_t dirty = 0;
    for (uint32_t i = 0; i < sizeof rs_dirty; i++) dirty += rs_dirty[i] != 0;
    assert(dirty < sizeof rs_dirty); /* catalogs/steps stay reusable */
    assert(!web_remote_apply(1));
    /* Representative square layouts, menus and dialogs use the same pixels.
     * Animation OFF makes the comparison independent of polling cadence. */
    for (uint32_t view = 0; view < 4u; view++) {
        ui.menu = ui.confirm = ui.layer = 0; ui.home = view == 0u; ui.msg_t = ui.hot_t = 0;
        ui.page = (uint8_t)page_first(view == 1u ? FAM_SEQ : FAM_ENV);
        if (view == 2u) ui.menu = 1;
        if (view == 3u) ui.confirm = CF_CLEAR_SEQ;
        ui.force = 1; assert(rs_capture()); n = rs_len;
        memcpy(snapshot, rs_buf, n); ui_draw_page(seq_counting()); memcpy(expected, web_fb, sizeof expected);
        memcpy(rs_buf, snapshot, n); assert(web_remote_apply(n));
        assert(!memcmp(expected, web_fb, sizeof expected));
    }
    for(uint32_t page=0;page<NPAGES;page++){
        remote_display=0;ui.home=0;ui.page=(uint8_t)page;ui.menu=ui.confirm=ui.layer=ui.ly=0;
        ui.msg_t=ui.hot_t=0;ui.cursor=ui.bank=0;song.sel=0;ui.force=1;scene_cur=3;scene_req=0xff;
        assert(rs_capture());n=rs_len;memcpy(snapshot,rs_buf,n);
        ui_draw_page(seq_counting());memcpy(expected,web_fb,sizeof expected);
        scene_cur=0;memcpy(rs_buf,snapshot,n);assert(web_remote_apply(n));
        if(memcmp(expected,web_fb,sizeof expected)){fprintf(stderr,"page pixels differ: %u %s\n",page,PAGES[page].title);return 1;}
        assert(scene_cur==3);
    }
    puts("GHOULBOX all-page snapshot pixel fidelity passed");
    puts("remote renderer tests passed");
    return 0;
}
