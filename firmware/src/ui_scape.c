/* SPDX-License-Identifier: GPL-3.0-only */
/* Persistent SCAPE surface. Cached strips reuse the existing small canvas. */
static void draw_scape(void)
{
    static const char *const TAB[]={"MUSIC","MIX","WORLD","LAYERS"};
    static const char *const BANK[]={"SHAPE","HARMONY","TRACK ROLES","VOICES","HABITAT","DRONE","CREATURES","WEATHER","WILDLIFE","SCENE"};
    static const uint8_t GROUP[]={0,0,0,1,2,2,2,3,3,0};
    static const uint8_t POS[]={0,1,2,0,0,1,2,0,1,3}, COUNT[]={4,1,3,2};
    static const char *const ROLE[]={"BEAST","BASS","BED","MELO","PULSE"};
    uint32_t bank=scape_bank(), group=GROUP[bank], sig, i;
    char b[24], v[12]; const char *unit;
    const page_t *pg=cur_page();
    if(ui.force) lcd_fill(0,0,240,240,T_BG);

    sig=(uint32_t)song.g[G_BPM] | (uint32_t)song.playing<<10 | (uint32_t)song.g[G_DREAM]<<11 |
        (uint32_t)scape_music.active<<12 | (uint32_t)scape_music.suspended<<13 |
        (uint32_t)(ui.bpm_t!=0)<<14 | (uint32_t)song.g[G_SCHOLD]<<15;
    if(ui.force || sig!=ui.head_sig) {
        ui.head_sig=sig; cv_begin(240,28,T_BG);
        cv_text_on(8,4,&AF_M,"SCAPE",T_TEXT,T_BG);
        cv_text_on(78,6,&AF_S,scape_music.suspended?"SONG":!scape_music.active?"READY":
            !song.playing?"PAUSED":song.g[G_DREAM]&&!song.g[G_SCHOLD]?"EVOLVING":"HELD",T_THEME,T_BG);
        fmt_int(b,song.g[G_BPM]);
        cv_text_r(231,4,&AF_M,b,ui.bpm_t?T_ACCENT:T_TEXT,T_BG);
        cv_blit(0,0);
    }
    sig=bank | (uint32_t)scape_music.active<<8 | (uint32_t)scape_music.chord_pc<<9 |
        (uint32_t)song.g[G_DMOOD]<<16;
    if(ui.force || sig!=ui.foot_sig) {
        ui.foot_sig=sig; cv_begin(240,46,T_BG);
        for(i=0;i<4;i++) {
            uint16_t bg=i==group?T_THEME:T_SURF, fg=i==group?T_INK:T_MID;
            cv_rect(3+59*i,0,57,19,bg);
            cv_text_in(3+59*i,2,57,&AF_S,TAB[i],fg,bg);
        }
        cv_text_on(8,25,&AF_M,BANK[bank],T_TEXT,T_BG);
        if(scape_music.active) {
            str_cpy(b,N_NOTE[scape_music.chord_pc%12],sizeof b);
            str_cpy(b+str_len(b)," ",2);
            str_cpy(b+str_len(b),N_DMOOD[clamp(song.g[G_DMOOD],0,5)],8);
            cv_text_r(231,27,&AF_S,b,T_THEME,T_BG);
        }
        for(i=0;i<COUNT[group];i++) cv_rect(8+i*12,43,8,2,i==POS[bank]?T_THEME:T_DIM);
        cv_blit(0,28);
    }
    /* Actual loop positions and quantized director gains. */
    sig=0;
    for(i=0;i<NTRK;i++) {
        sig=sig*16777619u+(uint32_t)trk[i].seq_idx;
        sig=sig*16777619u+(uint32_t)trk[i].p[P_MUTE];
        sig=sig*16777619u+(uint32_t)trk[i].p[P_SLEN];
        sig=sig*16777619u+scm_role(i);
        sig=sig*16777619u+(scape_music.active?(uint32_t)clamp(scape_music.gain[i]/2048,0,16):16u);
    }
    sig^=(uint32_t)song.playing<<30;
    if(ui.force || sig!=ui.graph_sig) {
        ui.graph_sig=sig; cv_begin(240,80,T_SURF);
        for(i=0;i<NTRK;i++) {
            uint32_t len=(uint32_t)clamp(trk[i].p[P_SLEN],1,NSTEP), pos=trk[i].seq_idx%len;
            uint32_t gain=scape_music.active?(uint32_t)clamp(scape_music.gain[i]/2048,0,16):16u;
            int32_t y=4+(int32_t)i*19;
            uint16_t fg=trk[i].p[P_MUTE]?T_DIM:T_THEME;
            b[0]=(char)('1'+i);b[1]=0;cv_text_on(7,y,&AF_S,b,T_MID,T_SURF);
            cv_text_on(21,y,&AF_S,ROLE[scm_role(i)],fg,T_SURF);
            cv_rect(66,y+4,104,6,T_RAISE);
            for(uint32_t j=0;j<8;j++) cv_rect(66+j*13,y+4,1,6,T_SURF);
            cv_rect(66+(int32_t)(pos*100u/len),y+2,3,10,song.playing?fg:T_MID);
            fmt_int(b,(int32_t)len);cv_text_r(194,y,&AF_S,b,T_MID,T_SURF);
            cv_rect(204,y+4,28,6,T_RAISE);
            if(!trk[i].p[P_MUTE] && gain) cv_rect(204,y+4,(int32_t)(gain*28/16),6,fg);
        }
        cv_blit(0,77);
    }
    for(i=0;i<4;i++) {
        int16_t *vp; const param_desc_t *d=page_desc(pg,i,&vp);
        uint16_t fg=VAL(i); int32_t ratio;
        if(!d || !vp) continue;
        param_format(d,*vp,v,&unit);
        str_cpy(b,d->label,sizeof b);str_cpy(b+str_len(b),v,12);
        uint32_t hot=ui.hot_t && ui.hot_col==i;
        if(!ui.force && str_eq(ui.col[i],b) && (uint8_t)ui.roll[i].sig==hot) continue;
        str_cpy(ui.col[i],b,sizeof ui.col[i]);ui.roll[i].sig=(uint8_t)hot;
        cv_begin(57,62,T_SURF);
        cv_text_in(0,4,57,&AF_S,d->label,hot?T_ACCENT:T_MID,T_SURF);
        const aafont_t *f=text_w(&AF_M,v)<=51?&AF_M:&AF_S;
        cv_text_in(0,23,57,f,v,fg,T_SURF);
        ratio=RATIO(d,enum_rank(d,*vp));
        cv_rect(5,48,47,3,T_RAISE);
        if(ratio>0) cv_rect(5,48,47*ratio/1000,3,fg);
        b[0]=(char)('1'+i);b[1]=0;
        cv_text_in(0,52,57,&AF_S,b,T_DIM,T_SURF);
        cv_blit(3+59*i,160);
    }
    cv_begin(240,16,T_BG);
    cv_text_in(0,1,240,&AF_S,ui.msg_t?ui.msg:bank==0?"SELECT: BANK / KEY: COMPOSE":"SELECT: BANK / HOME: EXIT",ui.msg_t?T_ACCENT:T_MID,T_BG);
    cv_blit(0,224);
}
