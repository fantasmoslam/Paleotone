# Master DSP optimization comparison

Candidate source: 8fba4c6036f5c201d8eeb81a5d90730e5ee7a6c8 (draft PR #13). Baseline source: c564f667fe54d051eb0158e54b86ba319e814c60. The candidate is now installed and identity-verified on the test FM-1. All software gates passed before installation.

## Target evidence

The exact candidate target build and unchanged image/updater/loader/instruction-budget gates pass (run 40427113987, artifact 12699952991). Master output has no helper calls in target assembly, compared with two DC-block, four conditional low-cut and two clipping call sites before. The standalone master body grows from 153 to 227 instructions as the callees move into it; this static count does not establish runtime speed. The whole image grows by 92 bytes to 548512 bytes. Static RAM remains 96136/98304 bytes; pool remains 332900/344064. No reserve, clock, buffer, threshold, or synthesis changes.

Package SHA256: 8f120eb792f180bb79df30afc11c3b67c1b3b7b9b0199afbc3674cbd8f930ee4. Package size: 609905 bytes. Expected candidate renderer: 2482780981 (computed from exact source). Full host regression (run 40427113932, job 121935528837) and pinned browser companion validation (run 40427418023, artifact 12700751575) passed before installation.

## Fresh baseline repeat

Same loaded four-track scene, 84 BPM, second core OFF. Harness verifies playback after start and each loaded phase, then stops transport and disables profiling.

| Phase | Calls | Master mean, us/block | Effects mean, us/block | Parts mean, us/block |
| --- | ---: | ---: | ---: | ---: |
| Idle | 4352 | 89.53 | 139.28 | 26.76 |
| Playing | 18144 | 90.20 | 141.28 | 337.87 |
| Playing and USB | 14424 | 89.89 | 142.59 | 343.37 |

The playing master mean repeats the earlier 90.12 us measurement closely (90.20 us here). Before phase profiling, late blocks increased by 5 and overload-shed voices by 4 across the status interval. During the eight-second USB capture, underruns increased by 293, compared with 16 in the previous capture; late blocks increased by 74 across the surrounding status interval. These are varying live-workload windows, not phase-aligned A/B benchmarks. The repeat confirms overload and USB starvation but cautions against claiming improvement from one short dropout count. Cumulative render/wall maxima remained 3065/3295 us, deadline 2902 us, nested maximum 247 us. Maxima are independent.

Connection-only candidate probe verified baseline renderer 3998626645 and all 82 globals, with no active companion revision changes. It performed no firmware write.

## Recovery and next actions

Exact baseline package and pre-install backups remain in the diagnostics hardware session. Candidate package and guarded scripts are staged under work/paleotone-audio-hotpath/hardware/trial-1793061363444. Fresh repeated baseline raw results are under work/paleotone-audio-diagnostics/hardware/baseline-repeat-1793061393344. Do not commit saved user-state objects or raw backups.

After software gates pass: fresh backup; identity-pinned install from renderer 3998626645 to 2482780981; repeat verified transport profiling and USB capture; compare stage costs and deadline rates; compare saved object CRC/size; leave playback stopped and profiling off. The results below establish a master-stage improvement, with remaining overload.

## Completed candidate hardware comparison

Installed and verified at 2026-10-27T00:49:28.617Z: product FM-1_900, renderer 2482780981, schema 2, 82 globals, exact package SHA above. Fresh pre-install backup saved; old diagnostics package remains available for rollback. Same loaded four-track scene at 84 BPM, second core OFF. Playback asserted throughout all loaded phases.

| Phase | Baseline master mean, us/32 frames | Candidate master mean, us/32 frames |
| --- | ---: | ---: |
| idle | 89.53 | 71.03 |
| playing | 90.20 | 72.26 |
| usb | 89.89 | 72.70 |

The loaded master stage is about 19.9% faster in this comparison. This is not a 20% speedup of the whole application. Playing master maxima changed 117 -> 103 us per control block. Aggregate parts remained about 338 -> 343 us and effects 141 -> 139 us; varying phrases/voices and cache effects mean these other small differences are not independently attributed to this change.

The unprofiled playback status interval was approximately 6.2 seconds in each run: late-block increments 5 -> 2; overload-shed voice increments 4 -> 4. Candidate render maximum 3025 us and wall maximum 3255 us still exceed the 2902 us deadline. Thus overload protection remains active. Cumulative late counters survive across reset: baseline repeat ended at 201, candidate started at 206; compare interval deltas, not absolute totals.

Candidate eight-second USB capture completed, with 25 new underruns, raw ring low-water 0 and producer publication-gap maximum 3976 us. The repeated baseline capture had 293 underruns but an earlier baseline capture had only 16. No claim of a reliable USB fix follows from these short windows. Candidate LCD timeouts remained 0; maximum nested-interrupt time 256 us. Independently maintained maxima must not be summed.

Post-test backup: all 13 objects match the fresh pre-install backup in size, CRC and encoded data. A fresh final serial read verifies playing=0, second core OFF, profiling OFF, idle CPU 35%. The second-core reserve and all synthesis/overload settings are unchanged.

Matching display: https://fantasmoslam.github.io/Paleotone/webapp/screen-audio-hotpath/ (HTTP 200, exact source/renderer build.json verified). Existing diagnostics display remains for rollback. Main site deployment preserves both. Draft PR: https://github.com/fantasmoslam/Paleotone/pull/13. Detailed six-item architecture audit: AUDIO-BOTTLENECK-AUDIT.md.

Next optimization targets are the shared effects and heavy ROAR/SAURUS rendering, with the same sound-preservation and hardware deadline gates. Independent SCAPE still needs its own redesign; this measured headroom change does not implement that separation.
