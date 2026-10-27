# Hall effects hotpath comparison

Candidate source: ba586018234cd8be5fbce09a3c3c7306c4c1389f (draft PR #14), based on installed master optimization 8fba4c6036f5c201d8eeb81a5d90730e5ee7a6c8. Current checkpoint: candidate NOT installed; target passed; full host and pinned companion checks pending.

## Source and target evidence

Force-inline only hl_mul, preserving its exact integer multiply, sign-dependent correction and right shift. No changes to coefficients, allpass order, modulation, clamping, state, clocks, reserve, buffers, voices or overload thresholds. The target removes eight direct calls per Hall sample plus two calls inside each of six regular allpasses and two modulated allpasses: 24 helper calls per sample in total. The allpass helpers remain outlined.

Target build run 40429298788, job 121942791022, artifact 12701715510 passed all unchanged image/updater/loader/instruction-budget gates. Hall loop static cost is 204 (was 182; budget 345 plus existing 10% tolerance). Inlining moves arithmetic into the counted loop, whereas the old static count did not expand the callees; this increase is not a runtime slowdown measurement. Target image 548560 bytes (+48), static RAM 96136/98304 and pool 332900/344064 unchanged.

Package SHA256 449b95d46eaa27f38ea11eabc539c4e33c4044436bfa419b83418dbb38884541, size 609905. Expected renderer 696079837. Candidate companion workflow is pinned to the exact source. Existing reverb tests include Hall decay/level, maximum-size sustained input followed by 60 seconds of silence, offset/stability, model transitions and delay partition isolation. Sound goldens and budgets are unchanged.

## Fresh installed-baseline hardware measurement

Read-only GET confirmed global 24 (reverb model) = 2, HALL. Same loaded scene at 84 BPM, second core OFF. Harness verified actual playing=1 in every loaded phase, stopped at end, profiling OFF.

| Phase | Calls | Effects mean, us/32 frames | Master mean, us/32 frames |
| --- | ---: | ---: | ---: |
| idle | 4320 | 138.69 | 71.15 |
| playing | 18132 | 138.79 | 72.61 |
| usb | 14304 | 139.60 | 72.54 |

USB capture completed with 21 new underruns. Short capture underrun totals vary; compare stage costs and interval deltas, not one count or cumulative maxima from different lifetimes.

## Local recovery/checkpoint

Baseline raw results: work/paleotone-audio-hotpath/hardware/effects-baseline-1793062488006. Candidate package and identity-pinned installer/profile/backup verification tools: work/paleotone-effects-hotpath/hardware/trial-1793062576516. Scripts were syntax checked; no transfer has started. Do not commit private backup data. After all gates: fresh backup, guarded install from renderer 2482780981 to 696079837, repeat verified playback/USB measurements, compare all saved objects and verify stopped/profiling-off cleanup.

Matching current device display remains screen-audio-hotpath until installation. This candidate does not implement independent SCAPE or change its voice allocation.
