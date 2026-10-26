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