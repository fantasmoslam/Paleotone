# FM-1 second-screen performance test plan (experimental only)

> **Do not install experimental firmware for this test.** These changes are browser-side and are stacked on PR #23. No work on `main` is needed.

## Prerequisites
- Known-good FM-1 firmware and a verified recovery backup.
- Compatible matching graphics-companion bundle (renderer identity must match the firmware).
- Desktop Chrome/Edge, USB **data** cable; optionally Android Chrome with a supported Web MIDI implementation.
- Close DAWs, firmware installers, and other MIDI applications so only one page owns the FM-1 MIDI ports.
- Do not attempt the flash installer during the session.

## Compare the same workload in each mode

For each mode below, use the same cable, browser, firmware and pages. Note device/browser versions and capture the status counters. Run each case for 60 seconds; repeat three times.

| Case | Layout | Display mode | Refresh | Exercise |
| --- | --- | --- | --- | --- |
| A | Original | Canvas | Normal | Idle, then turn one encoder continuously |
| B | Expanded | Canvas | Normal | Sequencer running with four tracks |
| C | Expanded | Canvas | Fast (experimental) | Repeat B without changing firmware |
| D | Expanded | Compatible image | Normal | Repeat B |
| E | Expanded | Compatible image | Fast (experimental) | Repeat B |
| F | Expanded | Current display | Normal | Switch control targets and rapidly change parameters |

Record frames received / painted / skipped, any black frames, stalls, audible artifacts, and input-to-screen delay if a high-frame-rate video is available. The counters describe *browser presentation calls*, not MIDI packets or USB bandwidth.

**Important:** the canvas image-mode fallback still has a built-in 90 ms gate; the 45 ms fast refresh toggle does not bypass it. Don't interpret the 45 ms request interval as a guaranteed 22 FPS display.

## Reliability and recovery
1. Disconnect/reconnect the USB cable while viewing the display and note whether the app recovers cleanly.
2. Background/foreground the browser tab, then verify both displays remain synchronized.
3. Change Original/Expanded and Compatible/Canvas during a session; inspect for stale or black frames.
4. Confirm the correct renderer/version or a useful compatibility error appears for a mismatched build.
5. If control behavior, audio output, or MIDI connectivity becomes abnormal: stop testing and return to the known-good setup.

## Acceptance before any merge
- No added MIDI commands or firmware changes in this performance branch.
- All `web/test_frame_presenter.mjs`, `web/test_companion_performance.mjs`, and existing web tests pass.
- Compare normal vs fast with real-device numbers. Fast must have measurable benefit without audio or reliability regressions; otherwise retain normal.
- Check desktop and Android separately. Avoid claiming FPS based solely on timer intervals.
- Keep the new Mesozoic visual redesign and any audio/sequencer telemetry in independent PRs.


## Development log — 2026-10-28

