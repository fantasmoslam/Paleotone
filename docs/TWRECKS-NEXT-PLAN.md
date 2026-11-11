# T-WRECKS — completion plan

Status: controls and manual calls are implemented, but the musical candidate was rejected. Dylan describes hollow/insufficiently deep bass, upbeat/noir tonality and unsuitable instruments. **“Apocalyptic” and “consequential” are the current musical acceptance criteria.** The [audio study](TWRECKS-APOCALYPTIC-AUDIO-STUDY.md) examines 48 minutes of reference streams plus candidate diagnostics and identifies register, harmonic grammar, orchestration and balance problems. Computational analysis is complete; it is not a claim of human critical listening or a verified reference transcription.

The overall apocalyptic vibe is closer, but Dylan rejects the existing drum timbre and sparse literal three-hit rhythm. The target is grandiose, resonant drums and fuller, more interesting beats that remain audible under a lead. Desktop [prototype 05](TWRECKS-APOCALYPTIC-PROTOTYPE-05.md) now auditions recorded taiko from a 40,312-byte compact bank, with alternating figures, ensemble accents and fills. It is a desktop sound/arrangement test; sample playback is not yet ported to the FM-1. Get listening acceptance before a wider bank rewrite or another hardware flash. A literal march is not required.

## Requirements retained

- Only the twenty dinosynth presets; no original bank or migration.
- Readable labels, four-column/four-row control tables and colored Pac-Man fill indicators; physical knobs do the editing.
- Direct instrument voice choices that save with the preset.
- Dinosaur sounds placed deliberately, with an optional automatic mode.
- At least one aggressive scene with deep synth bass and heavy drums; keep useful quieter scenes.
- Listening acceptance matters independently of passing engineering tests.

## Implemented in this candidate

### Home voice controls

SELECT chooses Drone, Sequence, Lead, Sequence 2, Atmosphere, Skin or Calls. ALGORITHM changes the selected melodic voice, Atmosphere character or creature. The selected name remains in the header. Skin retains the supported fixed TRIBE kit. The four right-hand encoders keep their established jobs. Voice edits save with the preset and survive reboot.

### Dinosaur performance

Calls default to Manual at boot. Select Calls on Home or press SAVE there to reach Creature calls immediately. The first eight white keys trigger the eight creatures; the first black key toggles Manual/Auto and the second stops the current call. The page shows the creature-key legend. Manual events bypass the automatic cooldown and work with music stopped or held. Level and auxiliary mute still apply. Automatic calls remain optional and bar-aligned. The operating mode is a temporary performance choice, not a saved parameter.

The existing mono SAURUS part keeps a bounded three-second gate and shares the eight-voice budget. Entering/leaving call key mode releases held instrument-key ownership and black-key effects. Manual triggering does not restart the composer or change tempo. Existing effects-grid and performance controls retain priority outside the dedicated context.

### STAMPEDE prototype

The existing second preset now has a stronger lower tar-bass part, a steady BOOM/SKIN pulse, variable log accents, a moderate 96 BPM pace and less echo/reverb masking the attacks. The intent is weight and drive. No literal march grammar or elaborate new composition framework has been added. Ambient scenes keep their sparse drum profile.

At high reported load, optional accents/automatic calls are suppressed before guaranteed drum hits. This is an event-admission policy; measured device CPU and audible stability are still required.

## Validation and acceptance

Host checks cover voice changes on each supported Home track, correct creature-key routing, mode toggles, black-key effects isolation, saving/rebooting, manual calls while stopped, automatic suppression in Manual mode, finite audio, headroom and the shared voice budget. The twenty production audio baselines deliberately change because automatic calls are now off by default and STAMPEDE changes musically. Original engine baselines remain host-only regression fixtures.

The hardware build must fit image, RAM and pool limits and pass updater checks before installation. After installation, listen to STAMPEDE with calls off and with manual accents, compare the bass/drum balance, test repeated calls and voice changes, and read average/peak CPU on Setup. Simulator render speed is not device CPU measurement.

## Next musical work

Judge STAMPEDE as a representative heavy prototype before spreading its approach across the bank. If needed, refine the bass figure, accents and articulation based on listening. Do not impose a literal march requirement.

Then evaluate a representative melodic/majestic scene, followed by an audit of all twenty roles: atmosphere, melody/grandeur, ritual/lumbering and aggression. Strengthen weak motifs, phrase variation and sectional contrast where needed; preserve quiet scenes that succeed. Keep voice choices predictable and retain twenty entries. A full bank rewrite and final musical acceptance are not complete in this candidate.
