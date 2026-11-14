/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p)
{ (void)x;(void)y;(void)w;(void)h;(void)p; }
#define __attribute__(x)
#include "../../firmware/src/gfx.c"
static uint8_t unpacked[65536];
static uint16_t offsets[256], reference[CV_MAX];
static void check(const felucca_font_t *font)
{
    felucca_font_t alpha = *font;
    unsigned gi, y, x, pos=0;
    char text[2];
    if (!font->bits) return; /* Optional antialiased font sets keep their 4-bit path. */
    for(gi=0;gi<=(unsigned)(font->last-font->first);gi++) {
        unsigned w=font->bw[gi], bpr=(w+1)/2;
        const uint8_t *p=font->data+font->off[gi];
        offsets[gi]=(uint16_t)pos;
        for(y=0;y<font->h;y++) for(x=0;x<w;x++) {
            unsigned i=y*w+x, a=((p[i/8]>>(7-i%8))&1)*15;
            unpacked[pos+y*bpr+x/2] |= (uint8_t)(a << ((x&1)?0:4));
        }
        pos += font->h*bpr;
        assert(pos<sizeof unpacked);
    }
    alpha.data=unpacked; alpha.off=offsets; alpha.bits=0;
    text[1]=0;
    for(gi=32;gi<=126;gi++) for(int mode=0;mode<4;mode++) {
        text[0]=(char)gi;
        int px=mode==0?-4:mode==1?235:8, py=mode==2?135:mode==3?8:-2;
        cv_begin(240,144,RGB(32,80,64));
        cv_text(px,py,&alpha,text,RGB(240,90,180));
        memcpy(reference,cv_px,sizeof reference);
        cv_begin(240,144,RGB(32,80,64));
        cv_text(px,py,font,text,RGB(240,90,180));
        assert(!memcmp(reference,cv_px,sizeof reference));
        assert(text_w(&alpha,text)==text_w(font,text));
    }
    memset(unpacked,0,sizeof unpacked);
}
int main(void)
{
    const felucca_font_t *fonts[]={&FONT_XS,&FONT_S,&FONT_B,&FONT_M,&FONT_L,&FONT_C,&FONT_US,&FONT_UM,&FONT_UL,&FONT_UN,&FONT_UH};
    for(unsigned i=0;i<sizeof fonts/sizeof fonts[0];i++) check(fonts[i]);
    const char *tracks[]={"DRONE","SEQ","LEAD","SEQ 2","ATMOS","SKIN","CALLS"};
    for(unsigned i=0;i<sizeof tracks/sizeof tracks[0];i++) assert(text_w(&FONT_UH,tracks[i])<=56);
    assert(FONT_UH.h==32 && FONT_UN.h==24 && text_w(&FONT_UN,"REVERB")<=45);
    puts("font renderer: packed/alpha pixels and clipping match for every packed font");
    return 0;
}
