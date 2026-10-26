# Paleotone hardware and future VST handoff
Date: 2026-10-26 (America/Phoenix). Owner: Dylan Spaniel.
Status: documentation only; stop after this handoff and wait for Dylan.

## Direction and authorization
Finish hardware Paleotone, make it usable and publicly available, then consider desktop implementation. The side project is parked. User authorized creating this GitHub planning folder and handoff, not starting the VST or flashing hardware.

## Current product problem
SCAPE is too repetitive, subtle or predictable and changes the music because it draws on sequencer/mixer tracks. Dylan wants all four sequencer tracks usable while independent AMB-1-like generation evolves, with every dinosaur call type independently triggerable and editable. Recording, sequencing and generation must work together. The second-core experiment precedes that redesign and does not solve ownership by itself.

## Exact source and validation state
Repository: https://github.com/fantasmoslam/dinobox-fm1
Experimental draft PR: https://github.com/fantasmoslam/dinobox-fm1/pull/11
Hardware baseline branch: feature/scape-garden
Installed baseline source recorded in the prior handoff: 76d32f3050fe21968ac13b017ad8d98a1928b456
Baseline renderer: 799295047
Baseline package SHA256: 9b808fd7da0c4422863d00a607a1158b66562ffa6dbd736c0f2da3297fb501b7
Current experimental source checked: 87e6a38f71c71d10fd16217bad62b2a7336fcf17 on feature/paleotone-dualcore.

- Earlier source 29ce385bd16d3e32e8950fc422d122cf505fd08f passed target and full host gates, including threaded/race checks and worker-active audio regressions.
- Current-source host run 40346779472: success.
- Current-source firmware run 40346779329: failure (exit 128; full root cause still requires inspection). Do not treat the old target pass as current approval.
- Pinned companion run 40346833647 on prep branch commit e2e11efcb0b26c076833a61d75bce13cde68b62a: success, following an earlier emulator-link failure. Its workflow pins the experimental source.
- Earlier target image was 547328 bytes with 34236 free flash; reserve requirement 15872 was preserved using size optimization. These measurements belong to the earlier passing source, not a newly verified current package.
- Nothing in this dual-core trial has been flashed or hardware validated. Do not publish it as stable.

The worker is opt-in and defaults OFF for old settings. CPU0 keeps MIDI, note allocation, sequencer, recording, SCAPE, effects and USB; CPU1 renders selected parts. Voice budget remains twelve. Sensitive engines remain on CPU0; PHYS disables splitting to protect shared randomness. Fault fallback may produce a glitch and does not guarantee a heavy workload fits one core. See docs/DUAL-CORE.md on the experimental branch for ownership and bench procedure.

## Connection and recovery reality
Dylan's Chrome companion connects successfully. Agent browser tooling cannot currently access that tab; native MIDI scans see no FM-1 in the agent session. This does not establish a user-side cable or permission problem. No browser/native device test or installation should be claimed.

Before another trial: restore an actual access path, verify exact-source checks and matching renderer, preserve a fresh settings/projects backup, retain the installed rollback package, and prepare recovery. The copied paleotone-scape.fwsc under hardware/dualcore-trial is the OLD package. probe-installed.mjs is pinned to OLD identity: never use its --install switch as a new candidate installer.

## Source continuity
Prior handoff: PALEOTONE-COMPLETE-HANDOFF-2026-10-25.md, Library ID libfile_84bdfd87dc048191aca2ce83e53224ef. It describes the installed baseline; current work above supersedes its claim that no next blocker remains. Preserve protocol/schema IDs, renderer matching, focus trailer and physical expanded-screen controls. Prior instructions keep Riders removed and SLICER retired; do not reintroduce either from older memory.

Local working directory: C:\Users\Dylan\Documents\Codex\2026-10-25\new-chat-9\work\paleotone-dualcore
Its local HEAD and dirty source do not mirror the API-created remote commits. Do not blindly push it or rerun one-shot port/fixture scripts. Use a clean exact-revision checkout when resuming. Do not overwrite separate or older worktrees.

## Documents and next action
The following sections contain the complete hardware release checklist, desktop plan and multi-firmware feasibility record. Immediate next action after Dylan resumes: resolve the current target workflow failure, establish device access, then hardware measurements. No plugin implementation until the hardware/public release gate is satisfied.


