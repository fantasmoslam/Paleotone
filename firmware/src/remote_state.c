#pragma once
/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared hardware -> browser display projection, encoded field by field.
 * Never copy track_t/voice_t or a native struct image: they contain pointers,
 * padding and architecture-dependent state. Only the browser can import. */
#ifndef PALEOTONE_RENDER_ID
#define PALEOTONE_RENDER_ID 1u
#endif
#define RS_MAX 16384u
#define RS_BLOCK 128u
#define RS_BLOCKS (RS_MAX / RS_BLOCK)
#define RS_SCHEMA 2u
#include "remote_catalog.h"
static uint8_t rs_buf[RS_MAX] __attribute__((section(".pool")));
static uint8_t rs_old[RS_MAX] __attribute__((section(".pool")));
static uint8_t rs_dirty[RS_BLOCKS / 8u];
static uint32_t rs_pos, rs_len, rs_rev, rs_base, rs_read, rs_error;
static remote_project_t rs_projects[4];
static smp_zone_t rs_zone;
static uint8_t rs_zone_valid;
static uint32_t rs_io(uint32_t v, uint32_t n)
{
    uint32_t k, out = 0;
    if (rs_pos + n > RS_MAX) { rs_error = 1; return 0; }
    for (k = 0; k < n; k++) {
        if (rs_read) out |= (uint32_t)rs_buf[rs_pos++] << (8u * k);
        else rs_buf[rs_pos++] = (uint8_t)(v >> (8u * k));
    }
    return rs_read ? out : v;
}
#define RS(v,n) do { if (rs_read) (v) = rs_io(0,n); else (void)rs_io((uint32_t)(v),n); } while (0)
#define RS8(v) RS(v,1)
#define RS16(v) RS(v,2)
#define RS32(v) RS(v,4)
#define RSA8(v) do { for (uint32_t zz = 0; zz < sizeof(v); zz++) RS8(((uint8_t *)&(v))[zz]); } while (0)
static void rs_walk(void)
{
    uint32_t i, j, k;
    RS32(fm1_ms);
    for (i = 0; i < G_COUNT; i++) RS16(song.g[i]);
    RS8(song.playing); RS8(song.seq_mode); RS8(song.grid); RS8(song.rec); RS8(song.sel); RS8(song.octave);
    RS32(song.tick); RS32(song.cpu_q8); RS32(song.master_q12); RS32(song.batt_raw);
    RS8(ui.home);
    RS8(ui.page);
    RS8(ui.bank);
    RS8(ui.cursor);
    RS8(ui.entry_open);
    RS8(ui.lane);
    RS8(ui.plk_src);
    RS8(ui.plk_lane);
    RS8(ui.plk_trk);
    RS8(ui.hot_col);
    RS8(ui.hot_t);
    RS8(ui.menu);
    RS8(ui.menu_sel);
    RS8(ui.msg_t);
    RS8(ui.torches);
    RS8(ui.torch_f);
    RS8(ui.bpm_t);
    RS8(ui.act);
    RS8(ui.confirm);
    RS8(ui.confirm_trk);
    RS8(ui.uslot);
    RS8(ui.ppick);
    RS8(ui.song_row);
    RS8(ui.song_trk);
    RS8(ui.ev_step);
    RS8(ui.ev_id);
    RS8(ui.uboot);
    RS8(ui.ly);
    RS8(ui.layer);
    RS8(ui.lock);
    RS16(ui.menu_scroll);
    RS16(ui.ev_row);
    RS16(ui.ev_keep);
    RS16(ui.pg_down);
    RS32(ui.home_t0);
    RS32(ui.rec_t0);
    RS32(ui.save_t0);
    RS32(ui.ly_t0);
    RS32(ui.frame);
    RSA8(ui.fam_last); RSA8(ui.menu_row); RSA8(ui.msg); RSA8(ui.msg2);
    for (i = 0; i < NE; i++) RS32(ui.enc_t[i]);
    RS32(ui_prefs); RSA8(favorites.factory); RS32(favorites.user);
    RS32(settings.palette); RS32(settings.lowcut); RS32(settings.zoom); RS8(settings_hold); RS8(settings_leds);
    RS8(mod_ui_slot); RS8(last_note); RS8(fop_op); RS8(fop_stg);
    RS8(cin_left); RS8(cin_total);
    RS8(usb.config); RS8(usb.suspended); RS8(usb.up); RS32(usb.setups); RS32(usb.sof_seen);
    RS8(scrn.st);
    RS32(scope_w);
    for (i = 0; i < SCOPE_N; i++) RS16(scope_buf[i]);
    {
        uint32_t buttons = panel_buttons();
        RS32(buttons);
        if (rs_read) fm1_in.buttons = buttons;
    }
    RS32(fm1_in.notes);
    RSA8(panel.btn); RSA8(panel.enc); RSA8(panel.dir);
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        for (j = 0; j < P_COUNT; j++) RS16(t->p[j]);
        RS8(t->engine); RS8(t->preset); RS8(t->eng_req); RS8(t->user);
        RS32(t->lfo_ph); RS32(t->lfo_val);
        RSA8(t->held); RS8(t->nheld); RS8(t->arp_phys); RS8(t->latched);
        RS16(t->seq_idx); RS32(t->seq_pos); RS32(t->peak); RS8(t->seq_active);
        RS8(lt[i].on);
        for (j = 0; j < NSTEP; j++) {
            step_t *s = rs_read ? &t->step[j] : (step_t *)&seq_steps(t)[j];
            RS8(s->n); RSA8(s->note); RS8(s->time); RS8(s->flags);
            RS8(s->vel); RS8(s->hit); RS8(s->acc); RS8(s->probability);
        }
        RSA8(fm6_patch[i]); RS8(fm6_slot[i]); RS32(fm6_pgen[i]);
    }
    RS8(motion.count); RS8(motion.on);
    for (i = 0; i < MOTION_MAX; i++) {
        RS8(motion.event[i].place); RS8(motion.event[i].param); RS8(motion.event[i].value);
    }
    RS8(chain_config.count);
    for (i = 0; i < CHAIN_ROWS; i++) { RSA8(chain_config.row[i].slot); RS8(chain_config.row[i].repeat); }
    RS8(chain.running); RS8(chain.row); RS8(chain.remaining); RS8(chain.clk);
    RSA8(chain.lane);
    RS8(nm.kind); RS8(nm.slot); RS8(nm.len); RS8(nm.cur); RS8(nm.num); RS8(nm.key);
    RS8(nm.tap); RS8(nm.rep); RS32(nm.t); RS32(nm.rep_t); RS8(nm.cat); RSA8(nm.s); RSA8(nm.ph);
    RS8(lys.l); RS8(lys.trk); RS8(lys.loaded); RS8(lys.oct); RS8(lys.cook); RS8(lys.fxk);
    RS32(lys.eqh); RS32(lys.solo); RS8(lys.dt_hint);
    for (i = 0; i < 9u; i++) RS16(lys.v[i]);
    RS8(proll.lo); RS8(proll.trk); RS8(proll.init); RS8(proll.moving); RS32(proll.frame);
    RS16(mt.pos); RS32(mt.frame);
    RSA8(perf_map); RS32(perf_held); RS32(perf_act); RS32(perf_latched); RS8(perf_latch_on);
    for (i = 0; i < 4u; i++) RS8(perf_k[i]);
    RS8(fx_latch);
    for (i = 0; i < 27u; i++) {
        RS8(kb_chn[i]); RS8(kb_trk[i]); RSA8(kb_chord[i]);
    }
    /* User preset records have a frozen 192-byte disk layout, not native runtime objects. */
    for (i = 0; i < UP_SLOTS / UP_PER_BANK; i++) {
        RS32(up_bank[i].magic); RS16(up_bank[i].rsize); RS16(up_bank[i].nslot);
        for (j = 0; j < UP_PER_BANK; j++) RSA8(up_bank[i].r[j]);
    }
    if (!rs_read) {
        rs_zone_valid = sample_wave.zone != 0;
        if (sample_wave.zone) rs_zone = *sample_wave.zone;
        else memset(&rs_zone, 0, sizeof rs_zone);
    }
    RS8(rs_zone_valid); RS32(rs_zone.n); RS32(rs_zone.ls); RS32(rs_zone.le); RS8(rs_zone.looped);
    for (i = 0; i < SAMPLE_WAVE_COLS; i++) { RS16(sample_wave.lo[i]); RS16(sample_wave.hi[i]); }
    RS32(sample_wave.key); RS32(sample_wave.gen); RS32(sample_wave.pos); RS32(sample_wave.peak); RS8(sample_wave.ready);
    if (rs_read) sample_wave.zone = rs_zone_valid ? &rs_zone : 0;
    RSA8(proj_name); RS8(proj_cur);
    for (i = 0; i < 4u; i++) {
        remote_project_t *p = &rs_projects[i];
        RS8(p->used); RSA8(p->name);
        for (k = 0; k < 4u; k++) { RS16(p->len[k]); RS32(p->density[k]); RS8(p->cells[k]); }
    }
    RS8(panel_screen.side);
    for(i=0;i<2u;i++){
        panel_view_t *v=&panel_views[i];
        RS8(v->home);RS8(v->page);RS8(v->track);RS8(v->bank);
        RS8(v->cursor);RS8(v->lane);RS8(v->seq_mode);RS8(v->grid);
    }
    if(rs_read)panel_views_ready=1;
    /* SCAPE's dashboard reads the real director, including on the second screen. */
    RS8(scape_music.active); RS8(scape_music.chord_pc); RS8(scape_music.suspended);
    for(i=0;i<NTRK;i++) RS32(scape_music.gain[i]);

}
#undef RS
#undef RS8
#undef RS16
#undef RS32
#undef RSA8
static uint32_t rs_capture(void)
{
    panel_views_capture();
    uint32_t i, j;
    for (i = 0; i < 4u; i++) {
        remote_project_t *p = &rs_projects[i];
        p->used = (uint8_t)project_info(i, p->name, p->len, p->density, p->cells);
    }
    rs_pos = rs_error = rs_read = 0;
    rs_walk();
    if (rs_error || rs_pos >= RS_MAX) return 0;
    rs_len = rs_pos;
    rs_rev = (rs_rev + 1u) & 16383u;
    if (!rs_rev) rs_rev = 1;
    memset(rs_dirty, 0, sizeof rs_dirty);
    for (i = 0; i < rs_len; i += RS_BLOCK)
        for (j = i; j < i + RS_BLOCK && j < rs_len; j++)
            if (!rs_base || rs_old[j] != rs_buf[j]) {
                rs_dirty[i / RS_BLOCK / 8u] |= 1u << (i / RS_BLOCK % 8u);
                break;
            }
    return rs_len;
}
static void rs_ack(void)
{
    memcpy(rs_old, rs_buf, rs_len);
    rs_base = rs_rev;
}
