# FM-1 short dual-core hardware trial — 2026-10-26

Candidate source: 21404aa73f3480c495424267d9b1acb6707ac4fd.
Package SHA256: 8d569b4cd471ae97a77f655cb4d927ae412cd88a0063b509b675b304f752f737.
Device renderer: 503813915; schema 2; globals 82.

Installation and reboot identity verified over MIDI. All 13 pre-install saved objects retained the same size and CRC after installation. USB serial enabled by owner; diagnostics accessed on COM6. Matching screen published at webapp/screen-dualcore/.

## Observed short comparison
Same current song/settings, reported BPM 84, selected track ROAR/STAMPEDE. No song edits were sent by the test tools. Three status samples separated by ten seconds in each mode; counts below are changes between first and third sample, approximately 21 seconds including request/read delays.

| Metric | Worker OFF | Worker ON |
| --- | --- | --- |
| Reported CPU samples (%) | 84, 85, 76 | 77, 79, 71 |
| Cumulative worst audio block (microseconds) | 3135 | 2931 |
| New late blocks in sample interval | 14 | 2 |
| New voices shed in sample interval | 15 | 15 |
| Worker active blocks at final sample | 0 | 61019 |
| Worker bad/fallback counters | 0/0 | 0/0 |
| Worker maximum wait (microseconds) | 0 | 8 |
| Worker stack high-water (bytes) | 0 | 748 |

Worker startup after power cycle confirmed cpu2_set=1 and cpu2=1; block count increased during playback. This is actual device evidence, not emulator output. A PLAY toggle was sent after the short comparison, but stop was not verified; owner subsequently reported continued playback. Never assume a toggle established a particular transport state.

## Interpretation and limits
Worker execution and a short-run timing benefit are demonstrated on this device/workload. Correction after source inspection: voices_shed is specifically CPU-overload protection (audio.c), not a count proving exhaustion of the twelve-voice budget. The budget is unchanged, but these observations establish processing pressure. Residual late blocks remain, so this is not an overload-free result or public-release acceptance.

Runs were not sample-aligned, did not have identical uptime, and worst-block counters are cumulative since boot. CPU numbers are snapshots, not averages. Do not claim a universal percentage speedup or compare absolute cumulative counts across boots. Listening quality has not been assessed by the automated tools.

Pending: owner listening feedback; longer stability run; USB audio under load; expanded-display simultaneous operation; live recording/save/reload; controlled fault/fallback and reset/update checks; additional engine workloads. SCAPE independence is not implemented. Retain backups/rollback before further updates.

## Follow-up with second core re-enabled

Owner identified the suspected whoosh as a heavily edited SAURUS/BRACHIO sound, rather than an established worker artifact. After another restart, cpu2_set=1 and cpu2=1 were verified with transport stopped. Playback was then started from that verified state.

Sustained readings reached 178335 worker blocks, cpu2_bad=0 and cpu2_gave_up=0; maximum wait 11 microseconds, stack high-water 748 bytes. Thirty actual display snapshot updates completed with 30 applied state buffers (421380 bytes processed). This tests the transfer protocol, not browser rendering or listening quality. At the last reading audio_late=19 and voices_shed=90 since boot: overload remains. Playback was left running for listening. No project patches or sequences were edited by these tests.

Pending: USB audio streaming, owner listening feedback, live recording on an agreed disposable project, save/reload, long-duration and other-engine trials. Do not call the candidate stable or ready for extra SCAPE load.

## Live recording, persistence and USB audio trial

Owner authorized editing the default loaded scene. A fresh 13-object pre-recording backup was captured first. All four project slots queried empty; test used Project A (slot 0), leaving B-D untouched. Four MIDI notes (60, 62, 65, 67) were sent while REC was armed; selected-track steps 0, 10, 12 and 14 changed, and all four requested pitches were present. Project save returned [1,0,1]. A step velocity was deliberately changed in RAM, then Project A reloaded ([0,0,1]); all 64 checked selected-track step responses matched the saved recording exactly. This verifies those steps, not every serialized field or power-cycle recall.

