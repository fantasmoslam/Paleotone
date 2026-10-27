# SAURUS coefficient cache: validation and hardware checkpoint

Candidate abe30e682e77caa9758f653fded04dd653393eb6 (draft PR #16), based on hardware-tested engine source 3da5576a9971191eef44d027f2cc1727203eb9f6. Decision: NOT ADOPTED. All software gates passed, but the hardware trial was slower in the loaded scene and used 720 extra bytes of RAM. The device was restored to source 3da5576a9971191eef44d027f2cc1727203eb9f6, renderer 2162762987.

## Implementation and correctness

Cache three coefficients plus exact clamped frequency/width keys for each of three resonators, in each of three throats, across four slots. A matching key skips only the pure saur_res calculation. Zero frequency is an invalid key because valid clamped frequencies are >=480. Existing whole-slot initialization clears the cache. Matching retriggers may reuse it; changes to either input force recalculation. The original local coefficient array remains in the sample loop to preserve its optimization opportunities. No changes to equations, rounding, sampling, phase/envelope/filter state updates, shared random draws/order, calls, buffers, voices, overload policy or core exclusions.

New differential test tests/saurus_cache_test.c compares mixed audio plus shared RNG evolution against forced recalculation before every block. All 14 call/lifecycle scenes passed: live call/size/mouth changes, same-key retriggers, wide key tracking, four-track slot pressure/stealing, release and slot reuse. Existing worker sound and concurrent tests also pass so far. No expected hashes or budgets were changed.

## Target / memory / companion

Target run 40439236057, job 121975274831, artifact 12705746485 passed image/updater/loader/unchanged static instruction budgets. SAURUS loop cost 1529 (budget 1757 plus existing 10% tolerance), versus 1407 previously. Static counts include the new branch/copy work but do not price cache-hit frequency or expand old coefficient computations: they are not runtime timing.

| Region | Baseline | Candidate |
| --- | ---: | ---: |
| Image bytes | 549088 | 549304 |
| Static RAM / 98304 | 96136 | 96856 |
| Free static RAM | 2168 | 1448 |
| Pool / 344064 | 332900 | 332900 |
| CPU1 user/exception stacks | 4096 + 256 | 4096 + 256 |

The cache consumes 720 bytes of previously free static working RAM. Processor stack sizes are not reduced. The separate pool headroom guard remains >=8192 bytes. For precision: tools/build.py SLOT_SPARE=15872 is the minimum unused application FLASH slot room, not the CPU1 stack; that guard is unchanged and passes. firmware/app.ld and hal/fm1_cpu1.S define the separate memory/stack allocations.

Package SHA256 6b5b02f9d570dffc6c9a44a3733a3e981e4734002cddd67199ab959f484cce1b, size 609905, expected renderer 1132235027. Companion run 40439294031, job 121975465714 passed; remote_views.json independently matches that renderer. Its isolated page is https://fantasmoslam.github.io/Paleotone/webapp/screen-saurus-cache/ (HTTP200 and exact source/renderer verified). Existing pages remain for rollback; site preservation includes the new directory.

## Fresh device baseline

Installed engine build 3da5576, same loaded 84 BPM scene, CPU1 OFF, serial COM6 and Felucca MIDI/audio. Actual playback asserted through all loaded phases. Thirty-second capture completed; final state stopped, profiling OFF.

| Phase | SAURUS mean, us/32 frames | Aggregate parts mean, us/32 frames |
| --- | ---: | ---: |
| idle | 4.07 | 28.97 |
| playing | 109.66 | 342.40 |
| usb | 109.52 | 343.13 |

Baseline USB capture produced 21 new packet underruns. These are not counts of distinct audible clicks. Track rows overlap aggregate parts. No candidate speedup is established yet.

## Recovery / next actions

Current Windows workspace root C:\Users\Dylan\Documents\Codex\2026-10-25\new-chat-9. Fresh baseline: work/paleotone-engine-hotpath/hardware/cache-baseline-1793068624700. Candidate package, companion, guarded scripts and future results: work/paleotone-saurus-cache/hardware/trial-1793068605444. Scripts were syntax checked before the completed trial; final results and rollback are below. Retain previous engine package SHA0899f068b59cf46783e0a0fe96b6636aa82abe37de94f65114213df1568ec607 for rollback.

After full exact-source host/race gates pass: fresh backup, identity-pinned install, verified playback profiling and at least 30-second USB capture; compare all 13 objects by size/CRC/data; leave playback stopped and profiling OFF, core state explicit. Decide whether measured headroom justifies the additional RAM; caching remains a draft until those results. Keep raw backups private.

## Completed hardware trial: not adopted, rollback verified

All exact-source software gates passed before installation, including host run 40439236130 / job 121975274766 (14 differential lifecycle scenes, 213 unchanged sound goldens, 61 concurrent scenes, ThreadSanitizer and the full host suite). Runtime instruction counters were unavailable in the host regression output; the static target budgets passed, but zero host counts are not a measured runtime CPU-budget pass.

Installed cache renderer 1132235027 at 2026-10-27T02:52:40.273Z after a fresh consistent backup. Same loaded scene at 84 BPM, core OFF, with actual playing=1 asserted throughout both loaded phases and a 30-second USB capture.

| Playing phase | Baseline mean, us/32 frames | Cache mean, us/32 frames |
| --- | ---: | ---: |
| track1 | 103.93 | 103.84 |
| track2 | 109.66 | 113.48 |
| track3 | 71.22 | 71.22 |
| track4 | 49.72 | 49.72 |
| parts | 342.40 | 346.16 |
| effects | 95.44 | 95.45 |
| master | 74.47 | 73.94 |

SAURUS became about 3.5% slower (109.66 -> 113.48 us) in the loaded profiling window; during the longer USB window it likewise changed 109.52 -> 113.25 us. Aggregate parts changed 342.40 -> 346.16 us, about 1.1% slower. Other track/effects means were close, supporting the decision not to keep this tradeoff. These are live windows, not musically phase-aligned benchmarks; they do not isolate the cause of the loss. Cache lookup/copy overhead and the live input-change/hit pattern remain hypotheses, not measured explanations. Held-note reuse rates did not establish a benefit for the live scene.

Thirty-second USB capture: baseline 21 new packet underruns, cache 20. Both hit raw ring fill 0; maximum publication gaps 3934 / 3960 us. Both shed 24 voices across the surrounding roughly 34.6-second intervals; new late blocks 8 / 6. Candidate render/wall maxima reached 2869 / 3101 us against deadline 2902 us. The baseline maxima were cumulative across a longer lifetime and are not comparable peak-speedup evidence. No reliable deadline or USB fix is established. LCD timeouts stayed zero.

All 13 objects matched the fresh pre-install backup after the trial in size, CRC and data. Because the scene gained no processing headroom while static RAM use grew by 720 bytes, the candidate was not retained.

A guarded rollback restored the previous engine package (SHA256 0899f068b59cf46783e0a0fe96b6636aa82abe37de94f65114213df1568ec607). Identity verified at 2026-10-27T02:57:36.650Z: source 3da5576a9971191eef44d027f2cc1727203eb9f6, renderer 2162762987, schema 2, 82 globals. A fresh post-rollback backup again matches all 13 original objects by size, CRC and encoded data. Fresh final read: stopped, profiling OFF, second core OFF, idle CPU 30%. The 720 bytes are no longer allocated by the installed build.

Rollback package, guarded script, installation identity, final backup/comparison and final-status.txt: work/paleotone-saurus-cache/hardware/rollback-1793069383317. Trial files remain in the earlier trial directory. Keep all raw backups private.

Current matching display is https://fantasmoslam.github.io/Paleotone/webapp/screen-engine-hotpath/. The cache-specific page is only for reproducing the rejected experiment. Draft PR #16 is closed without merging; its branch and results remain for reference. Next useful hardware comparison: enable the second core on the restored build, which requires a restart, then repeat the same workload. No second-core setting was changed in this session.
