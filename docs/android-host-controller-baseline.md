# Android host controller baseline

Implementation evidence collected after restoring the original Android interface. No controller installation or physical qualification is claimed here.

## Reused code and concrete behavior

| Component | Source pin | Finding |
| --- | --- | --- |
| Android host | Quixotic7/RemoteFM1Android `3e3ea79b729da53c11196513e0ebb9a56ef42ab5` | `ControllerInput.Selection.of(null)` defaults to All. `ControllerInput.scan` excludes every `isFm1` candidate from ordinary MIDI input, independently of whether a bridge session owns that device. |
| Android bridge input | Same Android pin | `RemoteLink::runOnce` registers `cbMidiIn`; `cbMidiIn` splits/queues channel messages; `RFMEngine::between` drains them to the active core through `toFirmware` while attached. MIDI_IN forwarding already exists. Do not implement it again based on an older README statement. |
| Candidate identification | Same Android pin | USB VID/PID 1209:0001 is treated as an FM-1 bridge candidate; names are used when IDs are unavailable. Actual enumeration is required before assuming an SMK or a non-bridge FM-1 will enter the ordinary controller path. |
| SMK bridge reference | Quixotic7/RemoteFM1 `ed06aa502c7b75a3414c9c1976d0a5abd4350f57`, `docs/SMK37.md` and `firmware/patches/smk37/` | Unqualified on physical SMK hardware in that source. Presents a legacy 27-key FM-1 panel. Outer ten keys, pitch/mod wheels, pedal and bank buttons use MIDI_IN; faders become relative knob deltas. The doc's statement that Android drops MIDI_IN conflicts with the actual pinned Android implementation above. |
| Owned SMK board reference | fantasmoslam/fmini-1 `smk37`, `4d639fe010a4c97a755258c88ccc4cdab78f755f` | Contains an Elite hardware test record, model packaging, protected storage/calibration boundaries, four-fader target-crossing pickup and SMK controls. Pro and MKE-P37 remain untested in that record. Reconfirm the current physical unit and installed firmware. |

## Next hardware observation

Record the exact FM-1/SMK models, currently installed firmware and Android-reported MIDI identity/ports. Test ordinary USB MIDI where the current identity/routing permits it, without requiring a firmware change merely for identification. Distinguish enumeration, ordinary note/control input and a negotiated screen/LED bridge session.

## Backend work indicated by the source

- Establish exclusive transport ownership. A device name or USB identity alone is not proof of a live bridge, and ordinary MIDI and the bridge must not open competing ports.
- Reuse the existing session and screen/LED transport where suitable. Build this project's controller firmware from reviewed board code; do not assume the owner already installed another application's firmware.
- Preserve separate FM-1 and SMK capability profiles. A working legacy 27-key session does not qualify all 37 SMK keys or the additional controls.
- Retain absolute fader identity and pickup policy where possible instead of silently reducing every physical fader to a relative encoder.
- Verify note/button/pedal release, queue overflow, attach/detach and reconnect before qualifying either controller.
- Preserve the owned port's model/package checks and calibration/storage boundaries. No flash or recovery procedure has been executed as part of this inspection.

Interface design work is paused. The active Android shell is the original interface, with the held-key preset fix retained in Paleotone.

## 0.1.5 MIDI routing correction

The supplied Tab S6 diagnostic screenshot identifies the current controller as
**Hügelton Instruments RAPTOR SMK**, USB **1209:0001**. Android enumerates it;
`linked` meant a device handed to the native backend, not a negotiated bridge
session. In **This tablet** mode, the bridge's eager claim combined with the
ordinary MIDI input's unconditional exclusion prevented controller notes.

The host now uses the selected source to assign exclusive MIDI port ownership:

- This tablet: keep device discovery, release native bridge ports, then admit
  candidate devices to ordinary MIDI input (subject to the saved input selection).
- FM-1: reserve candidates and release ordinary MIDI ports before allowing native
  bridge registration. Existing candidate identification and OMX exclusions stay
  intact.
- Serialize rapid mode changes and reject late native-open callbacks after release
  or physical removal. Keep Bluetooth transport discovery available while native
  bridge ports are released.

Five JVM regression cases check startup in both modes, ordered transfers in both
directions, repeated selections and rapid source changes. The patch is restored
and reapplied deterministically to the pinned runtime on every Android build.
Actual SMK notes, reconnect and mode-switch behavior still require physical testing.
This does not qualify SMK screen/LED feedback or every additional physical control.
