# Paleotone: at-home test sheet
Prepared 2026-10-26, America/Phoenix. Start with the firmware already installed.

## Goal
Have a short, useful playing session that establishes the current baseline. Then, if a backed-up candidate installation is ready, compare second-core OFF and ON using the same music. This session does not test a finished independent SCAPE: that redesign is not implemented.

## What is ready
Candidate source: 21404aa73f3480c495424267d9b1acb6707ac4fd.
All three exact-source checks have now passed:
- Target firmware/image/updater/loader budgets: https://github.com/fantasmoslam/Paleotone/actions/runs/40391720933
- Full host/dual-core validation: https://github.com/fantasmoslam/Paleotone/actions/runs/40391720169
- Matching pinned companion: https://github.com/fantasmoslam/Paleotone/actions/runs/40391789881

These checks do not establish physical-device reliability or authorize skipping backup/identity checks.
No dual-core candidate has been installed during this work.

Installed baseline recorded in the previous handoff:
- Source 76d32f3050fe21968ac13b017ad8d98a1928b456 (same tree as rewritten history source 10c6195ae76eb97974b9cee4655621489b3368d4).
- Renderer 799295047; schema 2.
- Package SHA256 9b808fd7da0c4422863d00a607a1158b66562ffa6dbd736c0f2da3297fb501b7.
- Matching display: https://fantasmoslam.github.io/Paleotone/webapp/scape-preview/screen/

The restored display and its download retain that original identity. They are not the new dual-core candidate.

## Bring these
- FM-1, a data-capable USB cable and a home computer that permits MIDI.
- Headphones or speakers at a comfortable low starting level.
- Chrome or Edge and the display link above.
- A saved project you know well. Prefer a copy that uses all four tracks.
- Optionally a phone/audio recorder for short before/after examples. No DAW required.

## Part A: current installed firmware, about 15 minutes
No firmware update is needed for this part.

### 1. Connection and controls — 2 minutes
1. Connect the FM-1 and open the matching display.
2. Connect through its MIDI control and select Expanded.
3. Use HOME+OCT− for the left pane and HOME+OCT+ for the right.
4. Change the focused pane's bank, then return to the other pane.

Record: connects / does not connect; each pane keeps its bank / does not; physical controls follow focus / do not.
If it fails, record the browser's exact message. Do not change the firmware just to diagnose a display problem.

### 2. Normal song playback — 3 minutes
1. Load a familiar saved song and play it with SCAPE activity disabled using its normal controls.
2. Listen to the start, a busy passage and the transition back to the beginning.
3. Stop and restart once.

Record: clicks, missing notes, stuck notes, sudden level changes or timing problems. Note the song name, track sounds and the action immediately before the problem. This is the reference for later comparison.

### 3. Independent habitat and drone — 3 minutes
1. Stop the sequencer.
2. Raise the habitat level modestly and listen; keep creature track OFF and passing-beast activity at zero for this check.
3. Change one environmental layer at a time.
4. Add a little of the separate drone, then change its key/movement.
5. Return levels to their previous positions.

Record: audible while stopped / not audible; which layers are too quiet or too similar; any clicks or unwanted song playback.
Do not judge the not-yet-built independent musical SCAPE from this test.

### 4. Capture the SCAPE problem — 3 minutes
Use a disposable project copy only. The present SCAPE composer can replace song patterns after confirmation.
1. With all four tracks occupied, identify which SCAPE action interferes with your music.
2. Try one action at a time: mix/breathing, creature activity, or composition.
3. If composition asks to replace patterns, cancel unless this is a backed-up disposable copy.
4. Note whether the result changes notes, sound choice, volume or transport.

Useful report: “On track __ using sound __, I changed __; it changed __. I expected __.”
A 20–30 second audio example is useful if easy to make.

### 5. Small recording check — 3 minutes
Use an empty disposable project slot or a known test copy; do not overwrite valuable music.
1. Record a simple four-note phrase using the usual live-note recording workflow.
2. Stop recording and play it back.
3. Save to the test slot, reload it and play again.

Record: notes captured correctly / not; timing acceptable / not; survives save/reload / not.
This checks sequencer note recording, not internal audio recording.

### 6. Choose the first listening target — 1 minute
Pick two or three priorities:
- Fewer repeated calls.
- More obvious changes over time.
- More silence and breathing room.
- Richer musical phrases.
- More distant/nearby creature movement.
- Individual call triggering and editing.
- Complete separation from my song.

## Part B: second-core candidate, only after installation preparation
Pause here until the exact candidate package, matching companion and backup/rollback procedure are staged and verified. Do not install the package from the baseline display expecting second-core support. A package hash will be supplied with the actual staged candidate; the source hash alone is not a package checksum.

### Installation preparation
- Capture and verify a fresh project/settings backup.
- Retain the known installed rollback package and recovery instructions.
- Confirm the staged package is for FM-1, its source/package identity, and its matching renderer.
- Resolve an actual device access path; the user's working browser connection does not imply the assistant can operate it.
- Close/disconnect competing MIDI clients for the transfer when requested.
- Install using the verified procedure, reboot, and verify reported identity.

### OFF/ON comparison — about 15 minutes
1. Set MENU > SYSTEM > 2ND CORE to OFF and restart.
2. Load the exact same saved test song. Keep all sound, effects, tempo and volume settings fixed.
3. Play its busiest passage for two minutes. Note audible faults and control/display responsiveness.
4. If exposed through the available diagnostics, capture worst audio-block time/late counts and worker status. Do not invent a menu location for telemetry.
5. Set 2ND CORE to ON (BETA), restart, reload the same song and repeat.
6. In both modes, briefly try live note recording, USB audio if normally used, expanded display focus, stop/start and save/reload.
7. Return OFF and restart if the worker faults or audio becomes unstable; capture the exact trigger before further experiments.

Important: some engines remain on core 0 and PHYS disables the split. Include a suitable non-PHYS workload before interpreting “no difference.” Booting with the default OFF setting does not test CPU1.
Do not expect more voices or independent SCAPE from enabling the worker. A failed worker can cause a glitch, and a heavy dual-core workload may not fit on one core.

### Longer check
If the short comparison is stable, let the ON workload play for 20–30 minutes and report any delayed glitches, stopped audio, disconnection or unexpected reboot. Deliberate power interruption during a save or update is not part of the home test.

## Copy-and-fill report
- Firmware tested: installed baseline / staged candidate
- Browser and computer:
- Connection and expanded focus:
- Test song and four track sounds:
- Normal playback:
- Habitat/drone while stopped:
- Exact SCAPE interference example:
- Four-note recording and reload:
- Most important listening improvements:
- If candidate installed: OFF result / ON result / long-run result
- Optional clip or error text:

## After the session
Use findings to select the smallest independent SCAPE prototype and a measured voice/CPU/RAM budget. Keep the VST side project parked. Public hardware release still needs usable sequencing, recording and independent generation, documentation, compatibility, recovery and wider testing.
