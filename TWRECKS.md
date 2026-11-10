# T-WRECKS (placeholder name)

Paleotone's generative branch for the M-VAVE FM-1: a dark dinosynth world plays by itself and you play over it,
in the way of Nachtfahrt and Aisling. Paleotone's sounds, engines and pages are all still here; the groovebox's
front screen is replaced by T-WRECKS's own.

Package identity **FM-1_942**, version string `T-WRECKS 0.1`, USB name `T-WRECKS FM-1`. Built by
`.github/workflows/dinosynth-fm1.yml` on the `dinosynth` branch; the package lands on `dinosynth-fm1-output`.

## Step 1: the shell (step 2 adds the varied arc, the sound swaps and SHUFFLE)

- **The screen**: the preset, its key and mode, the arc (EMERGE, WALK, HUNT, BRIDGE), a light for each layer as it
  plays, the four knobs.
- **ALGORITHM** picks what PRESETS changes: PRESET (the first place), or a layer (DRONE, OST, BASS, LEAD, CALLS,
  DRUMS). The keys play that layer (the LEAD on PRESET).
- **PRESETS** on PRESET picks the preset (three rough ones so far: EXTINCTION, STAMPEDE, RAPTOR NIGHT); on a layer it
  swaps that layer's sound through all of Paleotone's (on DRUMS: the kit). Swaps last until another preset loads.
- **REC**: SHUFFLE, every layer a new sound of the same kind (a pad for a pad, a bass for a bass).
- **SELECT** sets the tempo. **PLAY** starts and stops. **OCT- / OCT+** move the keys' octave.
- **The keys** play the LEAD, white keys walking the preset's mode from C4 (the root).
- **KNOB 1..4**: DENSITY (how many of the figures' notes play), DRUMS (0 = none), CALLS (how often a creature
  calls), AMB (the cave wind and crackle).
- **HOME** tapped: Paleotone's TRACKS screen (the mix); tapped again: back. **ENV, LFO, FX, EDIT ...** open
  Paleotone's pages for the selected layer. **HOME held**: the menu.
- **MIDI**: in, as Paleotone (each layer's channel in GLO > MIDI). Out: the keys always; everything the conductor
  plays too (SYSTEM > OUT = SEQ, set at power-on), each layer on its channel.
- T-WRECKS does not save anything yet, and never loads or overwrites Paleotone's autosaved project.

## The layers

| Track | Layer | What plays it |
|---|---|---|
| 1 | DRONE | the conductor: the chord's root and fifth, held; the chord moves every four bars |
| 2 | OST | the conductor: a figure of 16 sixteenths from the chord's tones |
| 3 | BASS | the conductor: a second figure of 15, low; the two drift against each other |
| 4 | LEAD | you (keys, MIDI in) |
| 5 | CALLS | the conductor: a creature now and then, most in the HUNT |
| 6 | DRUMS | the conductor: the ROAR rhythms on the STAMPEDE kit (ROAR's skin drum without the gurdy drone) |
| | atmosphere | ambience.c, no voice |

## Tests

`tests/run_tests.sh` renders each preset for 150 s (`TWR=n build/host/hostsim 0 0 1 out.wav`, `SOLO=k` for one
layer alone) and drives the screen (`tests/twr_ui_test.c`, screenshots `build/host/twr-*.ppm`). Paleotone's own UI
tests run with `TWRECKS 0`, so its pages stay tested as they are.

## Step 2: plays itself

- The arc varies: WALK 12 or 16 bars, HUNT 16 or 24 (once it has been heard), BRIDGE 4 or 8, now and then skipped,
  half the time hollow (the drone alone, or nothing). A section begins on the deep drum and a far thunder.
- The HUNT builds (more notes as it goes, ghost strokes in its second half, its end now and then an octave up).
- The drums: INTRO the low drum's loud strokes; A the high drum only in the WALK's second half, no talking drum; B
  the talking drum and the ghosts; FILL every fourth bar; BREAK soft and far.
- The chords: now and then a substitute a third up, the last chord hanging on, or the drone on a fourth.
- Each pass the figures change a little more (one step, up to four).
- `TRACE=1 TWR=n build/host/hostsim 0 0 1 out.wav` prints each section as it begins.

## Next

3. Follows you: the LEAD answers your phrases; the CALLS answer too.
4. All 20 presets, the other pages, saving.
