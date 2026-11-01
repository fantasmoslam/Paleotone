/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Runtime opt-in timing. No allocations, printing or locks in audio/worker code.
 * Track timings are disabled while CPU1 is enabled: they deliberately avoid
 * cross-core counter writes. Aggregate phases still measure CPU0 elapsed time. */
#ifdef AUDIO_PROFILE_TARGET
static volatile uint32_t t5_nested_ticks;
typedef struct { uint32_t tick, nested, on; } ap_stamp_t;
typedef struct { uint32_t calls, sum_us, max_us; } ap_metric_t;
static volatile uint32_t ap_on;
static ap_metric_t ap_metric[10];
static inline ap_stamp_t ap_begin(void)
{
    ap_stamp_t s = {0, 0, 0};
    if (ap_on) { s.tick=fm1_ticks(); s.nested=t5_nested_ticks; s.on=1; }
    return s;
}
static inline ap_stamp_t ap_track_begin(void)
{
#if FELUCCA_CPU2
    if (C1_ON()) { ap_stamp_t s={0,0,0}; return s; }
#endif
    return ap_begin();
}
static inline void ap_end(uint32_t id, ap_stamp_t s)
{
    uint32_t raw, nested, us;
    if (!s.on) return;
    raw=fm1_ticks()-s.tick; nested=t5_nested_ticks-s.nested;
    us=(raw>=nested?raw-nested:0)/FM1_TICKS_PER_US;
    ap_metric[id].calls++; ap_metric[id].sum_us+=us;
    if(us>ap_metric[id].max_us)ap_metric[id].max_us=us;
}
#define AP_BEGIN() ap_begin()
#define AP_TRACK_BEGIN() ap_track_begin()
#define AP_END(id,s) ap_end(id,s)
#else
typedef uint32_t ap_stamp_t;
#define AP_BEGIN() 0u
#define AP_TRACK_BEGIN() 0u
#define AP_END(id,s) ((void)(s))
#endif
