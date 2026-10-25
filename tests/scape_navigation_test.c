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
    go_title("SCAPE MUSIC");bad+=check("HOME cycles every SCAPE control in one group",home_through_scape());
    for(i=0;i<NPAGES;i++) if(!memcmp(PAGES[i].title,"SCAPE",5)) bad+=check("every SCAPE page belongs to the same HOME group",PAGES[i].fam==FAM_TRK);
    return bad!=0;
}