Direct capture used only Windows Line (Felucca), not a microphone. First capture: stereo PCM16 at 44100 Hz, 351918 frames / 7.98 s, peak 18222/32768, RMS 4660.9, zero full-scale clipped samples. USB diagnostics showed 7 underruns, 0 overruns, 0 missed packets and 0 stalls afterward.

A second approximately eight-second capture started with playback already running. During the capture, USB packets increased from 11267 to 15988 and underruns from 10 to 19. This demonstrates underruns continuing during the stream; it is not just an initial-start count. audio_late also increased from 48 to 50 across those reads. Worker faults remained zero. Do not label USB recording clean or assume CPU1 caused the underruns; a matched OFF comparison is still needed. No automated listening assessment was made.

Current scene now contains the authorized test notes and is saved in Project A. Original scene/settings backup retained locally. USB capture closed normally; playback left running for listening, second core ON. Next step: compare USB capture with worker OFF and isolate USB buffering/clocking versus audio workload before changing DSP or increasing voices.

## USB comparison with worker OFF

After owner disabled the worker and restarted, serial confirmed cpu2_set=0, cpu2=0 and stopped transport with counters reset. The same loaded scene was started, then captured through Line (Felucca) for approximately eight seconds. Between mid/end reads: USB packets 3171 to 7989; underruns 3 to 16; audio_late 12 to 16; voices_shed 15 to 19; reported CPU 84% then 80%. Capture completed successfully (1405908 PCM bytes). USB underruns therefore occur without the second-core worker.

Playback was then stopped and verified playing=0. A second capture under stopped-transport load completed (1409436 PCM bytes). During streaming, packets advanced 10985 to 15988 while underruns stayed at 16, audio_late stayed at 30 and voices_shed stayed at 32; CPU samples 38% and 37%. No added underruns in this observed low-load capture. Ambient/drone output was not explicitly muted, so this is a stopped-transport test, not a certified silence test.

This points toward a load-dependent problem in the audio/USB path rather than CPU1 being necessary for failure. It does not establish the precise root cause or exclude a buffering/scheduling defect aggravated by load. ON/OFF captures were not sample-aligned; do not rank the worker by these short USB counts alone. Profile expensive rendering, interrupt time and USB buffer behavior before changing thresholds, voice limits or SCAPE load. Final state verified stopped, second core OFF.

## Reversible track-mute isolation

Second core OFF; track levels retained; each phase settled for 5.5 seconds, then two serial reads bracketed a five-second wait. CPU values below are snapshots; intervals were not musically synchronized. Mutes stop new notes, so lingering release/effect tails can affect the comparison.

| Muted tracks | CPU snapshot (%) | New late blocks | New overload-shed voices |
| --- | --- | --- | --- |
| None | 78 | 2 | 5 |
| 1: STAMPEDE | 75 | 0 | 5 |
| 2: BRACHIO CALL | 70 | 0 | 4 |
| 3: MAMMOTH | 76 | 0 | 4 |
| 4: DEEP CHANT | 78 | 0 | 0 |
| 1: STAMPEDE, 2: BRACHIO CALL, 3: MAMMOTH, 4: DEEP CHANT | 37 | 0 | 0 |
| None | 85 | 4 | 4 |

Original level/mute readbacks matched exactly afterward; final transport stopped. Track identities: T1 STAMPEDE (engine 14); T2 BRACHIO CALL (engine 15); T3 MAMMOTH (engine 12); T4 DEEP CHANT (engine 5). T2 had the largest observed snapshot reduction, but the varying baseline prohibits treating these as exact independent costs. T4-muted and all-muted intervals had no new overload shedding.

Only FM6/T3 is eligible for the present worker split in this loaded mix: ROAR, SAURUS and VOICE remain on CPU0 for shared-state safety. The current part_cost array contains estimated engine/voice weights, not measured runtime costs. New diagnostic candidate c564f667fe54d051eb0158e54b86ba319e814c60 adds opt-in actual timing; it has not been installed as of this record.
