# FM-1 display hardware session — 2026-10-28

## Tested setup and evidence

FM-1 Elite, desktop Chrome, published graphics companion. Starting at
`webapp/screen/` automatically routed to `webapp/screen-roar-worker/`.
Its published build metadata names source `0a1327c816086576b3d741e785c2cfbf7e3d8c42`,
renderer ID `3513154928`, physical focus protocol 2. The live connection passed
the renderer/schema handshake. This is **not** a hardware test of draft PRs
#22–24 or their opt-in 45 ms polling UI.

Observed page readings varied: 7.4–12.3 updates/s, 28.3–35.3 ms transfer,
0.1–0.6 ms draw across individual samples. They are not controlled averages,
input-to-photon latency, or proof of an optimization versus an earlier build.
The updates/s field is cumulative since connection, not instantaneous FPS.

| User-operated condition | Reported result |
| --- | --- |
| Connected normal play, Original | Immediate-feeling response at encoder detents |
| Expanded panel switching | HOME + OCT− / OCT+ worked normally |
| Expanded, two sequencers, extreme simultaneous controls/menu/key spam | Visual stutter and audio glitches; recovered when spam stopped |
| Companion disconnected, sequencer at 240 BPM, aggressive input | No glitches reproduced |
| Original connected, comparable stress | Small visual stutter, no audible glitches |
| Expanded macros/effects, aggressive input | Heavy screen slowdown; possible audio glitching, uncertain |

These comparisons implicate companion activity under extreme load but do not
identify the firmware bottleneck or prove expanded rendering itself blocks audio.
Rendering the additional browser panel runs on the computer. The instrument
also services snapshot transfers and a focus lease; those costs need separating.
No hardware crash or firmware flash occurred during this session.

## Playhead investigation

User initially reported a browser-only horizontal offset. Source inspection
shows `graph_roll` places the playhead at column x + 6 with 12-pixel columns;
note bars start at x + 2. The photos and supplied 10.5-second, 30-fps video at
60 BPM are consistent with centered playheads on both screens. Sampled frames
do not establish accumulating drift or a browser-only geometry defect.
The tall outlined rectangle is the edit cursor, not the moving playhead.
Transition differences may reflect transfer/display latency. No exact latency
has been measured, and no drawing-coordinate correction is justified yet.

## Browser-only mitigation candidate

The current `refreshDelay` allows 30 Hz and adds no idle time when transfers
exceed that interval. New **Audio priority (experimental)** is unchecked by
default, preserving the responsive mode tested above. When enabled it caps
snapshot starts at 10 Hz and leaves at least 12 ms after each completed loop,
including slow transfers. Hidden mode retains its 4 Hz ceiling.

This is a testable request-pressure reduction, **not a confirmed audio fix**.
It deliberately trades some screen smoothness for less instrument request work.
It changes no firmware, MIDI message definitions, renderer pixels, or sequencer
clock. It sends existing snapshot requests less frequently. Physical focus
lease behavior stays unchanged.

`web/prepare_display_trial.py` builds a trial from the exact existing published
ROAR-worker bundle. It preserves its WASM and transport/focus code, changes
only the HTML and scheduling module, and records the WASM SHA-256. It rejects
wrong metadata, missing assets, and an existing destination. No firmware package
or installer is copied. Serve the artifact on localhost or HTTPS; opening
`index.html` as a file is not sufficient for module loading/Web MIDI.

## Validation and next hardware step

Locally passed: display performance tests (including all 65,536 RGB565 colors,
pane caches and priority cadence across 2–260 ms transfer loads), presenter,
physical focus, control targeting, Python trial packaging, full trial package
validation with the published WASM identity, and `git diff --check`.
Real Chromium smoke execution could not start locally because no Chromium
binary is installed. No claim of browser runtime or hardware acceptance of this
candidate follows from the tests.

The dedicated read-only Actions workflow runs regression checks, stages the
matching published renderer, verifies the package and uploads a trial artifact.
It neither builds firmware nor deploys a site.

## Completed PR #25 hardware smoke test

