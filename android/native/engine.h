/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include <stddef.h>
void pt_boot(const uint8_t *saved, size_t size);
void pt_render(float *out, unsigned frames, unsigned rate);
void pt_keys(uint32_t mask);
void pt_buttons(uint32_t mask);
void pt_encoder(unsigned role, int steps);
void pt_master(unsigned value);
void pt_midi(unsigned status, unsigned d1, unsigned d2);
void pt_panic(void);
void pt_screen(uint32_t *argb);
size_t pt_save(uint8_t *out, size_t cap);
unsigned pt_playing(void);
unsigned pt_control_count(void);
const char *pt_control_label(unsigned slot);
void pt_control_read(unsigned slot,int *value,int *min,int *max,char *text);
void pt_control_set(unsigned slot,int value);
