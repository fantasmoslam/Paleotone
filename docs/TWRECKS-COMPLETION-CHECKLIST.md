# T-WRECKS completion checkpoint

Approved and installed firmware source: 43703d8. Later acceptance/test updates do not change that firmware's sound or require another flash.

| Item | Evidence / status |
|---|---|
| Musical direction | RAPTOR NIGHT and all six western scenes explicitly approved on the device |
| Control UI | Large labels and colored fill indicators; independent saved instrument/beat selectors |
| Instruments | Cello, Cello Low, Sub Pad and Fat Pad available on each melodic layer; 116 registered voices checked |
| Drums | Eleven exact imported beat families plus three independent western variations; fourteen finite render checks |
| Creature calls | Manual by default; optional Auto; key routing and stop/mute behavior covered by scenarios |
| Preset bank | Twenty scenes; six authorized replacements; fourteen retained saved slots verified byte for byte |
| Storage | Beat/instrument save and reboot checks, interrupted replacement write/retry and old-header compatibility pass |
| Audio regression | Twenty production baselines repeat; RAPTOR NIGHT baseline preserved; ten legacy fixture baselines unchanged |
| Device installation | Write/restart verified; RAPTOR NIGHT observed at the same 130 BPM, F Phrygian and Drone controls before/after |
| Resource fit | Image 578,684 / 581,564 bytes; RAM 91,972 / 98,304; pool 333,448 / 344,064 |
| Updater | Entry, loader bounds/recovery and installer protocol checks pass |
| Full CI audit | Original full run passed every check except a six-second FM bell release deadline; all control scenarios, including orphans, passed |
| Corrected release check | The two voices were released FM bells and died naturally within one additional second; a bounded ten-second window passes, without changing audio code |

## Remaining device acceptance

- Play the western scenes with live Lead, creature calls and control changes; check average/peak CPU and dropout count on Setup (tap GLO briefly).
- Compare any speaker crackle through headphones before classifying it as a firmware fault. Cause is not confirmed.
- Preserve unsaved performance edits: the latest read-only snapshot showed HIGH FRONTIER with unsaved Lead edits. No reboot/flash is needed for this checkpoint.

The reported 55–75% CPU and a prior 60% average / 83% peak, zero-dropout snapshot establish only those observed conditions, not a worst-case test. No further musical rewrite is planned for the approved reference scenes.

Package SHA-256: 79bf9e572016c11554acba3a462c3321da166df4fe41ea446bcb7b937da007b1
