/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void)
{
    int bad=test_layer_lock();
    uint32_t i;int ok=1;
    static const char *const names[]={"SONG","SCAPE SETUP","SCAPE ROLES","SCAPE DRONE","SCAPE LIFE","SCAPE LEVELS","SCAPE LEVELS 2","SONG"};
    ui_power_on();frames(400);
    for(i=0;i<NELEM(names);i++){press(B_GLO);frames(400);ok &= str_eq(cur_page()->title,names[i]);}
    bad+=check("GLO slow taps cycle SONG and every advanced SCAPE page, then wrap",ok);
    go_title("SCAPE MUSIC");bad+=check("HOME keeps only movement, mix and habitat",home_through_scape());
    return bad!=0;
}
