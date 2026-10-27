# USB audio queue margin hardware experiment

Candidate source 0765942f34a1b2818b5f9af2ce1d8242c32bf5ca, renderer 696410948, package SHA256 a060ee37200645a96e2750541831ad5b0062768f850abebb806cb4d677cd8770, size 609905. Based on engine build 3da5576a9971191eef44d027f2cc1727203eb9f6. Core ON throughout the comparison; loaded scene unchanged at 84 BPM (STAMPEDE, BRACHIO CALL, MAMMOTH, DEEP CHANT).

## Change and latency tradeoff

Only USB prime/fill targets change. At 44.1 kHz: prime 104 -> 232, low 80 -> 208, high 128 -> 256 frames. This adds a target 128/44100 seconds = 2.9025 ms of queue wait. At 48 kHz: prime 113 -> 253, low 87 -> 227, high 139 -> 279 frames (approximately the same added time).

The ring remains 512 frames. Image 549088 bytes, static RAM 96136/98304 and pool 332900/344064 are unchanged. No new allocation, synthesis equation, I2S buffer, shared voice limit, worker exclusion or clock-control algorithm changes. This spends USB latency; it cannot fix synthesis that misses its I2S deadline.

The host queue test uses the real ring/packet code and records per-frame publication-to-packet time for the old and new source under identical delivery schedules. A deterministic 4 ms producer pause tests the margin; it is a controlled model, not a replay of device interrupt timings. This measurement excludes host driver/DAW buffers and cannot establish total key-to-DAW latency.

## Baseline

Two worker-ON runs of the previous build each captured 30 seconds and produced 6 new USB packet underruns and 2 late blocks over the surrounding ~34.6-second interval. Both shed 24 voices in that interval. Aggregate instrument rendering during USB capture averaged 311.15 / 310.98 us per 32 frames. Publication gaps reached 3756 / 3767 us, raw queue fill reached zero. These are live windows, not phase-aligned musical benchmarks.

## Validation

Target run 40469158720 / job 122071768668 / artifact 12717558956 passed target budgets, updater/loader tests and memory guards. Companion run 40469158695 / job 122071768829 / artifact 12717567839 passed, and live metadata verifies source/renderer at https://fantasmoslam.github.io/Paleotone/webapp/screen-usb-buffer/ . Main site preservation includes screen-usb-buffer; public installer firmware was not changed.

Host run 40469158933 / job 122071769317: queue timing/jitter, concurrent mixer, worker-active sound regression and ThreadSanitizer passed. Full existing single-core suite passed; ALL HOST TESTS PASSED at 08:05:56 UTC. Runtime CPU instruction counters in host output were unavailable/zero and are not measured runtime budget evidence.

Private raw evidence: work/paleotone-engine-hotpath/hardware/dualcore-compare-1793086667843 and hardware/usb-baseline-1793087248901 (baseline). Candidate scripts/artifacts/checkpoint: work/paleotone-usb-buffer/hardware/trial-1793087288494; repeat-1793087556398 reserved for repeat. A guarded previous-engine rollback package is ready in hardware/rollback-ready. Do not commit raw backups.

Draft PR https://github.com/fantasmoslam/Paleotone/pull/17 targets feature/engine-hotpath-inline. No merge or public release is implied.

## Completed device results

Installation identity verified 2026-10-27T08:08:44.681Z. Actual playback asserted throughout every loaded phase.

| Capture | Baseline underruns | Candidate underruns | Late blocks baseline / candidate | Voices shed baseline / candidate |
| --- | ---: | ---: | ---: | ---: |
| 30 seconds, profiling ON | 6 (both baseline runs) | 0 | 2 / 0 | 24 / 24 |
| 60 seconds, profiling OFF | 13 | 0 | 4 / 4 | 45 / 45 |

Counters for late blocks/shedding cover the surrounding approximately 34.6 / 64.6-second intervals. Captures completed successfully at 44.1 kHz stereo. Candidate USB overruns, missed packets and stalls stayed zero. During the longer stream raw minimum fill was 104 frames and publication gap maximum 3747 us. Worker faults/fallbacks zero; max wait 18 us, stack use 784 bytes. Audio wall maximum still reached 3002 us against the 2902 us deadline. Voice shedding and late audio blocks are therefore unresolved. No general USB stability claim follows from these short runs. Hardware 48 kHz capture remains untested; its existing host regression passed.

Host measured publication-to-packet queue latency, healthy schedule: baseline mean 2.7752 ms (1.88..3.66), candidate mean 5.6765 ms (4.78..6.57), increase 2.9013 ms. With deterministic 4 ms producer pauses baseline had 9 underrun packets / 287 repeated samples, candidate zero. This is modeled queue delay, not physical key-to-DAW latency.

All 13 post-trial objects match the fresh pre-install backup exactly in size, CRC and encoded data, including settings. Playback stopped, profiling OFF, second core ON. Candidate remains installed for further evaluation. Unprofiled evidence: C:\Users\Dylan\Documents\Codex\2026-10-25\new-chat-9\work\paleotone-engine-hotpath\hardware\usb-unprofiled-baseline-1793087933337 and C:\Users\Dylan\Documents\Codex\2026-10-25\new-chat-9\work\paleotone-usb-buffer\hardware\unprofiled-1793087951329. Comparison, saved-state proof and validation manifest are in the candidate trial directory. The reserved repeat directory was not used; the longer unprofiled run provided the second hardware observation.

Decision: retain as an experimental build because the USB margin improved without added RAM or synthesis work. Keep PR #17 draft. Next work should address synthesis deadline/overload pressure and longer MIDI/UI/recording interaction soak tests before public release.

## Additional MIDI readback stress

A further 60-second unprofiled USB capture ran alongside screen-context and track-dump request pairs every 100 ms (one pair in flight at a time). 483 pairs completed, seven pairs encountered an exception, maximum successful pair latency 64 ms. The harness counted exceptions but did not preserve their individual messages, so the cause is unresolved; this is not a clean MIDI reliability pass and does not test MIDI note/clock input or physical UI interaction.

USB still had zero new underruns, overruns, missed packets or stalls. The surrounding ~64.7 seconds had six late blocks and 45 voices shed; worker faults/fallbacks zero. Raw ring minimum 103 frames, max publication gap 3638 us. Final stopped, profiler OFF, worker ON. Evidence: hardware/midi-usb-stress-1793088985441. Follow up with request-level error logging and separate MIDI request types before drawing conclusions about MIDI stability.

Fresh post-stress backup again matches all 13 pre-install objects exactly by size, CRC and encoded data.
