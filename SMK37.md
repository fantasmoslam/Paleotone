# PALEOTONE on the M-Vave SMK-37

PALEOTONE also builds for the M-Vave SMK-37, a 37-key controller with the same chip as the FM-1. The FM-1 build is
unchanged (byte for byte); everything for the SMK-37 is behind `BOARD_SMK37`. On the SMK-37 it is the same
PALEOTONE: the same sounds, scenes, SCAPE, screens, boot and manual, with the keyboard's extra controls put to work
on the soundscape and the herd. The setup is recorded in [smk37-port.yaml](smk37-port.yaml).

The board layer comes from genkigenki's port kit (github.com/genkigenki/fm1-to-smk37) and its GHOULBOX port
(genkigenki/Felucca, branch `ghoulbox-smk37`), carried onto PALEOTONE.

## Supported models

- **SMK-37 Pro**: the default build (package identity `SMK-37 Pro_900`), `paleotone-smk37.fwsc`.
- **SMK-37 Elite**: `MODEL=elite` (identity `SMK-37 Elite_900`), `paleotone-smk37-elite.fwsc`.
- **MKE-P37**: `MODEL=mke` (identity `MKE-P37_900`), `paleotone-smk37-mke.fwsc`.
- **Starrykey 37 Play**: uses the SMK-37 Pro build.

All of them are the same hardware; each build carries its model's name, so M-Vave's updater still takes the
unit's factory package afterwards.

## Controls

The manual still applies; the SMK-37's controls map onto the FM-1's like this. The screen keeps the FM-1's names
(HOME, SAVE, OCT-, OCT+, KNOB 1..4, ...).

| SMK-37 | PALEOTONE |
| --- | --- |
| Keys F3..G5 (the middle 27) | the FM-1's 27 keys, with velocity |
| Keys C3..E3 and G#5..C6 (the outer 10) | more notes for the selected track, as keys (its chord, the drone latch, velocity; a DRUM track's GM drums); with HOME > KEYS SPLIT, C3..E3 play the split track, with LAYER both tracks |
| 16 pads, STEPS (KNOB BANK unlit) | the 16 white keys F3..G5, with velocity, low notes on the bottom row; where the keys are a grid (the DRUM grid, NAME, the layers) steps 1-8 are the top row and 9-16 the bottom row. Lit in the theme colour |
| 16 pads, CALLS (KNOB BANK lit) | the herd on the pads: each pad plays the SCAPE CALLS track (HOME > SCAPE CALLS > TRK; the selected track while TRK is OFF) in its scale from its ROOT, the bottom row's left pad lowest, then along the bottom row and on along the top; with velocity, held while held, with the track's chord and drone latch. Lit in the track's colour, each octave's root brighter |
| KNOB BANK | the pads STEPS / CALLS (lit in CALLS) |
| K1 | MASTER (volume) |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS (SELECT is also ROAR's crank, PULS TURN) |
| K5..K8 | KNOB 1..4 |
| FX, SCALE, CHORD, NOTE REPEAT, PARA, GLOBE | FX, SCL, ENV, LFO, EDIT, GLO |
| PATCH / BT | HOME / SAVE (hold PATCH for the menu) |
| ARP / SEQ | ARP / SEQ |
| SEQ PLAY / SEQ REC | PLAY / REC |
| OCTAVE- / OCTAVE+ | OCT- / OCT+ |
| < / > | the previous / next page of the current page group; in the menu, the previous / next row; on HOME, > is HOME tapped (MIXER, CLOCK, KEYS, EPOCH, then SCAPE's seven pages) |
| FADER BANK | what the faders set (lit off bank A): **A** LEVEL of tracks 1..4, **B** REV send of tracks 1..4, **C** SCAPE WIND, WATER, RAIN, BUGS, **D** SCAPE FROGS, BIRDS, DRIPS, RUMBL, **E** SCAPE LEVEL, DRONE, DIST, TIME |
| Faders 1..4 | the four values of the current bank, 0..127 as their knobs; a fader takes over only once it moves (also after a bank change) |
| Pitch wheel | pitch bend of the selected track (its MIDI channel's bend range, 2 semitones unless set); also sent as MIDI |
| Mod wheel | the selected track's mod wheel (CC1, the MOD matrix's MODW source); also sent as MIDI |
| MENU > MIDI > KEY VELOCITY | DYNAMIC (as played, the default), SOFT, HARD, FIXED 64 / 100 / 127: the keys and the pads |
| OCTAVE- + OCTAVE+ held at power-on | HARDWARE CALIBRATION: teach the buttons and knobs again |
| OCTAVE- held at power-on | the USB RESCUE |
| OCTAVE- + OCTAVE+ held 5 s | UPDATE MODE |

NOTE NAMES names the outer keys' notes too. The SMK-37's TRS jack is a MIDI output and this build does not use it;
there is no TRS MIDI IN, so HOME > CLOCK > CLK TRS has nothing to follow. MIDI works over USB.

## Build

See [BUILDING.md](BUILDING.md) for the toolchain. GitHub builds all three packages on every push to the branch
`smk37` (`.github/workflows/smk37.yml`), checks the FM-1 build against its base byte for byte, and puts the
packages on the branch `smk37-output`.

```sh
BOARD=smk37 ./build.sh                 # build/paleotone-smk37.fwsc (SMK-37 Pro, Starrykey 37 Play)
BOARD=smk37 MODEL=elite ./build.sh     # build/paleotone-smk37-elite.fwsc
BOARD=smk37 MODEL=mke ./build.sh       # build/paleotone-smk37-mke.fwsc
./build.sh                             # the FM-1 build again before tests/run_tests.sh (the tests use its files)
```

## Install

The keyboard must run M-Vave's V016 firmware once before the first install (update V013 or V015 with M-Vave's
M-UPGRADE first). Then connect it by USB and run, with the package for your model:

```sh
py -3.12 tools/fm1_install.py paleotone-smk37-elite.fwsc
```

`fm1_install.py` needs `mido` and `python-rtmidi` (`py -3.12 -m pip install mido python-rtmidi`). It checks that
the package is for the connected keyboard. If an install stops, run it again: it resumes. OCTAVE- and OCTAVE+ held
for 5 s put the keyboard in update mode by hand.

## Going back to the stock firmware

Install M-Vave's firmware package for your model with M-Vave's updater, M-UPGRADE. It works directly, because
the installed package keeps the keyboard's model name.

## Safety

- Back up first: a full flash backup (the port kit's docs/02-update-and-recovery.md, section 3), and M-Vave's stock
  package for your model and M-UPGRADE at hand.
- Keep the USB cable connected until the install has finished and the keyboard has restarted.
- The first start clears the stock firmware's settings, and any other firmware's data, in the shared data area
  (PALEOTONE's own data family, `PALEOTN`).
- The firmware never writes the keyboard's calibration, Bluetooth pairing or key areas; the flash map refuses
  them at compile time and again in the flash driver.
- Installing other firmware is at your own risk. This build has not yet been tried on a real SMK-37.
