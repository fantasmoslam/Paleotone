/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Physical focus events only. No song, parameter, transport or flash writes. */
typedef struct {
    uint32_t suppress, last_ms, resets;
    uint16_t serial;
    uint8_t side, enabled;
} panel_screen_gesture_t;

static uint32_t panel_screen_gesture(panel_screen_gesture_t *s, uint32_t buttons,
                                    uint32_t pressed, uint32_t home, uint32_t down,
                                    uint32_t up, int enabled, int blocked)
{
    uint32_t mask = home | down | up;
    if (s->suppress) {
        /* Keep swallowing until ALL chord buttons are released, even after USB loss. */
        if (!(buttons & s->suppress)) s->suppress = 0;
        return s->suppress;
    }
    if (!enabled || blocked || (buttons & ~mask) || !(buttons & home)) return 0;
    if ((buttons & (down | up)) != down && (buttons & (down | up)) != up) return 0;
    if (!(pressed & (down | up))) return 0; /* HOME first, then a fresh octave press */
    s->side = (uint8_t)((buttons & up) != 0);
    s->serial = (uint16_t)((s->serial + 1u) & 16383u);
    s->suppress = mask;
    return mask;
}
