/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
static uint8_t ed_test_out[664];
static uint32_t ed_test_n;
static void ed_b(uint32_t v) { assert(ed_test_n < sizeof ed_test_out); ed_test_out[ed_test_n++] = v & 127u; }
#include "../firmware/src/editor_remote.c"
int main(void)
{
    web_nor_erase(); web_boot();
    uint8_t enabled[1]={1}, disabled[1]={0}, bad[1]={2};
    assert(!ed_remote_args(ED_PANEL_FOCUS, enabled, 0));
    assert(!ed_remote_args(ED_PANEL_FOCUS, bad, 1));
    assert(ed_remote_args(ED_PANEL_FOCUS, enabled, 1));
    ed_test_n=0; assert(ed_remote_handle(ED_PANEL_FOCUS, enabled, 1));
    assert(ed_test_n==5 && ed_test_out[0]==1);
    uint8_t home=ui.home, page=ui.page, track=song.sel;
    int8_t octave=song.octave;
    int16_t params[P_COUNT]; memcpy(params,TSEL->p,sizeof params);
    uint32_t writes=web_flash_writes;
    web_buttons(1u<<B_HOME); web_frame();
    web_buttons((1u<<B_HOME)|(1u<<B_OCTUP)); web_frame();
    assert(panel_screen.serial==1 && panel_screen.side==1 && panel_screen.suppress);
    assert(ui.home_t0==0 && !ui.menu && song.octave==octave);
    assert(ui.home==home && ui.page==page && song.sel==track);
    for (uint32_t i=0;i<5;i++) web_frame();
    assert(panel_screen.serial==1);
    web_buttons(1u<<B_HOME); web_frame();
    assert(!panel_buttons() && song.octave==octave && ui.home==home);
    fm1_ms+=3100; /* expiry while held must still swallow the eventual release */
    web_buttons(0); web_frame();
    assert(!panel_screen.suppress && ui.home==home && !ui.menu);
    assert(!memcmp(params,TSEL->p,sizeof params) && web_flash_writes==writes);
    ed_test_n=0; ed_remote_handle(ED_PANEL_FOCUS,enabled,1);
    web_buttons(1u<<B_HOME); web_frame();
    web_buttons((1u<<B_HOME)|(1u<<B_OCTDN)); web_frame();
    assert(panel_screen.serial==2 && panel_screen.side==0);
    web_buttons(1u<<B_OCTDN); web_frame(); web_buttons(0); web_frame();
    assert(song.octave==octave && ui.home==home);
    ed_test_n=0; ed_remote_handle(ED_PANEL_FOCUS,disabled,1);
    web_buttons(1u<<B_OCTUP); web_frame(); web_buttons(0); web_frame();
    assert(song.octave==octave+1 && panel_screen.serial==2);
    ed_test_n=0; ed_remote_handle(ED_PANEL_FOCUS,enabled,1); usb.resets++;
    web_buttons(1u<<B_HOME); web_frame();
    web_buttons((1u<<B_HOME)|(1u<<B_OCTUP)); web_frame();
    assert(panel_screen.serial==2); /* stale USB lease cannot eat normal controls */
    web_buttons(0); web_frame();
    puts("physical panel integration: real UI chord suppression, release order, no parameter/flash changes, lease expiry and USB reset passed");
    return 0;
}
