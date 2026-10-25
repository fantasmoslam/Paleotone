/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Main-loop ownership of one companion request. The shared USB receive frame
 * is released immediately; OTA must never be able to consume a deferred
 * editor request. No audio ISR, heap allocation, spin or USB wait is involved. */
#define EDRQ_MAX 8u
typedef struct { uint8_t frame[EDRQ_MAX], len; uint32_t resets; } ed_remote_queue_t;
static int edrq_offer(ed_remote_queue_t *q, const uint8_t *p, uint32_t n, uint32_t resets)
{
    if (q->len || n < 4u || n > EDRQ_MAX ||
        p[0] != 0x7Du || p[1] != 0x46u || p[2] != 0x4Cu ||
        p[3] < 92u || p[3] > 94u) return 0;
    for (uint32_t i = 0; i < n; i++) q->frame[i] = p[i];
    q->resets = resets;
    q->len = (uint8_t)n;
    return 1;
}
/* 1 ready, 0 keep waiting, -1 discarded at disconnect/reset. */
static int edrq_ready(ed_remote_queue_t *q, uint32_t used, uint32_t resets, uint32_t connected)
{
    if (!q->len) return 0;
    if (!connected || q->resets != resets) { q->len = 0; return -1; }
    return used <= 4u;
}
