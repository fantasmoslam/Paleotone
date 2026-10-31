/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include <stddef.h>
void pt_boot(const uint8_t *saved, size_t size);
/* Precompute the converter for a device rate. Lifecycle thread, before start. */
void pt_prepare(unsigned rate);
void pt_render(float *out, unsigned frames, unsigned rate);
void pt_keys(uint32_t mask);
void pt_buttons(uint32_t mask);
void pt_encoder(unsigned role, int steps);
void pt_master(unsigned value);
void pt_midi(unsigned status, unsigned d1, unsigned d2);
void pt_panic(void);
size_t pt_save(uint8_t *out, size_t cap);
unsigned pt_playing(void);
unsigned pt_control_count(void);
const char *pt_control_label(unsigned slot);
void pt_control_read(unsigned slot,int *value,int *min,int *max,char *text);
void pt_control_set(unsigned slot,int value);
enum { PT_ENV_START=1,PT_ENV_STOP,PT_DRONE_START,PT_DRONE_STOP,
       PT_MUSIC_START,PT_MUSIC_STOP,PT_SCAPE_STOP_ALL,PT_SCAPE_GENERATE,PT_SCAPE_NEXT };
void pt_scape(unsigned command);
unsigned pt_scape_busy(void);
/* Display projection for display.c: PT_SNAPSHOT_MAX bytes, 0 if it failed. */
#define PT_SNAPSHOT_MAX (2u + 16384u)
uint32_t pt_snapshot(uint8_t *out, uint32_t cap);
unsigned pt_ui_frames(void);
