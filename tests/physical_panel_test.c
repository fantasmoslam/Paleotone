/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
static uint8_t ed_test_out[664];static uint32_t ed_test_n;
static void ed_b(uint32_t v){assert(ed_test_n<sizeof ed_test_out);ed_test_out[ed_test_n++]=v&127u;}
#include "../firmware/src/editor_remote.c"
static void focus_query(uint32_t enabled){
    uint8_t a[1]={(uint8_t)enabled};ed_test_n=0;
    assert(ed_remote_args(ED_PANEL_FOCUS,a,1));assert(ed_remote_handle(ED_PANEL_FOCUS,a,1));
    assert(ed_test_n==22&&ed_test_out[0]==2);
}
static void chord(uint32_t button){
    web_buttons(1u<<B_HOME);web_frame();
    web_buttons((1u<<B_HOME)|(1u<<button));web_frame();
}
static void release_chord(void){web_buttons(1u<<B_HOME);web_frame();web_buttons(0);web_frame();}
int main(void){
    web_nor_erase();web_boot();
    ui.home=0;ui.page=(uint8_t)page_first(FAM_LFO);ui.cursor=7;ui.bank=0;ui.lane=3;
    uint32_t left_page=ui.page,right_page=page_first(FAM_ENV);
    focus_query(1);
    assert(!panel_view_configure(0,right_page,2));
    int16_t params[NTRK][P_COUNT];for(uint32_t t=0;t<NTRK;t++)memcpy(params[t],trk[t].p,sizeof params[t]);
    int8_t octave=song.octave;uint32_t writes=web_flash_writes;
    song.playing=1;song.rec=0;uint32_t tick=song.tick;
    web_keys(1);web_frame(); /* a note held while swapping is permitted on a musical page */
    chord(B_OCTUP);
    assert(panel_screen.side==1&&song.sel==2&&ui.page==right_page);
    assert(song.playing==1&&song.tick==tick&&song.octave==octave);
    assert(panel_views[0].page==left_page&&panel_views[0].cursor==7&&panel_views[0].lane==3);
    release_chord();
    for(uint32_t t=0;t<NTRK;t++)assert(!memcmp(params[t],trk[t].p,sizeof params[t]));
    assert(web_flash_writes==writes);
    web_keys(0);web_frame();
    int16_t before=trk[2].p[P_ATK];web_enc(EN_K1,1);web_frame();
    assert(trk[2].p[P_ATK]!=before&&trk[0].p[P_ATK]==params[0][P_ATK]);
    ui.cursor=9;ui.lane=1;
    chord(B_OCTDN);assert(panel_screen.side==0&&song.sel==0&&ui.page==left_page);
    assert(ui.cursor==7&&ui.lane==3);release_chord();
    chord(B_OCTUP);assert(song.sel==2&&ui.page==right_page&&ui.cursor==9&&ui.lane==1);
    release_chord();
    /* Audio/transport state is shared and no flash writes are introduced by focus swaps. */
    assert(song.playing==1&&song.tick==tick&&song.octave==octave&&web_flash_writes==writes);
    /* A physical combo in a confirmation dialog must not confirm or change focus. */
    ui.confirm=CF_CLEAR_SEQ;uint32_t serial=panel_screen.serial;
    chord(B_OCTDN);release_chord();
    assert(ui.confirm==CF_CLEAR_SEQ&&panel_screen.side==1&&panel_screen.serial==serial);
    ui.confirm=0;
    assert(rs_capture());uint32_t n=rs_len;
    uint8_t saved[RS_MAX];memcpy(saved,rs_buf,n);
    panel_screen.side=0;panel_views[0].page=0;
    memcpy(rs_buf,saved,n);assert(web_remote_apply(n));
    assert(web_panel_side()==1&&web_panel_context(0,1)==left_page&&web_panel_context(0,4)==7);
    focus_query(0);web_buttons(1u<<B_OCTUP);web_frame();web_buttons(0);web_frame();
    assert(song.octave==octave+1); /* normal octave controls resume with the companion disabled */
    puts("device switching: immediate page/track/cursor restore, held-note swap, isolated knob edit, transport preservation, modal guard and atomic snapshot passed");
    return 0;
}
