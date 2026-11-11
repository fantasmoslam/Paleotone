# Six western scene replacements

Dylan authorized replacing six named scene presets with western scenes using the RAPTOR NIGHT drum direction. The bank remains twenty entries; RAPTOR NIGHT and the other thirteen untouched slots retain their recipes and saved settings. Instruments remain freely selectable, independently of the beat.

| Slot | Replaces | New scene | BPM | Default beat |
|---|---|---|---|---|
| 4 | ASHEN SKY | BONE RIDER | 124 | TRAIL |
| 6 | FERN SHADOWS | DUST TRAIL | 112 | TRES |
| 9 | AMBER CAVERN | RED MESA | 128 | OUTRIDER |
| 13 | CRESTED MOON | FOSSIL PASS | 104 | WAR2 |
| 16 | HOLLOW NEST | HIGH FRONTIER | 116 | GALOP |
| 18 | ECHOING RUINS | NIGHT RIDER | 136 | HUNT |

The new defaults bring the low drum ensemble forward, reduce echo/reverb masking, and leave automatic lead generation off. Keyboard Lead remains available. Some defaults select the existing Theremin, others the new cello voices; these are editable defaults, not melodies attached to drum playback. Low bass pads and cello support provide different starting points.

The original eleven imported Paleotone/Gurdy beats retain their IDs and event logic. TRAIL, OUTRIDER and HUNT are three appended T-WRECKS variations using the same taiko voices: galloping accents, displaced low/mid answering figures and a denser ensemble turnaround. All fourteen are available with ALGORITHM on Skin and save per preset.

## Updating existing saved presets

A revision byte uses previously zero-filled padding in the 28-byte globals object. Preset format 11, slot sizes and flash object addresses are unchanged. On the first load of an earlier bank revision, only the six authorized slots receive the new recipes and their default beats. The updated banks and revision are committed so later edits to the new scenes survive reboot. The other fourteen saved slots, including RAPTOR NIGHT, are copied intact. The same global sixteen-byte legacy header remains readable. Missing/corrupt bank recovery retains the existing behavior.

No six-preset manual reset is required. Holding GLO until RESET on one new scene later restores that scene's correct default beat as well as its instrument settings.

## Listening and CPU status

Dylan described the installed instrument candidate as really good so far. Earlier crackle was reported through the speaker; headphone comparison has not been recorded, so its cause is unconfirmed. Reported CPU was 55–75%; a read-only screen-reader snapshot of RAPTOR NIGHT at 130 BPM showed 60% average, 83% peak and no reported dropouts. That snapshot is not a worst-case load test or acceptance of these new scenes.

## Engineering validation

All fourteen beat renders pass finite-audio/overlap/panic checks; the original eleven still pass 22,528 source-event comparisons. All twenty scene renders pass audibility/headroom checks. The six new scenes measured RMS 0.044–0.071 and peaks 0.248–0.349 in thirty-second native host renders at the tested master gain. RAPTOR NIGHT's sixty-second audio hash remains 990aad3c.

Exactly six factory recipe rows changed. The other fourteen saved slot objects are verified byte for byte through the revision update, with independently saved beat selections, a simulated interrupted bank write and retry, and subsequent new-scene edits surviving another reboot. Twenty new sequential production hashes repeat; ten legacy fixture hashes remain unchanged. Other production hashes after the replacement slots can change because the imported synth's random state carries across sequential test renders.

Thirty-two unaffected control scenarios passed before the selector-limit correction; the five affected production scenarios then passed with the final selector, including western defaults, saved beat selection, new instrument choices and manual calls. The long orphan scenario was not rerun.

Final image is 578,684 / 581,564 bytes; static RAM 91,972 / 98,304; pool 333,448 / 344,064. Candidate 43703d8 is now installed; listening acceptance of the new scenes is pending.

The final package passed OTA entry, loader boundary/recovery and installer protocol checks. The FM-1 subsequently reconnected and candidate 43703d8 was installed successfully.

## Verified device installation

The installer completed its write and restart verification on 2026-11-11. An independent identity query confirmed FM-1_942 running. Read-only screen-reader snapshots immediately before and after installation both showed RAPTOR NIGHT, F Phrygian, 130 BPM, Drone Level 59 / Filter -18 / LFO 20 / Reverb 50, with no unsaved-edit indication. This confirms those observed settings survived the update; the all-fourteen-slot byte comparison is a host migration test. New-scene sound and worst-case device CPU remain listening/performance checks.
