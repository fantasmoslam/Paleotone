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

Worker startup after power cycle confirmed cpu2_set=1 and cpu2=1; block count increased during playback. This is actual device evidence, not emulator output. Playback stopped after the short comparison; second-core preference remains ON.

## Interpretation and limits
Worker execution and a short-run timing benefit are demonstrated on this device/workload. Voice competition remains: the twelve-voice budget was not increased. Residual late blocks remain, so this is not an overload-free result or public-release acceptance.

Runs were not sample-aligned, did not have identical uptime, and worst-block counters are cumulative since boot. CPU numbers are snapshots, not averages. Do not claim a universal percentage speedup or compare absolute cumulative counts across boots. Listening quality has not been assessed by the automated tools.

Pending: owner listening feedback; longer stability run; USB audio under load; expanded-display simultaneous operation; live recording/save/reload; controlled fault/fallback and reset/update checks; additional engine workloads. SCAPE independence is not implemented. Retain backups/rollback before further updates.
