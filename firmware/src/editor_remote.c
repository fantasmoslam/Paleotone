/* SPDX-License-Identifier: GPL-3.0-only */
/* Firmware-side companion protocol, commands 92..94. Snapshot blocks fit in
 * one USB SysEx ring without waiting. No incoming payload can import state. */
#include "remote_state.c"
#include "remote_codec.h"
enum { ED_REMOTE = 92, ED_REMOTE_BLOCK, ED_REMOTE_INPUT, ED_REMOTE_BATCH, ED_PANEL_FOCUS = 96 };
static void ed_u14(uint32_t v) { ed_b(v); ed_b(v >> 7); }
static uint32_t ed_r14(const uint8_t *a) { return a[0] | (uint32_t)a[1] << 7; }
static void ed_u32(uint32_t v) { for (uint32_t i = 0; i < 5u; i++) { ed_b(v); v >>= 7; } }
static uint32_t ed_remote_resets;
static struct { uint32_t resets; uint16_t rev; uint8_t count,pos,compact,block[4]; } ed_stream;
static int ed_remote_args(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    if(cmd==ED_PANEL_FOCUS)return (n==1u&&a[0]<=1u)||(n==4u&&a[0]==2u&&a[1]<=1u&&a[2]<NPAGES&&a[3]<NTRK);
    if (cmd == ED_REMOTE) return (n == 1u && a[0] <= 1u) || (n == 3u && a[0] == 2u);
    if (cmd == ED_REMOTE_BLOCK) return n == 4u && !(a[3]&64u);
    if(cmd==ED_REMOTE_BATCH) {
        if(n<4u)return 0;
        uint32_t count=a[2]&~64u;
        if(count<1u||count>4u||n!=3u+count)return 0;
        for(uint32_t i=0;i<count;i++) {
            if(a[3+i]>=RS_BLOCKS)return 0;
            for(uint32_t j=0;j<i;j++)if(a[3+i]==a[3+j])return 0;
        }
        return 1;
    }
    if (cmd == ED_REMOTE_INPUT)
        return (n == 3u && a[0] == 0u) ||
               (n == 3u && a[0] == 1u && a[1] < NE && a[2] >= 56u && a[2] <= 72u) ||
               (n == 1u && (a[0] == 2u || a[0] == 3u));
    return 1;
}
static int ed_remote_handle(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    (void)n;
    if (cmd == ED_PANEL_FOCUS) {
        panel_views_capture();uint32_t rc=0;
        if(a[0]==2u)rc=(uint32_t)panel_view_configure(a[1],a[2],a[3]);else panel_screen.enabled=a[0];
        panel_screen.last_ms=fm1_ms;panel_screen.resets=usb.resets;
        ed_b(2);ed_b(rc);ed_b(panel_screen.side);ed_u14(panel_screen.serial);ed_b(panel_screen.suppress!=0u);
        for(uint32_t side=0;side<2u;side++){
            const panel_view_t *v=&panel_views[side];
            ed_b(v->home);ed_b(v->page);ed_b(v->track);ed_b(v->bank);
            ed_b(v->cursor);ed_b(v->lane);ed_b(v->seq_mode);ed_b(v->grid);
        }
        return 1;
    }
    if(cmd==ED_REMOTE_BATCH) {
        uint32_t rc=ed_r14(a)!=rs_rev||!rs_len;
        uint32_t count=a[2]&~64u;
        for(uint32_t i=0;i<count;i++)if((uint32_t)a[3+i]*RS_BLOCK>=rs_len)rc=1;
        if(!rc) {
            ed_stream.rev=(uint16_t)rs_rev;ed_stream.count=(uint8_t)count;ed_stream.compact=(a[2]&64u)!=0u;ed_stream.pos=0;ed_stream.resets=usb.resets;
            memcpy(ed_stream.block,a+3,count);
        }
        ed_u14(ed_r14(a));ed_b(a[2]);ed_b(rc);return 1;
    }
    if (cmd == ED_REMOTE) {
        uint32_t rc = 0;
        if (a[0] == 1u) {
            if (ed_remote_resets != usb.resets) rs_base = 0;
            ed_remote_resets = usb.resets;
            ed_stream.count=0;
            if (!rs_capture()) rc = 1;
        } else if (a[0] == 2u) {
            if (ed_r14(a + 1) != rs_rev || !rs_len) rc = 2;
            else rs_ack();
        }
        ed_b(a[0]); ed_b(rc); ed_b(RS_SCHEMA); ed_u32(PALEOTONE_RENDER_ID);
        ed_u14(rs_rev); ed_u14(rs_base); ed_u14(rs_len);
        for (uint32_t i = 0; i < sizeof rs_dirty; i++) {
            ed_b(rs_dirty[i] & 127u); ed_b(rs_dirty[i] >> 7);
        }
        ed_b(4u|64u); /* four-block bursts; bit 6 negotiates lossless compression */
        return 1;
    }
    if (cmd == ED_REMOTE_BLOCK) {
        uint32_t rev = ed_r14(a), block = a[2] | (uint32_t)(a[3]&63u)<<7, off = block * RS_BLOCK;
        uint32_t count = off < rs_len ? rs_len - off : 0u;
        if (count > RS_BLOCK) count = RS_BLOCK;
        ed_u14(rev); ed_u14(block);
        if (rev != rs_rev || !count) { ed_b(1); return 1; }
        ed_b(0); ed_u14(count);
        uint8_t packed[RS_BLOCK];const uint8_t *data=rs_buf+off;
        if(a[3]&64u) {
            uint32_t compressed=remote_pack(data,count,packed);
            ed_b(compressed!=0u);
            if(compressed){data=packed;count=compressed;}
            ed_u14(count);
        }
        for (uint32_t i = 0; i < count;) {
            uint32_t k, m = 0, size = count - i > 7u ? 7u : count - i;
            for (k = 0; k < size; k++) m |= (uint32_t)(data[i + k] >> 7) << k;
            ed_b(m);
            for (k = 0; k < size; k++) ed_b(data[i++] & 127u);
        }
        return 1;
    }
    if (cmd == ED_REMOTE_INPUT) {
        if (a[0] == 0u) panel_remote_buttons(ed_r14(a + 1));
        else if (a[0] == 1u) panel_remote_turn(a[1], (int32_t)a[2] - 64);
        else if (a[0] == 2u) panel_remote_clear();
        else remote_panel.last_ms = fm1_ms;
        ed_b(a[0]); ed_b(0);
        return 1;
    }
    return 0;
}

/* One ordinary block reply per service pass, only with room for the entire <=162-byte SysEx.
 * Never fill/spin on the USB ring, allocate another snapshot, or enter the audio ISR. */
static int ed_stream_take(uint8_t *a,uint32_t used,uint32_t resets,uint32_t connected)
{
    if(!connected||ed_stream.resets!=resets){ed_stream.count=0;return 0;}
    if(ed_stream.pos>=ed_stream.count||used>4u)return 0;
    a[0]=(uint8_t)(ed_stream.rev&127u);a[1]=(uint8_t)(ed_stream.rev>>7);
    a[2]=ed_stream.block[ed_stream.pos++];a[3]=ed_stream.compact?64u:0u;
    return 1;
}
