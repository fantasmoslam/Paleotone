# T-WRECKS (placeholder name)

NACHTFAHRT for the M-VAVE FM-1 with Paleotone's sounds: it plays by itself, sparse, and answers what you play.
Package identity **FM-1_942**, USB name `T-WRECKS FM-1`. Built by `.github/workflows/twrecks-fm1.yml` on the
`twrecks` branch; the package lands on `twrecks-fm1-output` (`t-wrecks-FM-1_942.fwsc`).

## Step 1: Nachtfahrt as the base

Nachtfahrt (github.com/FartyPants/fm1-nachtfahrt, ba07283) as it is, its own identity.

## Step 2: Paleotone's sounds as the voices

`firmware/src/pt/`: Paleotone's synth (engines, voices, presets; fantasmoslam/paleotone 6f027f0) in a unit of its
own (`pt.c`, built at -Os as Paleotone is). Each layer's voice (ALGORITHM on its page) turns through its own voices,
then Paleotone's:

| Layer | Paleotone's sounds |
|---|---|
| Drone | 26: ROAR's drones, the choirs and chants, the pads, Bow Metal, Temple |
| Sequence | 22: PHYS's kalimba, marimbas, bells, harps; FM bells; the struck keys |
| Lead | 18: the horns (Mammoth, Bronto, Canyon), Shaman, the brass and leads |
| Sequence 2 | 34: the basses (Tar Bass, Tar Growl, Sub Bass, Pulse...), then the Sequence's |

Each sound is levelled to its layer's own voices (`tools/pt_levels.py`, from `tests/host/twr_voice_test.c`), so the
layer's Volume is its level whatever it plays. Paleotone's SAMPLE and GRAIN engines (its recordings) are not built.
Paleotone's 8 voices are shared by the layers on its sounds.

Tests: `tests/host/pt_test.c` (every sound plays), `tests/host/twr_voice_test.c` (each layer on each sound, its level,
its MIDI notes let go).

## Next

3. The STAMPEDE skin drums, sparse, with their own level; the creature calls.
4. The dinosynth presets (EXTINCTION, STAMPEDE, RAPTOR NIGHT, then the rest of the 20).
