# Hall effects hotpath comparison

Candidate ba586018234cd8be5fbce09a3c3c7306c4c1389f is now installed and identity-verified on the test FM-1. It includes the earlier master optimization 8fba4c6036f5c201d8eeb81a5d90730e5ee7a6c8. Draft PR #14: https://github.com/fantasmoslam/Paleotone/pull/14.

## Change and software gates

Force-inline only hl_mul, preserving its integer multiply, sign-dependent rounding correction and right shift. No change to Hall coefficients, allpass/state order, clamping, modulation, clocks, reserve, buffers, voice limits or overload thresholds. The target removes 24 helper calls per Hall sample (eight direct calls plus two inside each of six regular and two modulated allpasses); the allpass helpers remain outlined.

All exact-source gates passed before installation:
- Target run 40429298788, job 121942791022, artifact 12701715510: image, updater, loader and unchanged instruction budgets. Hall loop static cost 204 (was 182; budget 345 plus existing 10% tolerance). Inlined arithmetic enters the counted loop; the former count did not expand callees.
- Host run 40429298795, job 121942791520: concurrent mixer, worker sound goldens, ThreadSanitizer, complete single-core sound/CPU/health/UI/recording/storage/backup/installer/browser regressions. Logs end ALL HOST TESTS PASSED. Reverb tests include 60-second tails, offset/stability, level, transitions and partition isolation.
- Pinned companion run 40429330110, artifact 12701805612: exact-source renderer/protocol/browser fidelity. No goldens or budgets were relaxed.

Target image 548560 bytes (+48); RAM 96136/98304 and pool 332900/344064 unchanged. Package SHA256 449b95d46eaa27f38ea11eabc539c4e33c4044436bfa419b83418dbb38884541, size 609905. Installed at 2026-10-27T01:17:06.633Z; product FM-1_900, renderer 696079837, schema 2, 82 globals. Fresh backup captured before the guarded update.

## Hardware comparison

Read-only GET confirmed reverb global 24 = 2, HALL. Same loaded four-track scene at 84 BPM, second core OFF. The harness asserts actual playing=1 in every loaded phase and stops playback and profiling afterward.

| Phase | Baseline effects mean, us/32 frames | Candidate effects mean, us/32 frames |
| --- | ---: | ---: |
| idle | 138.69 | 94.76 |
| playing | 138.79 | 96.70 |
| usb | 139.60 | 97.56 |

The loaded shared effects stage is about 30.3% faster (138.79 -> 96.70 us), not a 30% whole-application speedup. A second candidate run measured 96.68 us without USB and 97.93 us during the longer capture, supporting the stage improvement. Master remains around 72–73 us, retaining the earlier improvement. Loaded effects maxima changed 172 -> 132 us. These live windows are not musically phase-aligned; small changes in parts and other groups are not independently attributed to the helper change.

Unprofiled playback status interval: baseline +1 late block in 6.172 seconds, candidate +0 in 6.282 seconds; overload-shed voices +4 in both. First candidate run peaked at render 2825 us and wall 3037 us versus deadline 2902 us. Across the longer run cumulative maxima reached render 2892 and wall 3122 us. The render-only maximum is now below the deadline in these observations, but wall time still exceeds it. Independently maintained maxima are not additive. Overload voice shedding persists.

## USB results and limits

The eight-second baseline capture produced 21 new underruns, raw minimum 0 frames, producer gap maximum 3901 us. The first eight-second candidate capture produced 0 underruns, raw minimum 72 frames, gap maximum 1606 us; its surrounding status interval had 0 new late blocks versus baseline 7.

A longer 30-second candidate capture (5292000 PCM data bytes) exposed 19 new underruns, raw minimum 0 frames, producer gap maximum 3869 us and 7 late blocks in the surrounding interval. Overload-shed voices increased by 24 across that interval. Therefore the short clean capture did not establish a USB fix. The effects timing gain is repeatable, but some worst-case producer stalls/deadline misses remain. No USB buffering or rate-control constants were changed. Underruns count packet underfills, not distinct audible clicks; see AUDIO-BOTTLENECK-AUDIT.md.

## Saved state, final state and recovery

After the initial test and again after the longer confirmation: all 13 objects matched the fresh pre-install backup in size, CRC and encoded data. Final fresh serial read: playing=0, second core OFF, profiling OFF, idle CPU 29%. No project/preset/settings edits were made by the tests. Previous master-optimized firmware and its matching display remain available for rollback.

Matching current display: https://fantasmoslam.github.io/Paleotone/webapp/screen-effects-hotpath/ (HTTP 200 and exact source/renderer build.json verified). Main site deployment preserves this directory and the older trial companions.

Workspace recovery paths beneath C:\Users\Dylan\Documents\Codex\2026-10-25\new-chat-9:
- Fresh baseline: work/paleotone-audio-hotpath/hardware/effects-baseline-1793062488006
- Package, before/after backups, installation verification, initial profile and validation manifest: work/paleotone-effects-hotpath/hardware/trial-1793062576516
- Longer capture, profile, final matching backup, saved-state comparison and fresh final-status.txt: work/paleotone-effects-hotpath/hardware/confirmation-1793064002790

Do not commit private raw backups. Next measured targets: heavy ROAR/SAURUS rendering, then controlled USB queue/latency experiments if underfills remain once renderer headroom improves. Second-core behavior in this mix still only offloads FM6; the engine exclusions and reserve are unchanged. Independent SCAPE is still a later redesign and is not implemented by these hotpath changes.
