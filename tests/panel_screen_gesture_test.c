/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/panel_screen_gesture.h"
int main(void)
{
    panel_screen_gesture_t s = {0};
    const uint32_t h=1u<<8, dn=1u, up=2u, all=h|dn|up;
    assert(!panel_screen_gesture(&s,h|up,up,h,dn,up,0,0));
    assert(!panel_screen_gesture(&s,h|up,up,h,dn,up,1,1));
    assert(!panel_screen_gesture(&s,h|up|16u,up,h,dn,up,1,0));
    assert(!panel_screen_gesture(&s,h|dn|up,up,h,dn,up,1,0));
    assert(!panel_screen_gesture(&s,h|up,h,h,dn,up,1,0));
    assert(panel_screen_gesture(&s,h|up,up,h,dn,up,1,0)==all);
    assert(s.serial==1 && s.side==1);
    assert(panel_screen_gesture(&s,h|up,0,h,dn,up,1,0)==all);
    assert(s.serial==1); /* no repeats while held */
    assert(panel_screen_gesture(&s,h,0,h,dn,up,0,0)==all); /* USB lost */
    assert(panel_screen_gesture(&s,dn,0,h,dn,up,0,0)==all); /* reverse release order */
    assert(!panel_screen_gesture(&s,0,0,h,dn,up,0,0));
    assert(panel_screen_gesture(&s,h|dn,dn,h,dn,up,1,0)==all);
    assert(s.serial==2 && s.side==0);
    assert(!panel_screen_gesture(&s,0,0,h,dn,up,1,0));
    s.serial=16383;
    assert(panel_screen_gesture(&s,h|up,up,h,dn,up,1,0)==all && s.serial==0);
    assert(sizeof s <= 16); /* no display buffers or pool allocation */
    puts("physical screen gestures: focus, release order, modal/session guard and serial wrap passed");
    return 0;
}