- Continued browser-only performance experiments on `experimental-screen-fast-refresh-20261028`; no changes to live firmware or `main`.
- Corrected the frame-presenter context-loss check so a lost Canvas context can trigger the compatible fallback even during the compatible update throttle.
- Existing regression fixtures and the benchmark checklist remain on the experimental branch; physical FM-1 performance has not been measured.
- Open draft pull requests: [#23](https://github.com/fantasmoslam/Paleotone/pull/23) (presentation optimization), [#24](https://github.com/fantasmoslam/Paleotone/pull/24) (faster polling and diagnostics).
- **Testing gate:** run Node regression tests and desktop/mobile browser checks before any merging; do not interpret the polling timer as measured display FPS.

## Explanation for the eventual test session

The browser renders firmware pixels received over MIDI. Performance depends on how long the FM-1 takes to send state, how long browser/WASM rendering takes, and how often the browser presents new pixels. Skipping identical frames reduces browser work but doesn't accelerate USB transport. The experimental fast-poll option requests updates more frequently; it may not improve the actual displayed frame rate if USB is already the bottleneck. The original pixel-perfect renderer remains the reference.

### Additional regression work

- Fixed a subtle context-loss case: if Canvas loses its graphics context while the FM-1 display remains unchanged, the old frame-deduplication logic could skip recovery indefinitely. Context health is now checked *before* comparing pixels.
- Added a regression case that feeds identical pixels after context loss and confirms migration to the compatible image renderer.
- Scope remains limited to the browser experiment. These changes are not yet hardware-verified, merged, or deployed.

- Reset cached ImageData on Canvas/compatible mode transitions to avoid reusing a drawing buffer across surfaces; regression validation remains pending.

- Extended frame-presenter regression assertions to verify that Canvas and compatible image modes maintain distinct cached ImageData surfaces, including context-loss fallback. Hardware and CI validation pending.

- Added a dedicated GitHub Actions workflow (`experimental-screen-tests.yml`) that runs the four focused browser/renderer regression scripts on experimental-branch pushes or by manual dispatch. It does not build or flash FM-1 firmware. CI results must still be checked; a committed workflow alone is not proof of passing tests.
- Expected outcome: catch display regressions earlier, reduce duplicate redraw work, and determine whether the optional higher polling rate produces meaningful visible improvements without increasing failures. Hardware measurements remain necessary.

### Frame workload regression — 2026-10-28

- Instrumented the frame-presenter test fixture to count PNG encodes, canvas writes, and ImageData allocations.
- Added a deterministic 100-repeat identical-frame test. The expected outcome is **zero additional encodes, draws, or allocations** for unchanged pixels after the first presented frame.
- This verifies the intended optimization through regression assertions, not measured device FPS. The browser-based hardware comparison still determines actual benefit.
- Checked CI availability through the GitHub connector; no completed workflow run was returned for the queried commit, so no CI-pass claim is made.

### Compatible-mode pacing regression

- Added a controlled clock test: a changed frame arriving 45 ms after the last displayed frame must not trigger PNG encoding or overwrite visible pixels; the latest frame arriving after the 90 ms gate must be shown.
- Also verifies a subsequent frame can restore the original pixels. Expected improvement: fewer wasted PNG operations and less browser churn, without silently losing the most recent screen state.
- Assertions are committed but not yet confirmed by a completed test run or FM-1 hardware measurement.

### Diagnostic improvements and fixture maintenance

- Corrected the painted-frame counter expectations after adding additional compatible-mode pacing cases. The expanded test now accounts for all actual draws rather than the earlier baseline.
- Experimental companion now displays measured browser presentation FPS (delta painted frames / elapsed time), alongside accumulated frames received, painted and skipped. This is **not** the same as device-side frame generation rate or USB transfer throughput.
- Updated the experiment's static guard to check for the presentation-rate instrumentation.
- Expected result: clear normal-vs-fast comparison during the hardware test, without claiming that polling targets equal delivered FPS. Node test and live hardware verification pending.

## Pre-hardware readiness gate (2026-10-28)

Software preparation that does **not** require the FM-1:
- [x] Isolate graphics speed changes on draft pull requests.
- [x] Add frame de-duplication and ImageData reuse.
- [x] Cover image-mode pacing, exact pixels, and canvas context-loss recovery in regression fixtures.
- [x] Add on-screen presentation-rate and frame-skip counters.
- [x] Add opt-in faster refresh and an experimental-only CI workflow.
- [x] Fix sampling baseline reset on each successful MIDI connection; prevent the first FPS measurement from using page startup time.
- [ ] Confirm Node tests complete successfully in CI; investigate any red job.
- [ ] Verify production-built companion bundle generated from this branch still loads all matching scripts and WASM assets.
- [ ] Validate Chrome/Edge UI in a real browser without hardware using fixture data; check layout changes on phone-sized screens.
- [ ] Capture a baseline under simulated unchanged and changing frames, and verify memory allocation profiles.

Work that **must wait for hardware**:
- [ ] Record real transfer latency, screen paint rate, input-to-photon response, and error/retry counts.
- [ ] Compare normal and fast polling under idle, knob rotation, and four-track playback.
- [ ] Confirm no audio or sequencer regression with the full graphics companion running.
- [ ] Exercise disconnect/reconnect, USB sleep/wake, and error recovery with real MIDI ports.
- [ ] Check whether live browser state captures accurate peak/RMS and dinosaur-call activity before designing new firmware telemetry.

**Decision gate:** do not interpret synthetic timers, status counters, or passing mocks as a proof of USB speed. Keep 45 ms fast refresh opt-in unless hardware benchmarks show a consistent improvement without failures.

## Software-only gate update — 2026-10-28

- Added `web/test_screen_workload.mjs`, a deterministic 1,000-frame workload checking unchanged frames, constantly changing 480×240 frames, image buffer reuse, and mobile fallback encoding frequency. It is a synthetic workload, **not an FPS measurement of the FM-1**.
- Added `web/test_companion_packaging.py`, which assembles a graphics companion from synthetic renderer files using the same publish helper and verifies expected JS, layout, WASM and build metadata files are carried into the output.
- Extended the experimental-only GitHub Actions workflow to execute both tests on branch pushes.
- Initialized presentation FPS sampling only after the MIDI connection handshake, so startup delays do not distort the first sample.
- No completed CI execution was confirmed using the available GitHub integration. Treat these as **committed test cases, not passing test results**. The experimental workflow itself also needs to be observed after a real GitHub Actions run.

### Remaining software verification (not dependent on FM-1)

- Confirm GitHub Actions runs the new tests successfully, fix any failures, and inspect the site assembly output.
- Exercise browser layout with real desktop/mobile browser rendering and fixture MIDI state. Browser Web MIDI functionality with actual hardware still requires the FM-1.

### Hardware-only acceptance

- Real-world input-to-display latency, USB transport throughput, frame delivery stability, actual second-panel accuracy, and interference with sound/sequencer activity cannot be established without the instrument.
- Keep the speed toggle opt-in and all pull requests in draft until those measurements are available.

### Direct JavaScript fixture execution (2026-10-28)

Executed the branch's actual `FramePresenter` source plus the `test_frame_presenter.mjs` fixture in an isolated V8 environment, replacing only Node's import/assert scaffolding. **Passed.**

Executed `test_screen_workload.mjs` against the same source and deterministic clock in V8. **Passed.** Recorded:
- 1,000 unchanged 480×240 frames: **1 painted, 999 skipped**.
- 1,000 continually changing frames in Canvas mode: **1,000 painted**, one reusable ImageData allocation.
- 1,000 changing frames in compatible image mode, with synthetic 45 ms arrival spacing: **500 painted, 500 skipped** by its 90 ms gate.

These figures are deterministic *rendering simulation counts*, not device-measured frames per second or USB speeds. Normal Node/Actions and real-browser integration tests remain to be confirmed. No hardware-dependent claim follows from these numbers.

## Software-only completion pass — 2026-10-28

- Added a real headless Chromium browser smoke test (`web/test_presenter_browser.html`) that exercises the branch's actual Canvas presenter, confirms precise pixel changes, deduplicates unchanged frames, then switches to compatible PNG display.
- Wired the real-browser check into `.github/workflows/experimental-screen-tests.yml` using a temporary local HTTP server and Chrome; the test does not need MIDI or the FM-1.
- Added syntax checks of screen browser modules and publishing Python files. Existing test jobs exercise synthetic packaging, renderer logic, page routing and the 1,000-frame workload.
- Independently executed the JavaScript frame regression fixture and synthetic workload in V8. Both passed (results above).
- **Outstanding verification:** Actions workflow run results are not visible through the available commit-status interface (it reports no statuses); the new Chrome check and Python packaging test have not been observed executing in GitHub Actions. This is a tooling/verification limitation, not a successful test result.
- Full end-to-end production site generation requires the pinned graphics renderer source, generated WASM, and build artifacts. The synthetic packaging test covers the helper but cannot establish a production release without running that full pipeline.
- Do not merge, publish, or flash solely on these findings.

### Proposed hardware test entry gate

Once the experimental Actions run is green and the compiled companion can be loaded from a staging URL, the remaining questions become hardware-specific: device transport, audio/sequencer stability, UI button control, and measured latency. If CI or browser rendering fails, fix it before the FM-1 session.
