# SCAPE implementation design — approved sound, bounded integration

Status: design and storage-codec foundation, not installed. The owner approved the eight-biome sound direction and the second heavy-footstep audition. Reference audio: dinobox-heavy-footsteps-v2-ab.mp3 (new version at 0:33 and 1:40). Riders remains excluded. Preserve main and existing firmware downloads during development.

## Instrument behaviour
SCAPE sits beside the four tracks, with its own mixer level and reverb send. It is off when its level is zero, including on older-project imports and a fresh boot. With a nonzero level it continues while transport is stopped, like existing AMB. STOP stops the music; turning SCAPE down stops the ambience. Do not send ambient notes to MIDI.

The generator is ecological rather than eight independent random metronomes: biome chooses a coherent population, time controls activity, weather suppresses appropriate birds/insects, and danger schedules occasional encounters. Retain the approved approach → hush → growl → roar → retreat shape and the second footstep revision. Key follows EPOCH by default, with a manual override. Continuous ambience need not be quantized; any tempo-related movement should use measured sequencer tempo without freezing when external clock disappears.

Proposed controls fit the existing four-knob pages:
- SCAPE: biome, level, overall density, danger.
- WORLD: time, weather, key (FOLLOW or C..B), reverb send.
- LAYERS: layer number, level, density, distance; PRESETS selects that layer's supported sound kind.
- DRONE: type, level, movement. It uses WORLD key.
- MIXER exposes SCAPE without taking a track.

All eight layer settings are editable and saved. The initial kind picker covers the prototype's procedural kinds and biome creature calls. Arbitrary engine/preset routing and sequencing a SCAPE layer on a music track are separate extensions, not silently promised by this first integration. Biome changes deliberately reset layer/drone defaults; do not make an accidental browse erase edits. Show a confirmation or apply on OCT+ as existing action pages do. Changes to level, distance and weather must ramp to avoid clicks.

## CPU and voices: enforce priority, do not grow the track array
Keep NTRK/NPART at four. Adding a fifth track would enlarge per-track DSP state and affect many fixed-format assumptions. The audition's callback that overwrites track 4 must never become production code.

Eight configured layers do not imply eight simultaneously active events. Introduce a separate fixed event cap and an admission policy; determine the cap with loaded-song measurements rather than choosing it from desktop timing. Keep approved solo audio at the highest feasible cap. Event omission under overload is preferable to disrupting the song.

Creature rendering needs a small dedicated SAURUS context using the existing engine code, not a complete extra track. Audit SAURUS dependencies before choosing its layout. Count sounding creature voices in the shared budget of twelve, admit only when a music slot is free, and release/fade them before music needs the slot. Do not steal or reconfigure music track voices. Ambient procedural events also consume CPU and need an explicit workload cap, even though they are not SAURUS voices.

Overload handling currently calls shed_voice after two halves above 85% deadline use. SCAPE should respond there before music shedding: refuse new events, fade selected ambient events, then reduce optional drone/bed work, with slow recovery and hysteresis. An average load alone is not enough to catch transient deadlines. Preserve safety margin for USB audio and nested timer work. Counters should record deferred events, active cap and peak load for testing. No skip must strand a creature gate or leave an encounter permanently hushed.

## Storage: lossless seven-bit FM packing
Current project payload is 3840 B, already the storage object's maximum. Four FM patches are 128 B each; proj_pack already rejects any byte above 127. Packing the 512 seven-bit values into 448 B recovers exactly 64 B without losing patch values. Do not borrow bits inside the FM values or change their engine representation.

New proposed disk magic: FUNB (format 11), same 3840 B payload. Keep the existing header, track/step, chain/motion, AMB/KEYS/EPOCH, name and checksum locations. At offset 3312, encode four FM patches as four 112 B blocks, ending at 3760. SCAPE occupies 3760..3823 (64 B). Existing name at 3824..3835 and checksum at 3836..3839 stay in place. Keep the existing two-byte gap before tail globals untouched.

SCAPE record budget: eight four-byte layer records = 32 B; biome, time, weather, danger, density, level, send, drone type/level/movement/key-mode and version/flags fit within 16 B; optional seed 4 B; at least 12 B reserved. Define exact field ranges and canonical reserved bytes before integration. Store configuration, not envelopes, timers or live voices.

New firmware must load FUN1..FUNA and default SCAPE off. Retain the original FUNA decoder: old FM bytes are not bit-packed. New FUNB gets its own magic check and patch decoder. Prefer writing FUNA when SCAPE is fully at canonical defaults/off and no new state would be lost; write FUNB when SCAPE configuration needs storage. Merely muting an edited SCAPE must not discard its saved settings.

Old firmware cannot read FUNB, so keep the installed-firmware backups and do not overwrite slots automatically. A downgrade must not reinterpret packed patches as FUNA. Editor, backup validation, autosave, retained cache and browser import paths all need explicit format support. Maintain one object/one checksum/one atomic commit so a power failure cannot split music and SCAPE settings into different generations.

patch7.h is a foundational codec only. It is not yet included in project.c and changes no existing project data. Its tests use an independent bit-position oracle, guard bytes, exhaustive single-position values, invalid-input atomicity, and arbitrary packed-data roundtrips.

## Resource evidence and gates
Current main image 550180 B. Approved DSP-only probe image 562356 B, RAM 89748 B, pool 300132 B. The probe leaves 19208 B of flash, only 2824 B above the required 16384 B reserve. It excludes real creature integration, UI and persistence. Full feature fit is unproven.

Measure each addition against the same target compiler. First seek savings in redundant configuration/render work and cold code; do not lower the reserve or remove existing sounds without owner direction. Seven-bit packing saves project storage, not executable flash. No claim that storage compression solves the image-size problem.

Implementation gates:
1. Prove the patch codec and layout, including invalid data handling.
2. Integrate versioned storage and roundtrip real four-track projects; test old imports, backup/autosave, corrupt/truncated data and power-failure atomicity.
3. Introduce real creature context and priority admission; test four occupied tracks, twelve voices, mute/STOP/clock changes and overload recovery.
4. Add controls and save state; test edits, biome apply/cancel, default-off and old project compatibility.
5. Measure complete target flash/RAM/pool and real loaded-device deadlines. Existing static engine-loop checks alone do not cover added SCAPE work.
6. Audition the integrated mixer beside four music tracks. Any audible departure from approved audio gets a preview.
7. Full regression/CI and a separately named install package, preserving the working package.

No generator firmware should be offered as finished before these gates pass.
