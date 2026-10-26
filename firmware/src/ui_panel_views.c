/* SPDX-License-Identifier: GPL-3.0-only */
/* Two navigation contexts owned by the instrument. Sound and transport stay shared. */
typedef struct {
    uint8_t home,page,track,bank,cursor,lane,seq_mode,grid;
    uint8_t uslot,ppick,song_row,song_trk,ev_step,ev_id;
    uint16_t ev_row,ev_keep;
    uint8_t fam_last[FAM_COUNT];
} panel_view_t;
static panel_view_t panel_views[2];
static uint8_t panel_views_ready;
static void panel_view_get(panel_view_t *v) {
    v->home=ui.home;v->page=ui.page;v->track=song.sel;v->bank=ui.bank;
    v->cursor=ui.cursor;v->lane=ui.lane;v->seq_mode=song.seq_mode;v->grid=song.grid;
    v->uslot=ui.uslot;v->ppick=ui.ppick;v->song_row=ui.song_row;v->song_trk=ui.song_trk;
    v->ev_step=ui.ev_step;v->ev_id=ui.ev_id;v->ev_row=ui.ev_row;v->ev_keep=ui.ev_keep;
    memcpy(v->fam_last,ui.fam_last,sizeof v->fam_last);
}
static void panel_views_capture(void) {
    if(!panel_views_ready) {
        panel_screen.side=0;
        panel_view_get(&panel_views[0]);panel_views[1]=panel_views[0];
        panel_views[1].home=0;
        for(uint32_t p=0;p<NPAGES;p++)if(str_eq(PAGES[p].title,"MIXER"))panel_views[1].page=(uint8_t)p;
        panel_views[1].seq_mode=panel_views[1].grid=0;panel_views_ready=1;
    }
    panel_view_get(&panel_views[panel_screen.side&1u]);
}
static int panel_view_blocked(void) {
    return ui.menu||ui.confirm||ui.layer||ui.ly||ui.lock||name_on()||ui.uboot||
        mom.n||remote_panel.held||ui.plk_rm||ui.entry_open||(song.grid&&fm1_in.notes);
}
static void panel_view_apply(const panel_view_t *v) {
    track_select(v->track);
    ui.home=v->home;ui.page=v->page;ui.bank=v->bank;ui.cursor=v->cursor;ui.lane=v->lane;
    song.seq_mode=v->seq_mode;song.grid=v->grid;
    ui.uslot=v->uslot;ui.ppick=v->ppick;ui.song_row=v->song_row;ui.song_trk=v->song_trk;
    ui.ev_step=v->ev_step;ui.ev_id=v->ev_id;ui.ev_row=v->ev_row;ui.ev_keep=v->ev_keep;
    memcpy(ui.fam_last,v->fam_last,sizeof ui.fam_last);
    ui.entry_open=ui.act=ui.hot_t=ui.msg_t=0;
    ui.home_t0=ui.rec_t0=ui.save_t0=ui.pg_down=0;
    if(!ui.home&&!page_visible(ui.page))ui.page=(uint8_t)page_first(PAGES[ui.page].fam);
    ui.force=1;sync_reload=1;
}
static void panel_view_switch(uint32_t old_side) {
    panel_view_get(&panel_views[old_side&1u]);
    panel_view_apply(&panel_views[panel_screen.side&1u]);
}
static int panel_view_configure(uint32_t home,uint32_t page,uint32_t track) {
    if(home>1u||page>=NPAGES||track>=NTRK||panel_view_blocked())return 1;
    panel_views_capture();panel_view_t v=panel_views[1];
    v.home=(uint8_t)home;v.page=(uint8_t)page;v.track=(uint8_t)track;
    v.bank=v.cursor=v.lane=0;v.seq_mode=(uint8_t)(!home&&PAGES[page].fam==FAM_SEQ);v.grid=0;
    panel_views[1]=v;if(panel_screen.side==1u)panel_view_apply(&v);return 0;
}
