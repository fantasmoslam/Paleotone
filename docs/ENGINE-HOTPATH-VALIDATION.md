# ROAR / SAURUS candidate validation

Candidate source: 3da5576a9971191eef44d027f2cc1727203eb9f6.
Draft PR: https://github.com/fantasmoslam/Paleotone/pull/15
Base: ba586018234cd8be5fbce09a3c3c7306c4c1389f, the last verified effects build.
All exact-source software gates passed, including CI host run 40434234930, job 121958970194 (ALL HOST TESTS PASSED). This includes the concurrent mixer, worker sound, ThreadSanitizer and complete single-core functional/regression suite. No hardware installation or hardware speedup measurement occurred in this cloud session.

## Changes

Force inline ROAR's two per-sample body filter calls and explicitly use the existing inline clipping implementation in SAURUS's per-throat sample loop. Equations, clipping interpolation, rounding, state order, random draws/order, clocks, buffers, voice limits, overload thresholds, second-core exclusions and reserve are unchanged.

## Target and companion

- Target run 40434235081, job 121958970575, artifact 12703650271: passed image, updater, loader and unchanged instruction budgets.
- Assembly confirms no gurdy_bp calls in gurdy_render and no direct softclip call in saurus_render. The shared soft-knee and voice-amplitude helpers remain outlined.
- ROAR static loop cost 323 (budget 548); SAURUS 1407 (budget 1757), with the existing 10% tolerance unchanged. Static counts are not cycle measurements and do not expand callees.
- Image 549088 bytes (+528); static RAM 96136/98304 and pool 332900/344064 unchanged.
- Package 609905 bytes, SHA256 0899f068b59cf46783e0a0fe96b6636aa82abe37de94f65114213df1568ec607.
- Expected renderer 2162762987, independently verified in the companion's remote_views.json.
- Pinned companion run 40434327653, job 121959268783: passed. Companion artifact 12703238204 is staged alongside the target package.

No expected sound results or CPU/target budgets were changed.

## Local sound and concurrency

213 golden renders unchanged, no health, voice/routing failures or crashes. Dedicated ROAR behavior checks pass, including the existing 256 EDIT corners across three pitches and crank/pressure/ringing behavior. All 61 concurrent mixer scenes match between one and two cores across tested splits.

The normal host fixture defines __attribute__ away. A supplementary generated fixture removed only that erasure and compiled the same regression and ROAR tests with -Os. With real compiler attributes active, all 213 golden renders remained unchanged and the dedicated ROAR checks passed. This fixture was generated under build/attribute-check; no firmware or tracked test source was modified for the audit.

Runtime instruction counts were unavailable in the local and CI sound-regression output (zero counts / zero budget notes). Those are unavailable data, not evidence that CPU costs are zero or that a numerical runtime CPU budget was measured successfully. The unchanged target static instruction-budget gates did pass. No target timing or runtime speedup is established.

## Hardware gate and recovery

This session moved to a Linux cloud workspace and cannot access the Windows computer's FM-1, COM6, WinMM MIDI or USB audio ports. The previous Windows hardware session remains documented in EFFECTS-HOTPATH-HARDWARE.md. Do not describe software validation as an on-device performance improvement.

The last verified device renderer is 696079837; its matching page is https://fantasmoslam.github.io/Paleotone/webapp/screen-effects-hotpath/. Do not use the candidate renderer's companion with that older build.

After restoring computer access: verify the actual device identity/state, capture a fresh consistent backup, retain the old package, stage exact-source identity/hash guards and matching companion, then measure the same loaded 84 BPM scene with second core initially OFF. Assert actual playback, compare track/aggregate/effects/master phases, deadline interval deltas and overload shedding, and capture USB for at least 30 seconds. Compare all 13 saved objects afterward and leave playback stopped/profiling OFF. See candidate docs/ENGINE-HOTPATH.md for the checklist.

Local cloud checkout: /workspace/scratch/3bb4285be32b/paleotone.
Staged artifacts and validation.json: build/engine-candidate/.
No public installer firmware or current device display was replaced.

## Separate research: resonator coefficient reuse

Temporary instrumentation counted consecutive identical clamped frequency/width inputs per slot/layer/resonator and cleared validity at new slot initialization. These were single held-note offline renders, eight seconds long with a six-second hold, not the user's live scene:

| Render | Calculations | Consecutive equal inputs |
| --- | ---: | ---: |
| BRACHIO CALL | 59862 | 58494 |
| TYRANT ROAR | 81855 | 79920 |
| BELLOW, high WILD/long LEN | 50772 | 50418 |

This suggests a future cache experiment, not a measured performance gain. The holding pattern may overstate reuse compared with frequently retriggered notes. A straightforward full-width input/three-coefficient cache would require about 720 bytes across four slots, three layers and three resonators, before any additional layout/validity overhead. Memory layout, invalidation, parameter changes, retriggers/stealing and exact sound fidelity need validation. No coefficient cache or extra RAM allocation was implemented in this candidate.
