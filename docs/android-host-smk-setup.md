# Instrument Host SMK integration — 0.2.0 candidate

The tablet runs Paleotone or Felucca and generates the sound. The keyboard runs a
stable USB controller bridge, installed once, which sends controls and displays
the selected instrument's original 240×240 screen and authoritative lights.
Switching instruments never flashes the keyboard.

## Current qualification

The owner confirmed ordinary RAPTOR SMK USB MIDI detection and note playback in
0.1.5 on the Tab S6. The new paired controller firmware has not been qualified on
physical hardware. Simulator/automated results do not establish LCD orientation,
encoder order, wheel calibration, power behavior or end-to-end latency.

## One-time setup

1. Record the exact model printed on the keyboard: **SMK-37 Pro**, **SMK-37 Elite**
   or **MKE-P37**. Each package is model-specific. Do not choose by USB product name.
2. Export the current custom instrument's sounds/settings using its own backup
   method, and retain its exact known-good firmware package and restore procedure.
   This controller installation replaces the currently running custom firmware.
3. Retain the supplied BUILD.json/package hashes. The controller does not write
   preset data, the stock mapping marker, the ownership tag, calibration or pairing
   during normal operation. The bootloader's app update and staging regions are
   necessarily written during installation. This does not guarantee another
   firmware's importer or installer preserves every setting.
4. Use the supplied model-aware `tools/fm1_install.py` with `--help` and `--info`
   first. Confirm its reported model before giving it that model's .fwsc package.
   Do not use a raw-disk FM-1 procedure on an SMK. A physical install has not been
   executed or approved automatically by the app.
5. Install the Android 0.2.0 update over 0.1.5. Connect the SMK to the tablet with a
   data cable. Android may ask permission. The product `Instrument Host SMK`
   automatically selects the hardware bridge. A negotiated capability identifies
   SMK profile v1. The screen should show **SMK · linked**, followed by the same
   instrument display on tablet and keyboard. Audio output remains your tablet's
   selected route; USB audio is an optional, separately selected setting.

Ordinary custom firmware remains usable as a MIDI keyboard, with the SMK tablet
panel selected by its reported SMK name. Screen and LED feedback require the
paired bridge. A shared USB VID/PID alone never proves a bridge handshake.

## Controls

| Physical control | Host behavior |
| --- | --- |
| 37 keys C3–C6 | Middle 27 original panel keys; ten outer notes through MIDI_IN, with velocity. |
| 16 pads, two rows of eight | Original instrument's 16 white panel keys and corresponding LED levels; a pad and its physical middle key share hold ownership. |
| K1 | Master level. |
| K2 / K3 / K4 | Select / Algorithm / Presets. |
| K5–K8 | Instrument KN1–KN4. |
| Four faders, default bank | KN1–KN4 via relative takeover, two MIDI units per detent. Absolute positions retained on the wire; first reading establishes a baseline. |
| FADER BANK | Toggle between KN1–KN4 and Master / Select / Algorithm / Presets; rearm fader baselines. |
| KNOB BANK | Toggle normal/fine sensitivity for KN1–KN4; fine uses two detents per step. |
| Pitch / mod / pedal | Pitch bend / CC1 / CC64 to the selected instrument. Its synthesis determines response. |
| Top six buttons | Physical ARP / NOTE REPEAT / SCALE / CHORD / GLOBE / BT → FX / SEL / ENV / LFO / EDIT / GLO. |
| Bottom six buttons | PATCH / PARA / FX / SEQ / SEQ PLAY / SEQ REC → Home / Save / Arp / Seq / Play / Rec. |
| Octave − / + | Original octave buttons. |
| < / > | Select one step. |

This preserves each complete firmware's controls. Physical SCALE/CHORD labels do
not force unrelated functions into a firmware that assigns that position another
role; the tablet shows both printed and active labels. Pads show mapped white-key
lights; there are no invented black-key LEDs or key aftertouch sensors. Pad
pressure is not exposed by this profile. The first controller build uses USB;
Bluetooth controller firmware is not part of this candidate.

Faders use explicit relative takeover because these original applications do not
expose a universal current parameter for every screen. We do not claim absolute
parameter pickup for arbitrary firmware pages. A bank or instrument switch resets
baselines. Screen, button and pad feedback use the existing diffed transport.

## Before calling it hardware-qualified

Test both instruments, all 37 keys including releases/velocity, pad/key overlap,
all knobs/faders/banks/buttons, wheels and sustain. Compare both screens and the
pad/button LEDs. Unplug while holding an outer key and pedal; reconnect with no
stuck note or stale bend/pedal. Repeat instrument switching and preset recalls,
then confirm saved tablet state and the retained controller firmware recovery
route. Record exact model, package hash, APK version and cable/audio setup.

## Sources

Project adaptations and build instructions live on `codex/android-instrument-host`
in fantasmoslam/Paleotone. Protocol/platform baseline: Quixotic7/RemoteFM1
`ed06aa502c7b75a3414c9c1976d0a5abd4350f57`, pinned fm1-base and felucca-bsp
submodules, preserving their license notices. Wire HELLO retains the legacy
RemoteFM1 protocol identifier for interoperability; USB product, project build,
and explicit SMK profile identify this controller. No claim of original authorship
is made for reused board, transport or decoder code.
