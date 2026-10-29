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