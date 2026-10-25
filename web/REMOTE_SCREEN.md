# PALEOTONE USB screen companion (development)

This companion runs PALEOTONE's existing C drawing functions in WebAssembly.
It does not start an AudioContext or a second sequencer. The FM-1 owns sound,
navigation, parameters, held buttons and transport; the page follows snapshots.

## Build and desktop test

Use the **build firmware** and **remote screen** Actions runs with the SAME
commit on `feature/fm1-physical-screen-focus`.

1. Download `paleotone-firmware` from the successful firmware run. Install
   its real `.fwsc` with the existing FM-1 installer.
2. Download `paleotone-screen-companion` from the successful remote screen run.
   Keep all files from `build/emu` together, including `screen.html`,
   `remote_screen.js`, `second_screen.js`, `control_target.js`,
   `physical_focus.js`, `remote_views.json` and `felucca.wasm`.
3. In that folder run `python -m http.server 8000 --bind 127.0.0.1`.
4. Open `http://localhost:8000/screen.html` in the desktop browser used for
   the earlier successful MIDI test. Click **Connect FM-1**, permit SysEx,
   and select the FM-1 ports if requested.
5. Compare HOME, ENV, EDIT, SEQ, MENU and a confirmation dialog against the
   device. Test physical knobs and buttons. Check that notes/audio and transport continue normally.
6. Switch to **Expanded**. Use **Second screen** to select the overview or
   a live Home, Mixer, Sequencer, Envelope, LFO, FX, Arpeggiator, Scale, Clock,
   or Soundscape page. Choose Follow FM-1 or a particular track.
   Use the FM-1 itself to choose which side its knobs control: **hold HOME, tap OCT− for LEFT or OCT+ for RIGHT, then release both**.
   The inactive panel retains its page and track while parameter values remain live. Choosing RIGHT from the overview opens Mixer.
   The shortcut is active only while this matching companion is polling in Expanded mode. It is ignored in menus, dialogs, naming and held layers.
   Ordinary HOME and octave behaviour is preserved when no companion is connected. Once a shortcut starts, release every chord button before using it again.
   This requires the matching firmware from this development branch; it is not a browser-only update.
7. Disconnect USB during a focus shortcut, then release HOME and OCT. Neither release should change the octave or open HOME. After three seconds without the companion, normal HOME and octave controls resume.

Opening the HTML with a file:// URL is insufficient for loading the WASM
module. A phone needs a browser exposing USB Web MIDI with SysEx and an HTTPS
host for this bundle. Phone hardware/browser validation is outstanding.
No automatically published website is part of this development branch.

## Protocol

All messages use `F0 7D 46 4C command ... F7`. Unsigned 14-bit integers are
low seven bits first. Renderer IDs are unsigned 32-bit values in five 7-bit
bytes, low first. These are different from existing signed parameter values.

- **92 REMOTE**: `[0]` queries; `[1]` freezes a new display projection;
  `[2, revision14]` acknowledges the frozen projection.
  Reply: `op, status, schema, rendererID32, revision14, base14, length14`,
  then 16 dirty-byte masks, each encoded in two 7-bit bytes (46 payload bytes).
- **93 BLOCK**: `[revision14, block14]`. Reply:
  `revision14, block14, status, rawLength14, packedData`.
  Blocks contain at most 128 raw bytes, packed in groups of mask + seven bytes.
  Every complete block reply fits the existing 64-packet USB SysEx queue.
- **94 INPUT**: `[0, buttonLabels14]` sets held panel buttons;
  `[1, encoderRole, 64 + delta]` turns an encoder (-8..+8);
  `[2]` releases remote controls; `[3]` renews a hold lease.
  Reply: `operation, status`. Encoder commands are never automatically retried.

The browser reuses blocks only when its last acknowledged revision equals
the device's base. A lost acknowledgement forces a full reload. Source IDs
prevent a renderer from silently accepting a different firmware build.
Only outgoing projection data is exposed: the hardware never imports a raw
browser snapshot. Simultaneous screen clients are unsupported; close other
screen tabs before connecting.

The state projection is field encoded, not a copy of runtime structs or
pointers. Current parameters, steps, FM6 patches, user preset records, project
summaries, names, menu/dialog selection, FX controls, sample display extrema
and live scope samples feed the original renderer.

## Validation and remaining work

Automated checks cover snapshot bounds, state round trips, representative
static HOME/ENV/SEQ/MENU/dialog pixels, native-to-WASM pixel equality, expanded
view preservation, protocol packing and remote button expiry. The original
editor regression also runs.

This is a development build, not a claim of complete hardware-tested 1:1
coverage. Polling cadence can change animation timing; live telemetry is
sampled, not a frame-locked video feed. Custom SLICE source/marker metadata
and all engine-specific navigation cases still need coverage. Input comes from the FM-1 panel; the companion displays the inactive page alongside it. Expanded is a companion dashboard, not a reflow of every
original screen. Hardware audio performance, reconnection and phone operation
must be tested on the real instrument before a release.

- **96 PANEL_FOCUS**: `[enabled]` (0 Original/hidden, 1 Expanded) renews a three-second gesture-reporting lease. Reply: `1, side, serial14, held`. HOME + OCT− / OCT+ emits an absolute left/right event. The browser waits for `held=0`, deduplicates the serial, and uses the existing serialized panel-navigation path. This adds 16 bytes of device state and no display buffers. Hardware validation is still required; do not treat a passing build as a tested image.
