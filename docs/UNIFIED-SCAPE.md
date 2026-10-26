# Unified PALEOTONE SCAPE

This integration combines the environment, automatic music and passing creatures under SCAPE,
retains SAURUS 2's 14 calls / 22 presets, and includes firmware-owned dual-screen control.

## Controls

HOME cycles MIXER, CLOCK, KEYS, then six SCAPE pages:

| Page | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- | --- |
| SCAPE MUSIC | Evolution on/off | Musical density | Evolution speed | Mood |
| SCAPE | Environment level | Biome | Distance | Day/night |
| SCAPE DRONE | Drone level | Key (AUTO follows music) | Movement | Creature pitch |
| SCAPE LIFE | Creature track (OFF/T1–T4) | Call rate | Passing-beast rate | Beast size |
| SCAPE LEVELS | Wind | Water | Rain | Insects |
| SCAPE LEVELS 2 | Frogs | Birds | Drips | Rumble |

On SCAPE MUSIC, a key composes ordinary editable 64-step patterns using the four current
sounds. Replacing existing patterns asks for confirmation. Evolution rewrites a later phrase,
preserving the opening, sustained creature parts and track mutes. It pauses when transport stops,
during song-chain playback, or when Evolution is off. Loaded projects supply their own key.

SCAPE LIFE uses one creature-track assignment for occasional calls and passing roars. Beasts
pan across the environment with footsteps and hush wildlife; calls wait until the pass finishes.
Turning SCAPE off releases generated calls/roars. No additional SAURUS instrument is hidden
inside the soundscape.

The eight procedural environmental layers and eight biomes remain. A single harmonic drone
replaces the four overlapping drone synth types. Four ambience event slots replace six.
Per-layer density follows the biome; the separate density pages and independent roar routing
are removed. Previous project IDs and stored fine settings still round-trip. Legacy drone type
and independent roar-route values are retained as data but use the shared drone/creature route.

## SAURUS 2 and memory

The layered-throat engine and its presets are retained. Its scratch pool is bounded to four
simultaneous calls across all tracks (the existing per-track polyphony is also four). An additional
call takes the oldest throat set. The static target-cost baseline is 1757 for the new engine,
measured in the earlier preview. Updating that baseline does not prove real-time hardware
capacity. The combined build must pass the normal 16 KiB flash reserve, RAM and pool checks.

## Dual screens

Use the companion renderer built from this exact integration, not an older published renderer.
Hold HOME and tap OCT+ for the right page; HOME and OCT− for the left. Release the combo,
then play/edit normally. Both pages retain their navigation context. Audio/transport is shared;
swapping writes no flash. Modal edits guard the combo. Page choices persist for the session.

The previously installed build and its matching published screen-device page remain available.
Do not replace their assets with this build's renderer without its matching firmware.

## Validation

The integration workflows build the actual target firmware, check image/loader/flash budgets,
run the complete host suite, and compare the real native/WASM renderer including physical focus.
These are separate from physical playing/listening approval. No device installation is part of
this integration. Final measured results are recorded in the pull request.
