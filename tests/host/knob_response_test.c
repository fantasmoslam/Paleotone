/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../../firmware/src/app/knob_response.h"
int main(void)
{
    knob_response_t r = {0}, other = {0}, batch = {0};
    uint32_t t = 1000;
    int i, sweep = 0;
    /* A relaxed edit stays precise, indefinitely. */
    for (i = 0; i < 20; i++) {
        t += 150;
        assert(knob_response(&r, t, 1, 100, 1) == 1);
    }
    /* Ordinary and fast sweeps grow progressively, never eightfold. */
    r.gain = 0;
    assert(knob_response(&r, t += 40, 1, 100, 1) == 1);
    assert(knob_response(&r, t += 40, 1, 100, 1) == 2);
    assert(knob_response(&r, t += 40, 1, 100, 1) == 3);
    assert(knob_response(&r, t += 40, 1, 100, 1) == 3);
    assert(knob_response(&r, t += 20, 1, 100, 1) == 4);
    assert(knob_response(&r, t += 20, 1, 100, 1) == 4);
    /* Reversing, slowing, pausing or changing the destination gives fine control. */
    assert(knob_response(&r, t += 20, 1, 100, -1) == -1);
    assert(knob_response(&r, t += 20, 1, 100, -1) == -2);
    assert(knob_response(&r, t += 150, 1, 100, -1) == -1);
    assert(knob_response(&r, t += 200, 1, 100, -1) == -1);
    assert(knob_response(&r, t += 20, 2, 100, -1) == -1);
    assert(knob_response(&other, t, 2, 100, 1) == 1);
    /* Discrete ranges remain literal; medium ranges cap at three. */
    for (i = 0; i < 10; i++) assert(knob_response(&r, t += 1, 3, 24, 1) == 1);
    for (i = 0; i < 10; i++) assert(knob_response(&r, t += 1, 4, 48, 1) <= 3);
    /* A fresh queued batch ramps 1+2+3+4, instead of multiplying all by four. */
    assert(knob_response(&batch, 40, 1, 100, 4) == 10);
    /* The same four ongoing detents give the same result separately or batched. */
    r = (knob_response_t){0}; batch = r;
    assert(knob_response(&r, 1000, 1, 100, 1) == 1);
    assert(knob_response(&batch, 1000, 1, 100, 1) == 1);
    for (i = 1; i <= 4; i++) sweep += knob_response(&r, 1000 + 20*i, 1, 100, 1);
    assert(knob_response(&batch, 1080, 1, 100, 4) == sweep);
    /* One brisk 30-detent sweep spans a normalized 0..100 control. */
    r = (knob_response_t){0}; sweep = 0;
    for (i = 0; i < 30; i++) sweep += knob_response(&r, 1000 + 30*i, 1, 100, 1);
    assert(sweep >= 100 && sweep <= 120);
    /* Tick wrap and zero input do not generate movement. */
    r = (knob_response_t){.time = UINT32_MAX - 10, .target = 1, .gain = 4};
    assert(knob_response(&r, 9, 1, 100, 1) == 4);
    assert(knob_response(&r, 10, 1, 100, 0) == 0);
    puts("knob response: slow/fast/reversal/pause/target/batch/range/wrap passed");
    return 0;
}
