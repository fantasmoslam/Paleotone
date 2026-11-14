# Selectable cello and bass pad voices

Four instruments are appended to each melodic track's voice selector:

| Voice | Character |
|---|---|
| Cello | Articulated bowed synth cello with harmonic excitation, restrained bow noise and fixed body resonances |
| Cello Low | Darker bowed tone with a gentler onset and longer release |
| Sub Pad | Clean sustained sine-based low pad with slight detuning |
| Fat Pad | Sustained filtered saw and sub-octave square, with restrained resonance and no added drive |

These are synthesized instruments, not acoustic cello recordings. On Home, SELECT a melodic track and turn ALGORITHM to the last four entries. Save normally. No factory reset is needed. Drum beat selection is independent; the existing Theremin and RAPTOR NIGHT factory recipe are unchanged.

The Drone's new cello instruments sound one octave below its composer notes; the new bass pads sound two octaves below. Keyboard Lead and both sequences retain their written note/register controls. All original voice IDs remain intact, including Sequence 2's original twelve bass voices and following Sequence voices. Save format and parameter count are unchanged.

## Verification

- All 116 registered melodic voices loaded and sounded in the voice-list check.
- The four additions passed 72 sustained register checks across all melodic tracks, with finite audio, bounded single-note peaks, key release and panic silence.
- Native diagnostic renders at C2 put the cellos' dominant component at 65.5 Hz. Sub Pad at C1 measured 32.5 Hz; Fat Pad also contains the octave below its played note. These are spectral measurements, not listening acceptance.
- Twenty production audio baselines, including RAPTOR NIGHT, match unchanged. Ten host-only legacy baselines match unchanged.
- Saved-data recovery and four relevant control scenarios pass, including new instrument selection, saving/rebooting and independently saved beat selection.
- Hardware image 578,140 / 581,564 bytes, static RAM 91,972 / 98,304, pool 333,448 / 344,064. OTA, loader and installer checks pass.

## Installation status

Candidate 54fa451 was installed successfully on 2026-11-11 after the FM-1 reconnected. The installer completed its write and restart verification; an independent --info check then reported FM-1_942 running on T-WRECKS FM-1 1. Actual FM-1 CPU, dropout behavior and listening acceptance of the new voices remain pending.
