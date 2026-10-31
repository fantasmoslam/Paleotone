/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* A second, drawing-only copy of the firmware. Owned by the UI thread: it never
 * makes sound and shares no state with the engine. */
void pt_display_boot(void);
/* Import a pt_snapshot and draw it. Returns 0 if the snapshot was rejected. */
int pt_display_apply(const uint8_t *snapshot, uint32_t size);
void pt_display_screen(uint32_t *argb);
void pt_display_control_read(unsigned slot, int *value, int *min, int *max, char *text);
unsigned pt_display_playing(void);
unsigned pt_display_scape_busy(void);
