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
