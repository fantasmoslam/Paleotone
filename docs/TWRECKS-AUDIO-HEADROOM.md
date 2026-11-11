# Audio headroom: guarded taiko worker

## Report and baseline

Dylan reported 70–101% CPU with pops while playing Lead over HIGH FRONTIER. A read-only Setup snapshot confirmed CPU 82% average / 95% peak and 389 accumulated dropouts; a later saved-state snapshot showed 74% / 98% and 481 dropouts. This establishes a real deadline problem. Earlier speaker-only attribution was not confirmed and does not explain away these missed deadlines.

## Paleotone source review

- [PR 25](https://github.com/fantasmoslam/Paleotone/pull/25) changes browser screen-request cadence only; it does not optimize standalone firmware DSP.
- [Paleotone audio.c](https://github.com/fantasmoslam/Paleotone/blob/main/firmware/src/audio.c) uses the same 256-frame audio half and tracks overload across halves.
- [cpu1.c](https://github.com/fantasmoslam/Paleotone/blob/main/firmware/src/cpu1.c), [fx.c](https://github.com/fantasmoslam/Paleotone/blob/main/firmware/src/fx.c) and the X0X/Melodee-derived guarded [worker HAL](https://github.com/fantasmoslam/Paleotone/blob/main/firmware/hal/fm1_cpu1.h) split independent render work between cores, check job handoff, and fall back if a core fails.
- Stock-supply startup and safe hold/shutdown are taken from Paleotone's fm1_sys.h; clocks are unchanged. Existing copyright and GPL notices are retained.

## Adaptation

Only independent taiko sample playback runs on core 1. Core 0 schedules hits, launches the worker, renders the melodic layers, joins the worker, then mixes its private stereo buffers in the original order. No shared synth engine, random generator, FX state or output accumulator runs concurrently. Each chunk joins before the next transport event or parameter application.

The guarded mailbox, self-test, core-specific stack guard, RAM-resident idle loop, bad-job check and one-core fallback follow Paleotone. Core 1 is held before reset, update-loader entry and memory initialization. A worker fault stays on that core until core 0 holds it. A saved taiko checkpoint restores partial state before serial fallback, preventing double advancement of the samples. Host-only thread tests use mutexes; the device does not.

The waiting loop has its own .c1_text section. Packaging and boot-copy checks prove it lies directly after the flash driver's RAM image and is copied with it; the flash driver's no-XIP-call check remains unchanged. The second-core stacks use existing normal RAM, without adding an untested SRAM region.

## Validation so far

All twenty approved production audio hashes match in serial, synchronous-worker, real threaded-worker and partially-executed-failure modes. The synchronous and threaded modes each complete 174,406 jobs; the injected partial failure falls back once and retains all twenty hashes. Storage/migration checks and updater/loader/installer checks pass. Native build fits: image 580,492 / 581,564 bytes; RAM 95,708 / 98,304; pool 334,472 / 344,064. Hardware CPU improvement is not yet measured.

Current HIGH FRONTIER edits were saved before preparing the install. Musical defaults and the approved audio baselines are unchanged. Installation and an actual before/after device load test are the next boundary; successful host timing does not establish hardware performance.
