# PALEOTONE dual-core candidate

This branch investigates headroom for an independent SCAPE engine. It does not
implement the SCAPE redesign, increase the twelve-voice budget, or claim a
hardware performance gain before measurement on Dylan's FM-1.

The worker is ported from Felucca v1.6.1 (GPL-3.0-only), including X0X,
Melodee, Jangada and gospodindilly-ops' contributions. CPU clock and voltage,
the flash reserve, project payloads, global parameter IDs and remote snapshot
focus trailer are unchanged.

## Operation

The compiled worker is OFF for existing settings: MENU > SYSTEM > 2ND CORE
uses a positive opt-in bit in previously unused settings byte [15][24], bit 7.
Changing it takes effect after a restart. The editor setting is appended as
ID 31; older setting IDs and descriptor indexes retain their meanings.

CPU0 owns MIDI, note allocation, sequencing, recording, SCAPE control/audio,
the effects and master mix. CPU1 renders selected instrument parts into its
own dry/send buffers. Parts use separate modulation and SPREAD scratch state;
FM6 temporary buses are on the rendering core's stack. LFO random draws are
completed in track order after the concurrent jobs join.

VOICE (engine 5) and SAURUS remain on CPU0 to preserve shared-state ownership.
The later [ROAR worker audit](ROAR-WORKER-AUDIT.md) makes ROAR eligible under
the joined-render ownership contract; this does not establish hardware speedup.
A PHYS part disables the split for that block: PHYS can draw from
the same RNG as the LFOs when changing models. This conservative policy is
applied even to forced test masks. It can be relaxed only after a further
state audit and fidelity/race tests. The split uses previous-block cost
estimates, so the first block can run on CPU0.

A job declined before execution is rendered locally with unchanged behavior.
A bad mailbox check, worker fault or timeout retires CPU1 and uses CPU0.
A partially executed failed job may cause one audible discontinuity: fallback
does not promise bit-exact recovery of the failing block. CPU0 clears the
opt-in setting and queues its persistence while transport is stopped. Failed
startup or a failed early boot also disables the worker for the session.

The worker is stopped before RAM initialization and all reset/update entries.
The idle loop lives in RAM; per-job check words, stack guards and the upstream
safe-hold bus-trap sequence are retained. The fallback does not guarantee a
full dual-core workload can run without reduced polyphony on one core.

## Validation and bench procedure

The branch runs concurrent host audio comparisons, existing golden/health
regressions with the worker active, ThreadSanitizer checks, the full existing
single-core suite, and the target build/image budget gates. No golden hashes,
reserve requirements or CPU limits should be weakened to pass this port.

Before a hardware trial, retain the installed rollback package and projects,
verify all exact-source gates and matching companion renderer, then stage a
new backed-up installation session. Compare the same demanding songs with
2ND CORE OFF and ON: audio max/late counts, USB audio, expanded display, live
recording, SCAPE, saves, long runs and reset/update entry. A browser render is
not evidence that the hardware worker is stable.

The serial console's status exposes cpu2_set, cpu2, cpu2_blocks, cpu2_bad,
cpu2_gave_up, cpu2_wait_max_us and cpu2_stack. A default OFF build has not
tested the second core merely by booting. Do not label this candidate as
installed, hardware validated, or ready for promotion until that is verified.
