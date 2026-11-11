# T-WRECKS — revised completion plan

Status: planning and research. The installed 464e9c7 candidate is usable and Dylan reports it is fine so far. It is not yet accepted as the final musical direction. Firmware implementation is paused for dinosynth research and the requirements below.

Read [the dinosynth research](TWRECKS-DINOSYNTH-RESEARCH.md) before starting implementation. Its source-backed findings, design inferences and outstanding reference-listening work are distinguished explicitly.

## User requirements

- Keep only the twenty dinosynth presets. Do not restore the original preset bank or migration work.
- Let the player choose the voices used by the instruments, visibly and directly.
- Let the player trigger and control dinosaur sounds when wanted.
- Include a genuinely aggressive scene with deep synth bass and heavy marching drums.
- Broaden the musical identity beyond the current predominantly ambient arrangements.
- Preserve the approved large labels, four-column/four-row organization and colored Pac-Man fill indicators. Physical controls are already present; no simulated knob bodies or numeric parameter readouts are needed.
- Research dinosynth before continuing implementation. Passing software/audio-level tests alone is not a musical acceptance criterion.

## 1. Research and musical brief

Documentary source review is complete in the accompanying report. Before a bank-wide rewrite, compare representative reference audio with the proposed musical brief: recurring ideas, deliberate bass/percussion roles, scene development, and a range from atmosphere to menace and grandeur. Do not present unmeasured BPMs or unperformed listening analysis as facts.

Use the research queue to check the core dungeon-synth direction, heavier Synthosaurus material, Thagomizer's documented brass/timpani/808 approach, and the wider melodic/ambient range. The heavy scene is Dylan's concrete target; the other scenes should retain their individual purposes.

Output: a concise reference-to-prototype brief. Keep engineering proposals distinct from genre-wide claims. Do not rewrite all twenty presets before a representative heavy and a representative melodic arrangement have been judged by listening.

## 2. Instrument voice selection

The existing layer pages already support ALGORITHM voice selection. Bring that operation into the central Home flow:

1. SELECT chooses a track.
2. ALGORITHM chooses that track's supported voice or character.
3. The selected voice name is clearly visible in readable text.
4. The four right-hand encoders retain their established parameter jobs.

Use the same convention for Drone, Sequence, Lead and Sequence 2. Atmosphere chooses its character; Calls chooses its creature. Audit a suitable kit/timbre choice for Skin using only supported drum resources. Do not offer an engine requiring state the track does not have.

A chosen voice is saved with the edited preset. Generative variation leaves that choice alone. Switching presets recalls their choices. Explicit sound-change gestures may still change a voice deliberately. Test held notes and releases during voice changes, including mono sounds and the shared voice budget.

Acceptance: the player can change an instrument from Home, see the chosen name, edit another track, return, save and reboot with the selection intact. Other track voices and the scene's tempo are unaffected by that edit.

## 3. Dinosaur sounds on demand

Make manual triggering an explicit operating mode, with optional automatic calls:

- Manual: the player places calls; automatic scheduling is silent.
- Auto: retain an optional generative caller for scenes where it is useful.
- A combined option is secondary to getting Manual and Auto unambiguous; it may be added if the control layout supports it cleanly.

Provide quick access to the call controls, rather than requiring five SAVE presses during performance. Audit the existing button/key assignments before choosing the final binding. A dedicated call mode can map physical keys to creatures while keeping normal instrument playing elsewhere. On entry and exit, release notes owned by the previous key mode so held keys cannot leave stuck voices.

Candidate expressive controls are pitch/register, call length/release, strength and reverb; Home keeps the call level. A manual trigger takes priority over the automatic cooldown and should respond promptly. A currently sounding automatic call can give way with a short fade where needed, while bass and rhythm continue. Use the existing mono SAURUS part first; reserve its voice budget and preserve a bounded release behavior.

Acceptance: trigger every creature deliberately, choose one repeatedly, stop or release long calls, switch between manual/automatic operation, and return to instrument playing. No restart of the music, tempo jump, stuck note, unintended automatic call in Manual mode or loss of the core bass/march is acceptable.

## 4. Aggressive STAMPEDE prototype

First rework the existing STAMPEDE slot as the heavy proof of concept, keeping the bank at twenty presets.

| Element | Proposed behavior |
|---|---|
| Bass | A prominent low mono synth figure using existing bass voices, with articulation and upper harmonics as well as low-frequency weight. |
| Drums | A retained marching pattern with clearly accented footfalls, supporting skin/log hits and restrained phrase-end fills. Replace the global three-candidate-hit restriction with a scene-specific patterned profile. |
| Motif | A short recurring bass or lead idea that remains recognizable through variations. |
| Harmony | A deliberate dark/modal setting supporting the figure; changes coordinated with phrase boundaries. |
| Form | Establish the march, answer it, briefly reduce it, and bring the full figure back. |
| Texture | Controlled saturation and filtering, with enough separation for drum attacks and the bass. Assess on headphones and the built-in speaker. |
| Calls | Available manually as accents; the scene's identity must hold with calls muted. |

An initial moderate march around 80–100 BPM is a design starting point, not a researched universal tempo. Perceived aggression should come from the figure, accents, articulation and timbre. Level and tempo adjustments alone are insufficient.

Retain a sparse percussion profile for ambient scenes. The change is to add suitable rhythm architectures, not force the march into every preset.

Acceptance by listening: a clear march and bass-led identity; a memorable recurring idea; useful phrase variation; an audible reduction and return; enough space to play over it. Evaluate with calls both off and manually placed. Do not propagate the approach across the bank until this scene works musically.

## 5. Broader preset revision

After the representative scenes work, audit all twenty by role: atmospheric landscape, melodic/majestic, lumbering/ritual, and aggressive. At least one scene must fully meet the requested aggressive brief. Several others need stronger melodic and rhythmic identity. Preserve quiet scenes that succeed at their purpose.

Change arrangement behavior where needed—motif, rhythm, bass role, articulation and sectional development—not just sound selection. Keep per-preset voice choices and manual edits predictable. Retain twenty entries; replace or revise weak scenes instead of adding filler.

## 6. Hardware and regression checks

Reuse the existing bass track, three compact drum states and mono call part before allocating new engines or voices. Current measured free space is approximately 51.5 KB of image capacity, 7.8 KB of RAM and 10.6 KB of pool space. CPU headroom is unmeasured on the installed new arrangements.

- Test Home voice changes and save/reload across every supported layer.
- Test manual call latency, release, repeated triggers, voice changes and mode changes with held keys.
- Test the heavy scene with bass, drums, effects and a call active together.
- Keep essential marching rhythm and bass reliable under load. Prefer reducing optional tails/effects or automatic events to losing the core pulse. Measure the actual cost before promising this policy.
- Record average/peak device CPU and any audio dropouts; simulator speed is not an FM-1 CPU measurement.
- Verify audio headroom, consistent perceived levels, stable transitions, and the protected save banks.
- Update tests for deliberate musical changes, retaining the old engine fixtures only as host regression material.
- Compare the musical result by ear against the approved brief. Numeric level checks do not establish that it sounds like dinosynth.

## Current task boundary

This revision changes research and planning documents only. The running firmware remains the installed candidate. No control changes, marching-profile implementation, preset rewrite or additional flash is part of this research pass.
