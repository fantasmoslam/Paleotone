/* SPDX-License-Identifier: GPL-3.0-only */
/* Firmware-side companion protocol, commands 92..94. Snapshot blocks fit in
 * one USB SysEx ring without waiting. No incoming payload can import state. */
#include "remote_state.c"
enum { ED_REMOTE = 92, ED_REMOTE_BLOCK, ED_REMOTE_INPUT };
static void ed_u14(uint32_t v) { ed_b(v); ed_b(v >> 7); }
static uint32_t ed_r14(const uint8_t *a) { return a[0] | (uint32_t)a[1] << 7; }
static void ed_u32(uint32_t v) { for (uint32_t i = 0; i < 5u; i++) { ed_b(v); v >>= 7; } }
static uint32_t ed_remote_resets;
static int ed_remote_args(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    if (cmd == ED_REMOTE) return (n == 1u && a[0] <= 1u) || (n == 3u && a[0] == 2u);
    if (cmd == ED_REMOTE_BLOCK) return n == 4u;
    if (cmd == ED_REMOTE_INPUT)
        return (n == 3u && a[0] == 0u) ||
               (n == 3u && a[0] == 1u && a[1] < NE && a[2] >= 56u && a[2] <= 72u) ||
               (n == 1u && (a[0] == 2u || a[0] == 3u));
    return 1;
}
static int ed_remote_handle(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    (void)n;
    if (cmd == ED_REMOTE) {
        uint32_t rc = 0;
        if (a[0] == 1u) {
            if (ed_remote_resets != usb.resets) rs_base = 0;
            ed_remote_resets = usb.resets;
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
        return 1;
    }
    if (cmd == ED_REMOTE_BLOCK) {
        uint32_t rev = ed_r14(a), block = ed_r14(a + 2), off = block * RS_BLOCK;
        uint32_t count = off < rs_len ? rs_len - off : 0u;
        if (count > RS_BLOCK) count = RS_BLOCK;
        ed_u14(rev); ed_u14(block);
        if (rev != rs_rev || !count) { ed_b(1); return 1; }
        ed_b(0); ed_u14(count);
        for (uint32_t i = 0; i < count;) {
            uint32_t k, m = 0, size = count - i > 7u ? 7u : count - i;
            for (k = 0; k < size; k++) m |= (uint32_t)(rs_buf[off + i + k] >> 7) << k;
            ed_b(m);
            for (k = 0; k < size; k++) ed_b(rs_buf[off + i++] & 127u);
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