The subsequent October 28 desktop Chrome/FM-1 Elite session used the separate
[PR #25 trial](https://fantasmoslam.github.io/Paleotone/webapp/audio-priority-pr25/).
Expanded mode and a 240 BPM sequencer were requested for the stress passes.
The user operated and listened to the physical instrument.

| Workload | Audio priority | User report |
| --- | --- | --- |
| Sequencer with heavy key/knob activity | Off | No audible glitches reported |
| Sequencer with heavy key/knob activity | On | No audible glitches reported |
| Sequencer with macros/effects stress | On | Screen chugged; no definite audible glitches, although macro modulation made judgment harder |
| Sequencer with macros/effects stress | Off | No audible glitches reported |

Both setting states passed this initial smoke test. **Audio benefit remains
unverified because the unchecked baseline was also clean.** The earlier
intermittent report remains relevant but was not reproduced in this comparison.
Heavy macros still caused visible screen slowdown.

There was one conversational pass per combination, not three controlled
repetitions. Approximately 30 seconds per pass was requested but not independently
timed. Exact patches, macro values, input patterns and right-panel state for
each interval were not recorded. There was no audio recording or automated
glitch measurement, and the exact firmware binary hash was not captured.
Successful synchronization establishes session compatibility, not complete
firmware provenance or quantitative sequencer alignment. No crash or firmware
flash was reported.

### CI and separate publication

[Actions run 40789424183](https://github.com/fantasmoslam/Paleotone/actions/runs/40789424183)
passed for tested implementation head
`91c06fc1aa2d7ce955bd9c2d1d4762da12edda0b`.
The regression, staging and artifact upload steps succeeded. Artifact:
`paleotone-browser-audio-priority-trial`, ID `12860817388`;
ZIP SHA-256 `e0e3344d87d33a3e6e7ad4a122fe832e170d76a4902f9c86ab94c45a556fcd51`.
This is the archive digest, not the WASM digest.

The separately authorized temporary URL was published on gh-pages by commit
`aa678bdc1a7d400f4e21c7592413806ddff39cf6` at
`webapp/audio-priority-pr25/index.html`. It uses PR-head HTML and inlined
display-performance code, with `<base href="../screen-roar-worker/">` to reuse
the matching adjacent renderer assets. It is a packaging variant, not a
byte-identical deployment of the CI archive. The original companion was not
replaced. The workflow itself does not deploy.

The live trial loaded, connected, synchronized, rendered Expanded mode and
exercised both checkbox states on real hardware. It depends on mutable sibling
assets; preserve the full validated artifact and its build.json/WASM hash for
an exact durable reproduction. No automatic cleanup or expiration was configured
for the temporary page.

### Acceptance review and next steps

Keep Audio priority opt-in, unchecked by default and labeled experimental.
Its reduced request pressure is established by scheduler tests; audible benefit
and optimal cadence are not established. Retaining it as a diagnostic fallback
is reasonable, but there is no evidence here to recommend enabling it by default.

Before general release, prioritize a short normal-playing/focus-switch check
with priority on, then reconnect/reload/unplug recovery and hidden/foreground
transitions. Also review Original/Expanded and compatible-display switching.
Longer sessions and representative sounds remain broader acceptance gaps.
These checks are outstanding, not reported failures.

Do not repeat the four completed smoke passes by default. If audio trouble
returns, record exact pages, tempo, active tracks, sound state, firmware/renderer
identity and priority state, then compare the same trigger off/on/disconnected.
If reduced requests do not help, investigate device capture/USB/audio scheduling
and focus-lease costs. No coordinate fix is justified by the current evidence.

PR #25 remains draft. This documentation update does not merge it, replace the
production companion or change firmware.

## Follow-up browser recovery smoke checks

On the same real FM-1 connection in desktop Chrome, the trial was checked with
Audio priority enabled. UI Disconnect changed status to Disconnected; Connect
restored Felucca synchronized. Original then Expanded switching preserved
synchronization. Compatible display enabled the live-image fallback and retained
synchronization; disabling it returned to normal display.

Reload reset the page to disconnected Original mode with Audio priority unchecked,
as expected for non-persisted page settings. Clicking Connect restored
synchronization without a device power cycle. The page was then left synchronized,
Expanded, compatible display off, Audio priority on, right Sequencer/Track 1,
physical controls LEFT. A screenshot confirmed both rendered panels.

These are browser/UI recovery observations, not audio acceptance or a physical
USB unplug/replug test. Physical focus switching with priority on, normal-playing
audio judgment, actual unplug/replug and verified hidden/foreground transitions
remain outstanding. No firmware changes were made.
