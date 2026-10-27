# SAURUS coefficient cache: validation and hardware checkpoint

Candidate abe30e682e77caa9758f653fded04dd653393eb6 (draft PR #16), based on hardware-tested engine source 3da5576a9971191eef44d027f2cc1727203eb9f6. At this checkpoint the cache candidate is NOT installed: target and companion passed; full host/race checks remain in progress. Current device baseline renderer 2162762987.

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

Current Windows workspace root C:\Users\Dylan\Documents\Codex\2026-10-25\new-chat-9. Fresh baseline: work/paleotone-engine-hotpath/hardware/cache-baseline-1793068624700. Candidate package, companion, guarded scripts and future results: work/paleotone-saurus-cache/hardware/trial-1793068605444. Scripts are syntax checked; installation has not started. Retain previous engine package SHA0899f068b59cf46783e0a0fe96b6636aa82abe37de94f65114213df1568ec607 for rollback.

After full exact-source host/race gates pass: fresh backup, identity-pinned install, verified playback profiling and at least 30-second USB capture; compare all 13 objects by size/CRC/data; leave playback stopped and profiling OFF, core state explicit. Decide whether measured headroom justifies the additional RAM; caching remains a draft until those results. Keep raw backups private.