---

# Hardware Paleotone comes first
Planning checklist, 2026-10-26. Unchecked items are acceptance work, not claims that every listed feature is broken.

## 1. Establish a trustworthy candidate
- [ ] Reconcile exact source, package checksum, renderer identity, companion and installation records.
- [ ] Resolve the latest target workflow failure and get all required checks green on one exact revision.
- [ ] Preserve the installed rollback firmware, settings/projects, and recovery instructions.
- [ ] Obtain a working device access path. Dylan can connect in Chrome; the agent currently cannot access that connected tab, and native MIDI enumeration cannot see FM-1. Do not diagnose the user's successful connection as broken.
- [ ] Back up before any authorized hardware install; verify product, package identity and compatibility. No flash has occurred in the current dual-core trial.

## 2. Decide the second core using measurements
- [ ] Test the same demanding projects with 2ND CORE OFF and ON after restart.
- [ ] Measure worst audio block time/late counts, worker faults/timeouts, stack margins and long-run stability, not just average CPU.
- [ ] Exercise USB audio/MIDI, expanded screen, live recording, saves, reset and updater entry together.
- [ ] Verify startup failure and worker retirement behavior; do not manufacture faults on hardware without a controlled recovery plan.
- [ ] Preserve flash reserve, RAM budgets, existing IDs and audio fidelity.
- [ ] Keep the worker experimental or omit it from the public stable build if evidence is insufficient. Dual core is a means to headroom, not a release requirement in itself.

## 3. Redesign SCAPE around independence
- [ ] Specify ownership: four sequencer tracks retain their notes, patches, levels and transport when SCAPE changes.
- [ ] Give SCAPE independent source settings and scheduling; shared final effects are acceptable only with explicit predictable controls.
- [ ] Choose voice allocation, priority/stealing rules, CPU limits and overload behavior from actual measurements. Separate controls alone do not create more voices.
- [ ] Distinguish ambient beds, musical generation and creature calls; define what runs while transport is stopped.
- [ ] Add individually triggerable call types with useful per-type controls: level, pitch/range, duration/envelope, timbre/aggression, variation and occurrence probability as appropriate.
- [ ] Improve variation across short and long timescales: weighted choices, timing variation, phrase memory, avoidance of immediate repeats, evolving density and occasional silence.
- [ ] Define seed/new variation/hold behavior; musical randomness must remain controllable.
- [ ] Compare the listening experience with AMB-1 as a reference without assuming its implementation or copying unreviewed code.
- [ ] Save and restore the new state with versioned compatibility and defaults for existing projects.
- [ ] Demonstrate four occupied song tracks plus SCAPE plus manual calls without overwriting the song. Specify and disclose any measured polyphony limits.

## 4. Finish the complete music workflow
- [ ] Confirm step entry/editing, track mute/levels, pattern/song operation and transport are understandable.
- [ ] Confirm live MIDI/note recording, quantization/overdub behavior, stop/restart and project save/reload.
- [ ] Explicitly define “record”: sequencer event capture versus recorded audio versus captured generative events. Do not promise internal audio recording until storage/bandwidth feasibility is established.
- [ ] Provide a practical way to capture the combined performance through supported audio output/USB audio; document DAW setup.
- [ ] Check levels, clipping, clicks, note-off handling, effects tails and transitions under combined load.
- [ ] Preserve physical FM-1 control of expanded display focus and independent screen banks.
- [ ] Verify backup/restore, schema migration and recovery after interrupted writes with suitable test tooling.

## 5. Make it usable and public
- [ ] Run exact-source host, race/fidelity, target/image, updater/loader, protocol and real-renderer companion gates.
- [ ] Complete hardware listening/usability trials and fix release blockers.
- [ ] Publish a versioned candidate for external testers; collect reproducible feedback and known limitations.
- [ ] Freeze a stable release, tag its source and publish checksummed firmware with the matching companion.
- [ ] Align README, site routes and installer support metadata; preserve existing published paths.
- [ ] Provide quick start, control map, SCAPE/calls guide, recording example, upgrade/backup/rollback and recovery instructions.
- [ ] Audit bundled code, samples, fonts, notices and corresponding source obligations.
- [ ] Publish short musical examples demonstrating sequencing, independent generation and recording together.
- [ ] Establish an issue/report template and a maintenance path; observe the release with real users and resolve blocking regressions.

