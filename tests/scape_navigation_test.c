/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#define UI_TEST_WITH_CASES 1
#include "ui_test.c"
int main(void)
{
    int bad=test_layer_lock();
    uint32_t i;int ok=1;
    static const char *const names[]={"SONG","SONG"};
    ui_power_on();frames(400);
    for(i=0;i<NELEM(names);i++){press(B_GLO);frames(400);ok &= str_eq(cur_page()->title,names[i]);}
    bad+=check("GLO remains SONG; SCAPE never requires a menu switch",ok);
    go_title("SCAPE MUSIC");
    int16_t bpm=song.g[G_BPM];
    bad+=check("SELECT reaches every bank; HOME exits the workspace",home_through_scape());
    bad+=check("bank navigation never changes tempo",song.g[G_BPM]==bpm);
    go_title("SCAPE MUSIC");turn(EN_SELECT,-12);
    bad+=check("bank navigation stops at the first bank",scape_bank()==0);
    turn(EN_SELECT,100);
    bad+=check("bank navigation stops at the last bank",scape_bank()==9 && song.g[G_BPM]==bpm);
    turn(EN_SELECT,-6);
    int16_t bed=song.g[G_SBED];turn(EN_K1,-1);
    bad+=check("visible voice bank edits its own mix",scape_bank()==3 && song.g[G_SBED]==bed-1);
    btn_down(B_GLO);frame();turn(EN_SELECT,1);btn_up(B_GLO);frame();
    bad+=check("GLO+SELECT changes tempo without leaving the bank",scape_bank()==3 && song.g[G_BPM]==bpm+1);
    for(i=0;i<NPAGES;i++) if(!memcmp(PAGES[i].title,"SCAPE",5)) bad+=check("every SCAPE page belongs to the same HOME group",PAGES[i].fam==FAM_TRK);
    return bad!=0;
}
