# Future Paleotone desktop instrument
Status: parked until the hardware release gate in HARDWARE-FIRST.md is met.

## Product
A playable prehistoric instrument: deliberate four-track composition alongside evolving habitat/music and independently performed creature calls. It must generate sound on the computer without requiring the FM-1. The FM-1 is an optional physical controller.

The current companion's display role does not automatically supply plugin editing, automation or audio generation. Reusable software rendering is useful groundwork, not a finished host-safe instrument.

## Ordered start plan
1. Record the public hardware baseline and acceptance results; preserve that maintenance branch.
2. Ask Dylan which DAW/version and desktop OS to support. Windows x64 is a sensible first candidate given the current setup, not a confirmed specification.
3. Audit source/assets/dependencies and select a compatible licensing/distribution approach.
4. Evaluate the existing FM-1 emulator plugin before building a new wrapper (MULTI-FIRMWARE.md). Compare one exact Paleotone package, sound, saves, CPU and MIDI behavior.
5. Decide emulation versus a native source port from measured results. Emulation favors original behavior and multiple firmwares; native favors deep editing, automation and routing.
6. Make a minimal standalone audio harness, then an empty VST3 instrument that loads/unloads in the chosen DAW.
7. Connect one voice and MIDI note-on/off; support host sample rates and variable block sizes, timed events, reset and all-notes-off.
8. Remove firmware assumptions from the native path: singleton/global state, hardware registers, fixed clocks, flash writes, USB ownership and the hardware worker. Desktop instances need isolated state; do not reproduce the FM-1 dual-core scheduler by default.
9. Add full state serialization and versioned migration; two instances must remain independent and reopen correctly.
10. Add host tempo/transport and explicit internal versus host clock modes. Choose DAW piano roll versus internal sequencer authority to avoid double playback.
11. Add independent SCAPE, call mappings and editable sequencer/instrument controls.
12. Add stable parameter IDs, ranges/units, smoothing and thread-safe automation. Use bounded queues between UI/control and audio; no allocation, blocking locks, filesystem/network access on the audio callback.
13. Test session recall, offline bounce, sample-rate/block-size changes, fast transport jumps, loops, tempo changes, suspend/resume and editor reopen.
14. Profile demanding combinations and multiple instances; test silence, denormals, clipping, stuck notes and long runs.
15. Validate with the official format validator and the selected real DAW; CI is necessary but cannot replace host trials.
16. Package a limited beta with notices/source requirements, installation/uninstallation, known limitations and crash-report instructions.
17. Collect feedback, stabilize and release; add platforms/formats only after support capacity exists.

## Feature priorities
First usable version:
- Full instrument/sequence/SCAPE editing, stereo sound, MIDI input and host synchronization.
- Individual call triggering and meaningful controls.
- Independent generation with seed/hold/new variation; all required state saved with the DAW project.
- Stable automation for selected important controls; reliable preset and session recall.
- A practical recording path: DAW audio recording/bounce first; MIDI capture/export only where supported and tested.

After the first beta:
- Separate song/SCAPE/call outputs, additional polyphony after profiling.
- Capturing generated notes and controller gestures, richer modulation and preset browsing.
- FM-1 control mapping and optional feedback.
- AU/CLAP/macOS/Linux as scoped follow-ups; no current platform guarantees.

Success means a reproducible musical workflow and dependable host behavior, not merely a working plugin window.

## Licensing/framework decision
Paleotone inherits GPL-covered code and third-party assets: inventory exact files/versions, notices and source obligations before distribution. An SDK license does not relicense the instrument. Do not assume JUCE is automatically the correct/free compatible choice; check the chosen version's terms and integration. Compare a direct SDK wrapper, a suitable framework, and existing upstream plugin work.

Sources checked 2026-10-26:
- Steinberg VST3 SDK: https://github.com/steinbergmedia/vst3sdk (current SDK MIT; pin and review the actual dependency).
- JUCE terms: https://github.com/juce-framework/JUCE/blob/master/LICENSE.md
- Felucca source/licensing: https://github.com/hugelton/Felucca
No framework has been selected and no plugin implementation has started.