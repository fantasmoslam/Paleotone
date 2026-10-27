# Master DSP optimization comparison

Candidate source: 8fba4c6036f5c201d8eeb81a5d90730e5ee7a6c8 (draft PR #13). Baseline source: c564f667fe54d051eb0158e54b86ba319e814c60. No candidate firmware has been installed at this checkpoint.

## Target evidence

The exact candidate target build and unchanged image/updater/loader/instruction-budget gates pass (run 40427113987, artifact 12699952991). Master output has no helper calls in target assembly, compared with two DC-block, four conditional low-cut and two clipping call sites before. The standalone master body grows from 153 to 227 instructions as the callees move into it; this static count does not establish runtime speed. The whole image grows by 92 bytes to 548512 bytes. Static RAM remains 96136/98304 bytes; pool remains 332900/344064. No reserve, clock, buffer, threshold, or synthesis changes.

Package SHA256: 8f120eb792f180bb79df30afc11c3b67c1b3b7b9b0199afbc3674cbd8f930ee4. Package size: 609905 bytes. Expected candidate renderer: 2482780981 (computed from exact source). Full host regression and pinned browser companion validation are pending; do not install before both pass.

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

After software gates pass: fresh backup; identity-pinned install from renderer 3998626645 to 2482780981; repeat verified transport profiling and USB capture; compare stage costs and deadline rates; compare saved object CRC/size; leave playback stopped and profiling off. A measured performance benefit has not yet been established.
