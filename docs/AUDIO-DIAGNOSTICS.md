# Opt-in audio diagnostics candidate

Based on installed source 21404aa. Not installed or hardware validated.

The current full mix remains overloaded; do not raise CPU limits or voice budgets to hide it. The track mute comparison is an intervention test, not exact per-engine timing. Five-second samples gave CPU snapshots 78% baseline; 75% with T1 muted; 70% T2 muted; 76% T3 muted; 78% T4 muted; 37% with all muted; 85% on restored baseline. All-muted and T4-muted windows had zero new shed voices. Musical phase/tails/evolution make these approximate. Original mixer levels/mutes were restored and transport verified stopped.

## New telemetry
status: audio_wall_max_us (includes nested interrupts), audio_nested_max_us, audio_deadline_us, lcd_timeouts, uac_raw_fill_lo (actual queued frames before a packet), uac_feed_gap_max_us (largest time between 44.1 kHz producer publications while streaming). Maxima are cumulative since boot or USB re-prime as applicable. Historical maxima are not synchronous samples or additive.

profile on: clear the phase counters and enable timing; profile off: disable it; profile: report counters. Each row contains calls, accumulated whole microseconds, maximum microseconds per call. Runtime profiling starts OFF. Rows: track1..4, events/director, aggregate parts/performance setup/worker join, ambient+SCAPE, effects/performance preprocessing, master/scope, USB copy. Individual tracks overlap the aggregate parts row and must not be added to it. Track rows are deliberately disabled whenever CPU1 is enabled to avoid concurrent writes or incorrect subtraction of CPU0 interrupt time from worker timings.

The timer removes nested TIMER5 time from the phase measurements. Short sections lose sub-microsecond precision. Added timing instrumentation itself has a cost: compare profiling OFF and ON on this same build. Total wall/nested maxima and USB feed-gap sampling are always present. Profile commands change diagnostic counters only; no song, flash settings, clock, buffer target or overload threshold is changed.

USB feed-gap telemetry currently measures only the 44.1 kHz path used by this firmware. Not evidence for 48 kHz. Per-track native cycle/stall/cache measurements, LCD wait durations and coherent interval snapshots remain later work.

## Acceptance before installation
Target firmware/image/loader/CPU-budget checks, profiling counter tests (including timer wrap and nested-time subtraction), full host/race/fidelity checks, matching renderer companion. Before flash preserve latest project/settings backup and rollback, verify exact package and source. Do not label a queued workflow as a pass.

## Device experiment
Start with CPU1 OFF, same saved scene and stopped transport. Record baseline, start playback, profile OFF then ON for controlled intervals, repeat with USB capture and static/active display. Recheck worker ON using aggregate phases only. Watch late/shed/underrun deltas and original project state. Measure before changing buffers or DSP.
