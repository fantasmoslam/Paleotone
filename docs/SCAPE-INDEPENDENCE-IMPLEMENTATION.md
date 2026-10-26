# Independent SCAPE implementation boundary

Status: investigation completed 2026-10-26; implementation pending measured hardware headroom. This is not a claim that SCAPE has been separated.

## Confirmed interference
- dream.c dr_phrase clears and rewrites trk[ti].step and changes P_SLEN.
- dream_compose changes song track root/scale, clock subdivision, chord/arp controls and ROAR parameters.
- dr_chord transposes the notes stored in every song track.
- scape_director.c scm_level changes sequencer track output gains from SCAPE role levels.
- scape.c calls trk_note_on on selected song tracks for CALLS and passing-beast roars; this shares patches and sounding voices.
- Generation currently requires song.playing, and uses sequencer positions for evolution boundaries.

The procedural habitat and its extra drone already have their own oscillator/event state. Preserve those rather than replacing working independent audio.

## Proposed boundary
Keep the explicit pattern composer as a separate, clearly destructive tool with replace confirmation. Independent live SCAPE must never call that tool, transpose song steps, change track parameters, or apply role gain to song audio.

Introduce compact SCAPE-owned layer settings, event clocks, PRNG streams and synthesis state. Do not add four full track_t instances: each contains twelve voice slots and extensive sequencing state, and target RAM is tight. First prototype one independently synthesized creature voice with a bounded overlap count; derive costs before selecting the musical-layer implementation. Consider reusing engine DSP only after removing assumptions that its track pointer belongs to trk[].

Song, SCAPE and creature performance should mix through explicit buses. A shared master/reverb remains possible, but SCAPE controls cannot silently change song sends or levels. Define whether SCAPE follows the song key or uses its own; following reads harmony and never writes song notes.

SCAPE owns its clock while song transport is stopped. Optional transport sync must be explicit. Freeze/new variation/next harmony operate on SCAPE state only. Manual calls use stable call IDs and independent editable call settings. Randomness uses separate streams for timing, pitch, timbre and structure so one control change does not scramble everything else. Add weighted alternatives, recent-choice memory, clustered calls and quiet intervals; preserve seed-based reproducibility for tests.

## Implementation order
1. Record sizeof/target RAM and worst block costs, including existing dual-core candidate. Decide minimal layer/overlap budget.
2. Introduce a small independent event scheduler and state, with deterministic sample-clock tests and no audio-side allocation.
3. Implement one standalone call path and its note-off lifecycle. Expand to all call types after CPU/audio checks.
4. Add independent musical layers in measured increments; do not promise twelve song voices plus unlimited SCAPE voices.
5. Route output and expose physical controls; preserve expanded-screen focus.
6. Add a versioned sidecar extension only after specifying defaults, slot capacity, backup and older-project behavior. Existing IDs remain fixed.
7. Compare long software audio renders, then test combined load on FM-1 before release.

## Meaningful acceptance tests
- Fill all four song tracks. Snapshot steps, patch parameters, automation and transport. Exercise SCAPE new/next/hold/key/calls/evolution; assert song state remains byte-identical except its normal playback counters.
- With downstream shared effects disabled and resource contention excluded, compare the isolated song bus with SCAPE on/off; it must be identical.
- Exercise manual and generated overlapping calls; verify no stuck notes and that release messages cannot stop song notes.
- Verify muted SCAPE bypass, independent stopped-transport operation, save/reload and backup/restore.
- Reproduce renders with a fixed seed and compare controlled alternate seeds. Inspect repetition and variation over minutes, not just a short loop.
- Stress CPU, RAM, headroom and deterministic overload policy. Define and disclose voice competition if any remains.

## Hardware session
Use the same demanding project for second-core OFF/ON after restart. Capture worst block/late counters and worker telemetry, listen for glitches, exercise recording + USB + expanded display + save/reload, and retain rollback. A browser emulator cannot establish physical CPU1/USB/update reliability.
