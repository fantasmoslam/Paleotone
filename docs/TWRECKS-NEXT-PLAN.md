# NightClaw completion plan

NightClaw source f92fe8c is installed with landscape artwork and a minimum three-second static title screen. The device reconnected under its new name and resumed playback; Dylan reports that it is good so far. The preceding branding baseline passed full CI run 43092611149; the knob-response revision has its own regression checks. See [branding and timing details](NIGHTCLAW-BRANDING.md).

## Approved reference

The audio baseline 0f889bf retains the twenty approved audio baselines and runs independent taiko playback on a guarded second core. RAPTOR NIGHT with user-selected Theremin Lead is the signature reference. Dylan also explicitly approved BONE RIDER, DUST TRAIL, RED MESA, FOSSIL PASS, HIGH FRONTIER and NIGHT RIDER after installation. Preserve these sounds and the independence of beat and instrument choices.

## Completed

- Physically accepted progressive knob response, precise reversals and immediate movement away from zero.
- Large readable labels, four-column track controls and colored Pac-Man fill indicators; existing Performance controls retained.
- Twenty dinosynth scenes, with six authorized western replacements and fourteen retained saved slots. The original non-dinosynth preset bank is excluded from device builds.
- Eleven original Paleotone/Gurdy rhythms plus TRAIL, OUTRIDER and HUNT, independently selectable and saved per scene.
- Selectable Cello, Cello Low, Sub Pad and Fat Pad voices; existing Theremin and all prior instrument IDs retained.
- Eight manual creature calls; optional automatic mode; manual mode at boot.
- Saved instrument/beat choices, old format-11 header compatibility, scoped replacement revision and interrupted-save recovery tests.
- Native memory bounds, update-loader/recovery checks, twenty repeatable production audio hashes and ten legacy fixture hashes.
- Full CI checkpoint 6f85262 passed, including the long orphan scenario. Worker-mode tests prove all twenty hashes in serial, concurrent and partial-failure fallback execution.
- The live HIGH FRONTIER Lead test originally produced pops at 70–101% CPU. The installed two-core candidate repeats that performance cleanly at about 65% average / 91% peak, with zero reported dropouts or worker fallback after restart. Dylan confirmed it plays well.

## Current closeout

The installed knob-response revision f92fe8c is physically accepted: Dylan reports “Feels much better.” Preserve its progressive sweep and fine reverse corrections. The focused response/UI checks and native build pass; the remaining broad checks are being completed.

The full guarded-worker CI run passed: build, all audio-worker modes, storage/updater checks and all control scenarios including orphans. The published branding-baseline package SHA-256 matches its installation exactly; the later installed control revision has a separate checksum in the knob-response record. PR #41 is ready for review. The repository front page and [quick-start guide](TWRECKS-QUICKSTART.md) now describe NightClaw controls and its own candidate output; the upstream README is retained separately.

The fully CI-verified branding baseline is 3cf8a1a; current installed source is f92fe8c; the fixed output commit and checksum are recorded in the completion checklist. Wider long-session coverage remains useful for release confidence; do not describe the short hardware test as a worst-case benchmark. No further musical rewrite is required for the accepted scenes.

## Remaining coverage

The [72-minute host endurance pass](NIGHTCLAW-ENDURANCE.md) now covers all twenty production scenes and all four appended Lead voices. Its 24 selection and output-peak checks pass. Longer physical-device sessions and wider instrument combinations remain the next release-confidence work; no additional firmware installation is needed for these checks.

## References

- [Completion checklist](TWRECKS-COMPLETION-CHECKLIST.md)
- [Paleotone source review, worker adaptation and measured result](TWRECKS-AUDIO-HEADROOM.md)
- [Western scene mapping and migration](TWRECKS-WESTERN-SCENES.md)
- [Instrument additions](TWRECKS-INSTRUMENT-ADDITIONS.md)

## User requirements retained

Physical knobs do the editing; indicators show settings without simulated on-screen knobs or numeric parameter clutter. Direct saved instrument choices and dinosaur triggering remain accessible. Beats stay independent of melodic instruments. The sound target remains deep, consequential and apocalyptic, with grandiose low percussion, clear voices, restrained distortion and room for live Lead. A literal march is not required.
