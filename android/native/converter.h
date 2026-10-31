/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <math.h>
#include <string.h>
/* Sample-rate converter from the engine's 44.1 kHz to the device rate:
 * polyphase windowed sinc. The rate ratio is reduced to up/down (44.1 -> 48 kHz
 * is 160/147) and each output frame is a CONV_TAPS FIR at its exact fractional
 * position. Flat to about 20 kHz, images above 24 kHz about 70 dB down, 24 input
 * frames (0.54 ms) of delay. A ratio needing more than CONV_PHASES phases falls
 * back to linear interpolation. No allocation; conv_run is audio-thread safe. */
#define CONV_TAPS 48
#define CONV_PHASES 1024
#define CONV_PI 3.14159265358979323846
typedef struct {
    float coef[CONV_PHASES * CONV_TAPS];
    float l[2 * CONV_TAPS], r[2 * CONV_TAPS];  /* each frame twice: one contiguous window */
    unsigned rate, up, down, phase, pos, linear, primed;
    float a[2], b[2];
    double frac;
} pt_converter;
static inline void conv_reset(pt_converter *c) {
    memset(c->l, 0, sizeof c->l); memset(c->r, 0, sizeof c->r);
    c->pos = c->phase = c->primed = 0; c->frac = 0;
}
static inline double conv_i0(double x) {
    double sum = 1, term = 1;
    for (unsigned k = 1; k < 64 && term > 1e-12 * sum; ++k) { term *= x * x / (4.0 * k * k); sum += term; }
    return sum;
}
/* Lifecycle thread only: computes up to CONV_PHASES * CONV_TAPS taps. */
static inline void conv_prepare(pt_converter *c, unsigned in, unsigned rate) {
    unsigned x = in, y = rate;
    while (y) { unsigned t = x % y; x = y; y = t; }
    c->rate = rate; c->up = rate / x; c->down = in / x;
    c->linear = c->up > CONV_PHASES;
    if (!c->linear) {
        /* Cutoff at the lower Nyquist, in cycles per input frame; Kaiser beta 7. */
        const double cutoff = rate < in ? 0.5 * rate / in : 0.5, beta = 7.0, half = CONV_TAPS / 2;
        for (unsigned p = 0; p < c->up; ++p) {
            float *h = &c->coef[p * CONV_TAPS];
            double sum = 0;
            for (unsigned k = 0; k < CONV_TAPS; ++k) {
                /* Taps run oldest to newest; the output sits half the taps back. */
                double t = (double)(CONV_TAPS - 1 - k) - half + (double)p / c->up;
                double r = t / half, v = 2 * cutoff * t;
                double sinc = fabs(v) < 1e-12 ? 1 : sin(CONV_PI * v) / (CONV_PI * v);
                double window = fabs(r) < 1 ? conv_i0(beta * sqrt(1 - r * r)) / conv_i0(beta) : 0;
                h[k] = (float)(sinc * window); sum += h[k];
            }
            for (unsigned k = 0; k < CONV_TAPS; ++k) h[k] = (float)(h[k] / sum);   /* unity gain at DC */
        }
    }
    conv_reset(c);
}
static inline void conv_run(pt_converter *c, float *out, unsigned frames, void (*next)(float *)) {
    if (c->linear) {
        if (!c->primed) { next(c->a); next(c->b); c->primed = 1; }
        const double step = (double)c->down / c->up;
        for (unsigned i = 0; i < frames; ++i) {
            out[i*2] = c->a[0] + (c->b[0]-c->a[0]) * (float)c->frac;
            out[i*2+1] = c->a[1] + (c->b[1]-c->a[1]) * (float)c->frac;
            c->frac += step;
            while (c->frac >= 1.0) { c->a[0]=c->b[0]; c->a[1]=c->b[1]; next(c->b); c->frac-=1.0; }
        }
        return;
    }
    for (unsigned i = 0; i < frames; ++i) {
        while (c->phase >= c->up) {
            float v[2];
            next(v); c->phase -= c->up;
            c->l[c->pos] = c->l[c->pos + CONV_TAPS] = v[0];
            c->r[c->pos] = c->r[c->pos + CONV_TAPS] = v[1];
            if (++c->pos == CONV_TAPS) c->pos = 0;
        }
        const float *h = &c->coef[c->phase * CONV_TAPS], *l = c->l + c->pos, *r = c->r + c->pos;
        float l0 = 0, l1 = 0, r0 = 0, r1 = 0;
        for (unsigned k = 0; k < CONV_TAPS; k += 2) {
            l0 += h[k] * l[k]; l1 += h[k+1] * l[k+1];
            r0 += h[k] * r[k]; r1 += h[k+1] * r[k+1];
        }
        out[i*2] = l0 + l1; out[i*2+1] = r0 + r1;
        c->phase += c->down;
    }
}
