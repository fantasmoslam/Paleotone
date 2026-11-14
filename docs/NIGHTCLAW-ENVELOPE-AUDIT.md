# NightClaw envelope controls: current gap and next work

The existing firmware has partial envelope controls, not a unified per-track ADSR editor. This remains unfinished sound-editing work; the earlier feature-complete assessment was too broad.

## Verified source behavior

| Layer | Exposed controls and actual behavior |
|---|---|
| Drone | Attack (P_CATT) and Release (P_DREL), on different rows/pages. They drive the native swell and an outer envelope around imported Drone voices. No user-editable decay/sustain pair completes ADSR. |
| Sequence | Decay (P_DECAY). It sets the native pluck amplitude/filter decay; for imported voices it determines time until note-off. The imported voice retains its own amplitude envelope. |
| Lead | Attack (P_LATT) controls the native Lead envelope. In the imported-voice branch, ld_att changes the meter envelope, while audio comes directly from pt_render. The exposed attack therefore does not shape imported Lead audio. Native Lead release is fixed in that path. |
| Sequence 2 | Decay (P_S2DEC), with the same native-decay/imported-note-gate distinction as Sequence. |
| Skin | Decay is passed into the taiko/skin models. These struck sounds are not currently a held-note ADSR track. |
| Atmosphere / Calls | No common editable ADSR panel. Creature voices have internal voice envelopes and note/gate behavior. |

Evidence: firmware/src/app/ui.c VIEW_KNOB; firmware/src/dsp/amb.c parameter handling, pt_pluck, render_plucks, render_drone and render_lead; firmware/src/pt/pt.h and pt.c.

The imported Paleotone implementation already contains P_ATK/P_DEC/P_SUS/P_REL and env_tick in firmware/src/pt/voice.c, but NightClaw's public pt.h wrapper does not expose an envelope-edit API. pt_sound loads the engine preset and its internal envelope. Some engines, including FM6, use their own amplitude envelopes; their behavior needs explicit treatment rather than assuming one outer ADSR implementation covers all voices identically.

## Next development priority

1. Provide one Envelope page for the four melodic tracks: Drone, Sequence, Lead, Sequence 2. Four columns are Attack, Decay, Sustain, Release; SELECT chooses the row and K1–K4 edit it. Use the existing sound-page navigation pattern; preserve accepted Home controls, fonts, colors and keypad layout.
2. Make each control affect audible output for both native and imported instruments. Separate amplitude-envelope stages from existing tone/filter-decay controls and sequencer note length. Test key release, retrigger, legato, sustained Drone chords, automatic phrases and voice stealing.
3. Preserve the current preset/voice envelopes until edited. Define how a deliberate track override behaves when changing instruments, and save overrides with the scene. Existing approved scene sounds and saved voice/beat choices must survive the migration.
4. Add suitable envelope/tail editing for Skin, Atmosphere and Calls according to each sound model. Keep percussion's natural transient/body rather than silently changing it into a held synth voice.
5. Check parameter-code capacity (the current special-knob IDs start at 200), saved-object size/versioning, RAM/flash fit and CPU before device installation. Verify that longer release tails do not create excessive voice load.
6. Cover native/Paleotone envelope response, save/reboot, unchanged default scene audio, firmware render/fit and browser parity. Then use short listening previews and device testing to validate the sound.

This audit adds work to the docket. It does not change DSP, instrument presets, saved settings or installed firmware.

## Project checkpoint alongside this work

The installed FM-1 remains the accepted f9b11d6 candidate. Approved music, twenty scenes, fourteen independent beats, manual calls, appended cello/pad voices, literal encoder response and readable firmware UI are retained. GitHub Pages hosts the physical-panel player, separate visual guide and tablet mode. Dylan accepted the restored two-row keypad at source 1398257; keep its geometry and uniform scaling.

The current browser CI passed for source 1398257 (run 43537466329). PR #41 remains open and unmerged. Longer physical-device endurance, wider voice combinations, page readability feedback and final release review remain open alongside this envelope work.
