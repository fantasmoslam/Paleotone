/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>

typedef struct {
    uint32_t time;
    int16_t target;
    int8_t gain;
} knob_response_t;

/* Slow edits and a new direction/target start at one unit. Faster turns ramp
 * by at most one unit per detent, capped at four (three for narrower ranges).
 * Apply each queued detent separately: a delayed UI frame must not multiply
 * the entire batch by its final gain. No synthetic movement after release. */
static int32_t knob_response(knob_response_t *r, uint32_t now, int target,
                             int range, int32_t steps)
{
    uint32_t dt = now - r->time, count, i;
    int dir, gain, fresh, wanted, total = 0;
    if (!steps) return 0;
    dir = steps > 0 ? 1 : -1;
    count = (uint32_t)(steps > 0 ? steps : -steps);
    fresh = !r->gain || r->target != target || dt >= 180u ||
            (r->gain > 0) != (dir > 0);
    gain = fresh ? 1 : (r->gain > 0 ? r->gain : -r->gain);
    r->time = now;
    r->target = (int16_t)target;
    if (range <= 24) { r->gain = 0; return steps; }
    dt /= count;
    wanted = dt >= 120u ? 1 : dt >= 70u ? 2 : dt >= 35u ? 3 : 4;
    if (range < 100 && wanted > 3) wanted = 3;
    for (i = 0; i < count; i++) {
        if (!(fresh && i == 0)) {
            if (gain > wanted) gain = wanted;
            else if (gain < wanted) gain++;
        }
        total += gain;
    }
    r->gain = (int8_t)(dir * gain);
    return dir * total;
}
