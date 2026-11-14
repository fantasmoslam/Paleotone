# NightClaw per-track envelopes

## Implemented behavior

From Home select a track, then tap SAVE for **Envelope**. SELECT chooses the row; K1–K4 edit Attack, Decay, Sustain and Release. Four rows remain visible at once, with scrolling for Calls and Atmosphere. ALGORITHM changes the selected row’s instrument. SAVE again opens that track’s sound page.

The page uses the accepted NightClaw font family, 24–32 px track names, 36 px indicators and saturated palette. Home controls and the accepted tablet keypad geometry are retained.

| Track | Envelope behavior |
|---|---|
| Drone | Native voices and imported instruments receive ADSR. Chord changes trigger the new bank/voices; outgoing notes release. |
| Sequence / Sequence 2 | Per-note ADSR shapes native and imported voices. Existing Decay controls still determine tone decay and sequencer note length; the new envelope does not replace timing. |
| Lead | Native oscillator, piano and imported voices receive amplitude ADSR. Imported FM6 retains its operator timbre envelopes underneath the editable amplitude envelope. |
| Calls | ADSR shapes each timed creature voice. Manual trigger/Auto and stop controls remain available. |
| Atmosphere | ADSR shapes the continuous bed with Play/rest as the gate, leaving individual natural events intact. |
| Skin | Keeps the existing percussion Decay control on its drum page. Struck drums retain their transient and natural body rather than behaving as held notes. |

Attack, Decay and Release use the imported synth’s logarithmic 1 ms–10 s table; Sustain is a level from 0–127. Short sampled and modeled sounds retain their intrinsic duration/energy decay: Sustain cannot extend a finite recording or indefinitely replenish a plucked string.

## Defaults, instrument changes and saving

Every stage initially inherits its instrument’s setting. No override is enabled until edited, preserving the approved factory sound. Turn a stage below zero to restore inheritance; the indicator then shows the effective voice value. Changing instruments keeps manual stages and updates inherited ones.

The original sound-page Attack/Release/Decay controls also feed their matching amplitude stages when edited, including the previously ineffective imported Lead Attack. They retain their tone/note-length jobs where applicable.

Overrides save per scene with GLO, survive voice and scene changes and reload after reboot. Scene reset clears only that scene’s overrides. Manual/Auto Calls remains a temporary performance choice.

Format-11 preset banks and parameter IDs remain unchanged. The original 28-byte globals header is a prefix of the extended 508-byte header; older saved beat/instrument/mix choices load with no envelope overrides. Envelopes use the existing A/B-protected globals object. Ten-slot banks remain within 3,840 bytes.

## Validation and limits

Host audio tests compare all four Lead stages on native Theremin, imported analog and imported FM6, plus attack response on Drone, both sequences, Calls and Atmosphere. They check finite output and silence after stopping. Persistence tests cover independent scenes, reboot, local reset, old 28-byte headers and invalid saved envelope bytes.

The physical-button simulator covers Home → Envelope → sound, literal detents, row selection, creature triggers and save/reboot. Shipped-WASM tests cover envelope editing, voice changes, reboot and physical panel messages at 44.1/48 kHz. Actual framebuffer renders are included in the guide.

Native build: image 551,296 / 581,564 bytes before the additional spoken-track label; RAM 96,468 / 98,304; pool 334,472 / 344,064. The subsequent build records the final image size. All 40 control scenarios pass. Approved default-audio fixtures remain regression targets, including serial, concurrent and worker-fallback output.

The candidate still needs physical listening and CPU/long-tail coverage before calling the new envelopes hardware-validated. The last installed/user-accepted firmware remains f9b11d6 unless a later installation is explicitly recorded. PR #41 remains open.

## Original gap corrected

Previously Drone Attack/Release and sequence Decay were scattered; Lead Attack shaped native audio but only the meter for imported voices. The imported synth’s internal ADSR had no public editing API. The implementation now exposes that API, handles engines with their own amplitude curves and adds native track envelopes without altering untouched recipes.
