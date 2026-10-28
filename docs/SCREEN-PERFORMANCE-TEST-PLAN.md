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
