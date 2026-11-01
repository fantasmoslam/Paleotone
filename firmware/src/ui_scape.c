/* SPDX-License-Identifier: GPL-3.0-only */
/* SCAPE keeps the proven knob cards and transport, with one shared performance
 * panel underneath. Its cached native strip is also the companion's renderer. */
static void draw_scape(void)
{
    static const char *const TAB[]={"MUSIC","MIX","WORLD","LAYERS"};
    static const char *const ROLE[]={"BEAST","BASS","BED","MELO","PULSE"};
    static const uint8_t GROUP[]={0,0,0,1,2,2,2,3,3,0};
    uint32_t bank=scape_bank(), group=GROUP[bank], i, sig=bank;
    char b[24];
    if(ui.force) lcd_fill(0,0,240,240,T_BG);
    draw_head();
    draw_columns();
    sig=sig*131u+song.playing+2u*scape_music.active+4u*scape_music.suspended+
        8u*(uint32_t)song.g[G_DREAM]+16u*(uint32_t)song.g[G_SCHOLD]+32u*scape_music.chord_pc;
    for(i=0;i<NTRK;i++)
        sig=sig*16777619u+trk[i].seq_idx+128u*(uint32_t)trk[i].p[P_SLEN]+
            16384u*(uint32_t)trk[i].p[P_MUTE]+32768u*scm_role(i)+
            262144u*(uint32_t)clamp(scape_music.gain[i]/2048,0,16);
    if(ui.force || sig!=ui.graph_sig) {
        ui.graph_sig=sig; cv_begin(240,124,T_SURF);
        for(i=0;i<4;i++) {
            cv_text_in(3+59*i,1,57,&AF_S,TAB[i],i==group?T_THEME:T_MID,T_SURF);
            if(i==group) cv_rect(9+59*i,20,45,2,T_THEME);
        }
        cv_text_on(8,25,&AF_M,cur_page()->title,T_TEXT,T_SURF);
        str_cpy(b,scape_music.active?N_NOTE[scape_music.chord_pc%12]:"",sizeof b);
        str_cpy(b+str_len(b),scape_music.suspended?" SONG":!scape_music.active?"READY":
            !song.playing?" PAUSE":(!song.g[G_DREAM]||song.g[G_SCHOLD])?" HOLD":" RUN",10);
        cv_text_r(232,27,&AF_S,b,T_THEME,T_SURF);
        for(i=0;i<NTRK;i++) {
            uint32_t len=(uint32_t)clamp(trk[i].p[P_SLEN],1,NSTEP), pos=trk[i].seq_idx%len;
            int32_t y=47+19*(int32_t)i;
            uint16_t fg=trk[i].p[P_MUTE]?T_DIM:T_THEME;
            b[0]=(char)('1'+i);b[1]=0;
            cv_text_on(8,y,&AF_S,b,T_MID,T_SURF);
            cv_text_on(22,y,&AF_S,ROLE[scm_role(i)],fg,T_SURF);
            cv_rect(68,y+5,105,4,T_RAISE);
            cv_rect(68+(int32_t)(pos*100u/len),y+2,3,10,song.playing?fg:T_MID);
            fmt_int(b,(int32_t)len);cv_text_r(195,y,&AF_S,b,T_MID,T_SURF);
            cv_rect(205,y+5,27,4,T_RAISE);
            uint32_t gain=scm_running()?(uint32_t)clamp(scape_music.gain[i]/2048,0,16):16u;
            if(!trk[i].p[P_MUTE]&&gain)cv_rect(205,y+5,(int32_t)(gain*27/16),4,fg);
        }
        cv_blit(0,76);
    }
    cv_begin(240,38,T_BG);
    cv_text_in(0,1,240,&AF_S,ui.msg_t?ui.msg:"SELECT: BANK / HOME: EXIT",ui.msg_t?T_ACCENT:T_MID,T_BG);
    cv_text_in(0,19,240,&AF_S,bank==0?"KEY: COMPOSE":"GLO+SELECT: BPM",T_MID,T_BG);
    cv_blit(0,202);
}
