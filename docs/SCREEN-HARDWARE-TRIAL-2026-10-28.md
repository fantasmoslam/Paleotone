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

Next: compare the **same trial page** unchecked versus checked, same firmware,
pages, sound and 240 BPM stress workload, ideally three repetitions per mode.
Record definite audio glitches separately from display stepping. Check normal
play and physical focus switching too. If the cap does not help, investigate
device capture/USB/audio scheduling rather than simply increasing refresh.
Keep this change draft; no merge, live-site replacement or firmware flash has
been authorized as part of this trial.