## Gate before desktop implementation
Evidence required: public stable release URL/tag/checksum; matching working companion; actual hardware results; usable sequencing/generation/recording workflow; backups/recovery; no unresolved release-blocking faults. Dylan then explicitly chooses to resume desktop work. Documentation and limited feasibility research now do not authorize an implementation pivot.
---

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
---

# One desktop instrument, many FM-1 firmwares
Status: feasible direction to investigate later; “EVERY firmware” is an aspiration, not a verified compatibility promise.

## Existing project to evaluate first
https://github.com/joakimeriksson/fm1-emu

Its README describes a chip emulator with a firmware-loading plugin, saved-project support, MIDI input and AU/VST3/CLAP builds. Published plugin instructions currently target Apple silicon; Windows/Linux plugin builds are explicitly described as untried. Our Windows setup and exact Paleotone candidate have not been tested. Reported automation is limited, so loading firmware does not imply a rich editor for every instrument.

This is a strong reuse candidate, not an upstream project we authored or a proven replacement for hardware testing. Pin its revision, review its source/license and validate claims locally before adopting it.

## Three different architectures
| Route | What runs where | Strength | Main limitation |
| --- | --- | --- | --- |
| Firmware emulator plugin | Selected firmware binary runs on the computer | Broad compatibility without a source port per instrument | CPU cost, emulated hardware fidelity, limited generic editing/automation |
| Native engine collection | Each permitted instrument is ported to desktop code | Deep controls, outputs and automation | Separate engineering and maintenance for each firmware |
| Hardware companion | FM-1 runs its installed firmware; desktop controls/records it | Authentic hardware sound | One installed instrument at a time; hardware audio routing and firmware-specific protocols |

Recommended investigation order: emulator compatibility trial, optional FM-1 controller mapping, then native Paleotone only if its benefits justify the effort. Different routes can coexist but should have clear sound/control ownership.

## FM-1 as the physical controller
- Inventory what the currently installed firmware actually sends: note/velocity, knobs/encoders, buttons, clock, channel and SysEx. Receiving remote controls does not mean it transmits them.
- Start with standard notes and learnable CC mappings routed by the DAW; retain ordinary MIDI keyboard support.
- If necessary, design an optional controller mode/firmware with documented bank/page controls and message mappings. This is additional device work, not already present.
- Decide whether the DAW owns the MIDI port or a standalone bridge does; handle port exclusivity, disconnect/reconnect and multiple plugin instances.
- Route controls to one selected instance and prevent MIDI feedback, doubled notes and clock loops.
- For screen/LED feedback, define a versioned protocol and verify outbound support. Not every firmware exposes compatible remote state.
- Default controller-only mode should mute local audio when needed to avoid hearing hardware plus plugin together; make that an explicit user setting.
- A physical unit controls a desktop-selected instrument without needing the same firmware flashed. Reflashing on each plugin selection is unnecessary.
- Device audio and plugin audio are separate paths. USB MIDI is control data, not audio; record hardware audio through its actual supported audio connection.

## Compatibility and rights work
For each firmware/version record: author/source URL, exact revision/package checksum, code and asset licenses, load/boot, audio/MIDI, timing, controls, save/recall, second-core/peripheral needs, CPU/latency and DAW tests. Mark unsupported cases explicitly.

Use author-provided packages or user-supplied local files where appropriate; do not bundle unknown-rights firmware. Local loading does not itself grant redistribution rights. Stock/proprietary images need separate review. Avoid calling a collection universal until a defined catalog passes its matrix.

## Later proof of concept
1. Freeze the released Paleotone package and select a small set of contrasting, licensed firmware versions.
2. Build/run upstream plugin on the chosen OS/DAW and test exact packages.
3. Verify isolated state, firmware switching, restore after restart and missing-package behavior.
4. Test FM-1 note/knob/button mapping with the actual device and document gaps.
5. Measure audio quality, CPU, latency compensation, host clock and offline render.
6. Decide whether to contribute upstream, maintain a thin integration, or build a native Paleotone plugin.
No upstream author has been contacted and no compatibility claim has been independently validated here.