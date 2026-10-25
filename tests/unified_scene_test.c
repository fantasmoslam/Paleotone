/* SPDX-License-Identifier: GPL-3.0-only */
/* The combined scene through the real mixer and native companion UI. No hardware timing claim. */
#include "../web/emu/felucca_web.c"
#include <assert.h>
#include <time.h>
int main(void)
{
    uint32_t t,i,k,peak=0,owned;
    int32_t out[CTL*2];
    clock_t start;
    web_nor_erase();web_boot();
    for(t=0;t<NTRK;t++) {
        host_preset(&trk[t],ENGI_SAURUS,t==0?5:t==1?1:t==2?20:21);
        trk[t].p[P_LEVEL]=80;
        trk[t].p[P_AMODE]=0;
    }
    scape_biome(1);song.g[G_SCAPE]=100;song.g[G_SDRN]=70;
    song.g[G_SCTRK]=2;song.g[G_SCRATE]=127;song.g[G_SDNGR]=127;
    /* Four held creature parts, with overlapping new calls and automatic passes. */
    for(t=0;t<NTRK;t++)trk_note_on(&trk[t],48+t*3,96);
    ui.home=0;ui.page=(uint8_t)page_first(FAM_ENV);
    panel_screen.enabled=1;
    assert(!panel_view_configure(0,page_first(FAM_LFO),2));
    start=clock();
    for(k=0;k<FS*30u;k+=CTL) {
        mix_block(out,CTL);
        for(i=0;i<CTL*2;i++) {
            uint32_t a=(uint32_t)(out[i]<0?-out[i]:out[i]);
            if(a>peak)peak=a;
            assert(a<=32768u);
        }
        if(k%(CTL*100u)==0) {
            /* The instrument navigation path; no synthesized editor key events. */
            uint32_t old=panel_screen.side;
            panel_screen.side^=1u;
            panel_view_switch(old);
            assert(rs_capture());
            owned=0;
            for(i=0;i<SAUR_SLOTS;i++)owned+=saur_slot[i].owner && saur_slot[i].owner->active;
            assert(owned<=4 && voices_busy()<=NVOICE);
        }
    }
    for(t=0;t<NTRK;t++)for(i=0;i<NVOICE;i++)if(trk[t].v[i].gate)trk_note_off(&trk[t],trk[t].v[i].note);
    song.g[G_SCAPE]=0;mix_block(out,CTL);
    assert(!sc.cnote && !sc.rnote && !sc.on && peak>100);
    printf("unified scene: 30 s, four SAURUS tracks + full SCAPE + calls/passes + alternating snapshots; peak %u; host %.3f s\n",
           peak,(double)(clock()-start)/CLOCKS_PER_SEC);
    return 0;
}
