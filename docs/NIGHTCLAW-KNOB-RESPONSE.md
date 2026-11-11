# NightClaw knob response

Dylan reported that all knobs felt jumpy and difficult to move in one turn. The shared continuous-parameter path had abrupt 1/3/5/8-unit speed bands after four quick detents, retained its quick-run state across direction changes, and treated batched input with the final multiplier. Signed controls also swallowed two clicks before leaving zero. SELECT level/energy editing used a separate fixed one-unit path.

The revised response starts new edits, reversals and destinations at one unit. Slow turns stay one unit per detent; continuous faster movement increases the gain one unit per detent to a maximum of four, or three for intermediate ranges. Slowing immediately reduces gain. Each queued detent is processed separately. Small discrete ranges stay literal. Track, voice, beat and scene selectors retain one selection per decoded detent. SELECT level/energy editing now shares the continuous-control response.

Centered controls still stop at zero when crossing it; the next detent moves immediately. Hardware quadrature decoding has not been changed: this patch addresses confirmed UI response problems, and physical feedback will determine whether another input issue remains.

Tests cover slow and fast movement, reversal, pauses, target changes, independent encoders, narrow ranges, batched movement, zero input and tick rollover. A full UI scenario checks a 0–100 sweep, one-unit reverse corrections, immediately leaving zero and unaccelerated voice/scene selection. Native image is 581,484 / 581,564 bytes; RAM is 95,644 / 98,304; pool is 334,472 / 344,064.

Dylan tested the installed revision and reported: “Feels much better.” This response is now the accepted hardware setting. Audio synthesis, saved-preset format and approved scene recipes are unchanged.

## Installation checkpoint

Source f92fe8c installed successfully on 2026-11-11; the installer completed the restart and NightClaw answered its status handshake with HIGH FRONTIER playing at 116 BPM. Package SHA-256: 4e381e114e91943739f8a4ef1adef920ccbc91a1d56a03f922a93c18d3cec168. Focused response unit tests, the full UI knob scenario and native resource checks pass. The broad local regression suite and full CI are still running at this checkpoint; Dylan subsequently confirmed that the knobs feel much better.

## Broad-check correction

The broader homesel scenario caught an unintended zero stop introduced when SELECT Energy was routed through the generic centered-control path. SELECT Energy now uses the same progressive response directly, preserving its existing uninterrupted sweep through zero. The unchanged homesel scenario and the new knob scenario both pass after the correction. The right-side control response Dylan accepted is unchanged.

The corrected source 653ff79 installed successfully and resumed HIGH FRONTIER at 116 BPM. Its package SHA-256 is 47ca4112f28425021c9f23bf9bb11558d240b7d71409d80e2a2abcfd865137ea. Native image: 581,516 / 581,564 bytes; RAM: 95,644 / 98,304; pool: 334,472 / 344,064.

## Host audit portability

The local orphan-control audit exposed a host-only header-order problem: suppressing __attribute__ before including unistd.h stripped libc inline attributes on this toolchain, emitting a strong read() symbol that recursed into itself. System headers now load before device-attribute suppression. The rebuilt simulator imports libc read instead of defining it. Native device code is unaffected.

## Completed local verification

All 38 regular control scenarios pass on the corrected simulator, including the unchanged Home Energy scenario and the new knob-response scenario. The separate exhaustive audit passes with 91 knobs checked. Production and legacy audio baselines, serial/concurrent/partial-failure worker comparisons, storage recovery, updater entry/loader and installer checks all pass. These results were collected in successive targeted runs: the initial Energy failure was corrected and rerun; the hung host audit was repaired and rerun. They are not a claim that the interrupted original aggregate run exited successfully. GitHub CI remains a separate pending check.
