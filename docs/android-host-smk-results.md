# Paired SMK integration — 0.2.0 software results

Build source: `c4e66e653108be737e95ebc1998949a341e179c0` on
`codex/android-instrument-host`. Successful Actions run:
https://github.com/fantasmoslam/Paleotone/actions/runs/42455112789

## Scope and evidence

The owner confirmed 0.1.5 enumerates the RAPTOR SMK and plays the tablet-selected
instrument on the Tab S6. His previously confirmed physical model is SMK-37 Elite.
This is an ordinary MIDI success, not a screen/LED bridge qualification.

The 0.2.0 paired candidate adds an automatically selected SMK tablet panel with
37-key geometry, original instrument screen, 16 pads, eight knobs, four faders,
printed/active button labels, wheels and pedal. The original FM-1 panel remains
available for the FM-1. The paired controller exposes its SMK capability, absolute
fader identity, bank controls and host-authoritative pad/button/bank feedback.

The tablet always runs the instrument. The source selector assigns controller
transport ownership; audio output remains separately selectable. No instrument
selection invokes a keyboard firmware update. Installing the bridge once replaces
the keyboard's currently running custom instrument firmware.

## Automated checks

- Four native host tests passed, including the unchanged two-instrument/audio
  comparisons and the new SMK adapter checks. Adapter tests cover fader takeover,
  bank edge detection, bank rearming, master limits, fine knob accumulation,
  outer-key/pedal/bend release, and physical/touch outer-key ownership.
- JVM: **285 passed, 1 skipped**, no failures or errors. Includes all 37-key hit
  centers, black-key overlap/bounds, pad mapping and five port-ownership cases.
- Fifteen real-board simulator scenarios passed in CI: five for each Pro, Elite
  and MKE build. They cover key/pad overlap, control mapping, MIDI_IN bytes,
  screen-session transport, original pad/button LEDs, the new bank lights,
  session-end light reset, and preservation of seeded foreign save bytes and the
  stock mapping marker through boot and attempted saving.
- Additional local simulator fixture `docs/fixtures/smk-all-keys.txt` passed for
  all three models. It individually exercises **37 physical keys and 16 pads**,
  checking velocity and release packets, then empty held-key state. This fixture
  is additional to the CI's 15 scenarios, not included in that count.
- Six more local simulator regressions passed: the inherited `session.txt` and
  `hold.txt` for all three models, changing only the expected capability mask to
  include SMK bit 11. These verify every tile encoding, shadow/screen equality,
  frame ACK/error handling, multi-message frames, HOLD timeout/flush behavior,
  keepalive loss and USB reset/unplug cleanup. Fixtures are retained under
  `docs/fixtures/`. Together with all-keys, these are nine additional local
  scenarios, separate from the fifteen CI scenarios.
- The inherited standalone preset assertion was adapted to INIT VOICE (0) because
  this build distributes no Yamaha ROM cartridges. It does not claim a ROM-bank
  audition passed. Standalone synthesis is not this controller's product role.
- Model-specific target builds and their size checks passed. Elite image 155,756
  bytes, static RAM 44,652/98,304 bytes, pool 310,632/344,064 bytes. These are static
  build budgets, not runtime stack/latency/thermal measurements.
- APK v2 signature verified by the build; its certificate also matches accepted
  0.1.5. Existing app ID and data/update identity retained. Both actual instrument
  libraries and native host are present, with the SMK UI and ownership classes.
- Shipped Elite package checked again with the shipped installer: correct package
  CRC, loader marker, Elite identity and flash-image app boundary (643,072 bytes).
  All normal and interrupted-update identity model guards were tested. The
  installer now refuses Pro/Elite/MKE cross-model writes.

## Build hashes

| Deliverable | SHA-256 |
| --- | --- |
| Android `Instrument-Host-0.2.0.apk` | `33b7581c87ac4801da0f649ecfb6f29bca75f539f5d5853dc4e974cb6e9a251a` |
| Elite `.fwsc`, identity `SMK-37 Elite_53000200` | `a1950e5db524cb896156ec38912c0d92ccecc3451d20ab8e9a619c8c31c8f3e4` |
| Pro `.fwsc` | `ad4e52e916ecf3809cd8044218fb4ef77e91064f47a8700919b4a6c668efc0cb` |
| MKE `.fwsc` | `74c43ee7ba03999b5b7f52364b4f5fc196a730b1ee55e3122834b6dd1e83c4e9` |

## Profile v1 wire contract

Retains protocol-v2 HELLO and diffed screen/LED transport, with the legacy
`RemoteFM1` wire identifier for interoperability. USB product is
`Instrument Host SMK`; HELLO caps bit 11 explicitly identifies this profile.
A USB identity alone is not a negotiated session.

- MIDI_IN `BF 14..17 value` (hex CC 20..23, channel 16) carries four absolute fader
  values. First value is a baseline; host relative takeover uses two units per
  step. No screen-pixel parameter inference or universal absolute pickup claimed.
- MIDI_IN `B0 6E value` / `B0 6F value` carries knob/fader bank press/release;
  toggle only at a new press edge. Banks are normal/fine KN1–4 and parameter/main
  fader targets. Main faders map Master, Select, Algorithm, Presets.
- LEDS flags bit 1 is knob-fine and bit 2 fader-main; only profile-v1 peers use
  them. The host sends authoritative states, and firmware clears on session end.
- Ten outer notes, bend, modulation and pedal remain whole MIDI messages. The
  native host adds physical/touch outer-note ownership and releases held outer
  notes, sustain, bend and modulation on session loss. MIDI_IN queue generations
  reject messages from a released link session.
- Middle keys/pads, original buttons/encoders, TOUCH velocity and original LED
  classes retain the reviewed protocol/platform paths.

## Data and hardware boundaries

Normal bridge operation disables instrument-store writes and ownership claiming;
seeded stock mapping and foreign instrument data stay intact in simulation.
Installation still writes the bootloader's app/staging regions. A recoverable
backup of the current RAPTOR instrument and its exact firmware remains necessary
before physical installation; this is not a verified backup/restore of that unit.

**No physical bridge installation or full-experience qualification is claimed.**
Remaining tests: LCD orientation/refresh, actual encoder order/direction, fader
feel, every physical key/pad/button, LEDs, wheel and pedal behavior, USB/audio/power
routing, disconnect while held, instrument/preset switching, saved state and real
latency/stability. Pad pressure and black-key LEDs are not represented by this
profile; SMK keys have no aftertouch sensor. This first controller build uses USB.

See `docs/android-host-smk-setup.md` for setup and mappings. Controller baseline
and attribution are pinned there. Keep broader M3 state/queue hardening and release
qualification separate from this first paired hardware trial.
