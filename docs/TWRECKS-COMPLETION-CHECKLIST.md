# NightClaw completion checkpoint

Musically approved source: 43703d8. Worker audio baseline: 0f889bf. Current installed NightClaw candidate: 0cb944a, retaining all twenty approved audio hashes.

| Item | Evidence / status |
|---|---|
| Musical direction | RAPTOR NIGHT and all six western scenes explicitly approved on the device |
| Control UI | Large labels and colored fill indicators; independent saved instrument/beat selectors |
| Instruments | Cello, Cello Low, Sub Pad and Fat Pad available on each melodic layer; 116 registered voices checked |
| Drums | Eleven exact imported beat families plus three independent western variations; fourteen finite render checks |
| Creature calls | Manual by default; optional Auto; key routing and stop/mute behavior covered by scenarios |
| Preset bank | Twenty scenes; six authorized replacements; fourteen retained saved slots verified byte for byte |
| Storage | Beat/instrument save and reboot checks, interrupted replacement write/retry and old-header compatibility pass |
| Audio regression | Twenty production baselines repeat; RAPTOR NIGHT baseline preserved; ten legacy fixture baselines unchanged |
| Device installation | Write/restart verified; RAPTOR NIGHT observed at the same 130 BPM, F Phrygian and Drone controls before/after |
| Resource fit | Current NightClaw image 543,932 / 581,564 bytes; RAM 95,644 / 98,304; pool 334,472 / 344,064 |
| Updater | Entry, loader bounds/recovery and installer protocol checks pass |
| Full CI audit | Guarded-worker source 0f889bf passed the complete CI run, including concurrent/fallback audio checks and the long orphan scenario |
| Corrected release check | The two voices were released FM bells and died naturally within one additional second; a bounded ten-second window passes, without changing audio code |

## Hardware overload follow-up

Lead over HIGH FRONTIER reproduced pops and 70–101% CPU. Candidate 0f889bf adapts Paleotone's guarded second-core worker for independent taiko playback. It installed successfully and reports two active cores. Dylan repeated the test and reported clean playback with about 65% average / 91% peak. Three read-only snapshots showed 63–68% average / 81–90% peak and zero reported dropouts or worker fallback after restart.

All twenty production audio hashes match under serial, concurrent and injected partial-worker-failure execution. Resource fit is now image 580,492 / 581,564, RAM 95,708 / 98,304 and pool 334,472 / 344,064. See [source review and measured result](TWRECKS-AUDIO-HEADROOM.md).

Long-session and wider instrument combinations remain broader release coverage. The original overload reproduction now plays cleanly; further musical rewrites of approved scenes are not planned.

Installed worker package SHA-256: 4d6336a0e7657f9943986694a7e40964fc5b83600cb4a237777af65dd9feb015

## Verified CI publication

[Worker CI run 43076204934](https://github.com/fantasmoslam/Paleotone/actions/runs/43076204934) completed successfully. Published output commit 249b5f999308d8e0f5388132d696cf9b07afc8fa identifies source 0f889bf99dde15656d211f0064fe7ffa43a3cffd with success. Its package SHA-256 exactly matches the locally installed worker package above. The output log reports audio-worker and scenario-orphans passed, all tests passed, exit 0.

## Installed NightClaw publication

[Full CI run 43092611149](https://github.com/fantasmoslam/Paleotone/actions/runs/43092611149) passed for source 3cf8a1a4c9e8feedfc14ceebdf7578fb4481ef34. Fixed output commit 77f1b86224ea55fd076eb4df496e70d455b234c9 identifies that source as successful. Its nightclaw-FM-1_942.fwsc SHA-256 matches the installed package exactly: beb6ce14e4b50ef44270442c312e750abb8d6aa49f02ed1c6a72cdb59671586e.

The installer completed and the device reconnected as NightClaw FM-1, answered the screen-reader handshake as NightClaw DEV, and resumed HIGH FRONTIER at 116 BPM. Dylan reports that it is good so far. The title hold is at least three seconds after the completed LCD transfer; audio and USB servicing continue.

## Extended playback

A [72-minute simulated playback pass](NIGHTCLAW-ENDURANCE.md) across all twenty scenes and the four appended Lead instruments passed all 24 selection/output-peak checks. This extends host coverage; long physical-device sessions remain separate.

## Knob response accepted

Installed source f92fe8c replaces abrupt acceleration and centre dead clicks with a progressive response. Dylan reports “Feels much better.” Focused response tests, all 38 regular control scenarios and the 91-knob exhaustive audit pass after the Energy correction and host audit fix. Audio, worker, storage and updater checks pass; GitHub CI remains pending. See [response and installation record](NIGHTCLAW-KNOB-RESPONSE.md).
