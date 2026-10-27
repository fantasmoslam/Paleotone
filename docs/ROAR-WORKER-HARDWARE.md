# ROAR worker audit hardware trial

Candidate 0a1327c816086576b3d741e785c2cfbf7e3d8c42, renderer 3513154928, package e1c9b802a4d738fb5d091c420f031c23775d86ea43aaef04e384028a34a046c6, 610098 bytes. Based on81bae2a, not the rejected oscillator experiment.

Audit: ROAR writes voice-local phases/noise/filter/drum/envelope state. Its shared reads (clock, song.playing, crank controls) are stable during the joined audio interrupt: events and engine_block finish before launch; foreground ui_input cannot run until the ISR has joined the worker; nested USB/input service enqueues rather than changing those controls. Engine changes commit in events_block; fade rendering only uses the owning track's old parameters and decrements its own fade. Other tracks do not write these inputs while rendering. A future independent control task would require snapshots/re-audit.

Remove only ROAR exclusion; VOICE/SAURUS remain on CPU0, PHYS still disables splitting. Actual worker-participation checks updated. Concurrent63 scenes include four ROAR tracks with all19 pulse modes, crank speed/strokes, RING, clock/tempo and transport changes BETWEEN joined blocks, plus mixed ROAR/SAURUS/FM6/VOICE. Initial concurrent/fidelity, forced ROAR and worker-golden checks passed; ThreadSanitizer passed. Full existing host suite passed (ALL HOST TESTS PASSED2026-10-27T10:59:22.391Z), with213 unchanged sound goldens and all63 concurrent/ThreadSanitizer scenes matching. Host runtime instruction counters were unavailable/zero and are not runtime CPU-budget evidence.

Target40490167747/job122141821192/artifact12727981586 passed. Image549732 (4 bytes smaller than81bae2a), staticRAM96184/98304 and pool332900/344064 unchanged, fixed CPU1 user/exception stacks4096/256 unchanged. No additional synthesis state, buffers, oscillator arithmetic or higher budgets. Companion40490167827/job122141821297/artifact12728566755 passed; live exact-source page verified: https://fantasmoslam.github.io/Paleotone/webapp/screen-roar-worker/ . Existing current page screen-midi-dispatch preserved. PR21 draft.

Fresh91 BPM baseline81bae2a: instrument means322.0922 (short playing window) and304.7938 us/32 frames (30-second USB capture); surrounding34.677 seconds: one late block,26 voice cuts, zero USB underruns. Device core ON, stopped/profile OFF after baseline. Baseline: work/paleotone-midi-dispatch/hardware/worker-baseline-1793097753254.

Prepared private trial: work/paleotone-roar-worker/hardware/trial-1793097916891. Repeat: hardware/repeat-1793098183253. Rollback81bae2a package and guard: hardware/rollback-dispatch. Before profiling, preserve-live.mjs captures post-install state, restores only live object0 if that alone differs, and requires all13 objects match original; other differences stop the trial. This avoids measuring an older reloaded tempo.

Worker selection proof uses existing read-only memr console command, not new firmware instrumentation. Exact ELF symbol fm1_c1_mb=29397760 (size128), argument field=29397764, validated within SRAM. Its low byte is the selected track mask (argument is mask | block_frames<<8); track1 ROAR is bit0. Status batches read only that byte so measurements retain their existing serial timing. A selected mask with bit0 plus successful worker counters/zero faults shows actual ROAR submission and completion; it is a recent job snapshot, not a count of all ROAR blocks. Byte sampling is not a coherent entire-mailbox snapshot.

Installation and trials completed; results below. Keep raw backups private.

## Verified installation and live-state preservation

Fresh backup preceded guarded installation. Identity verified2026-10-27T11:01:25.533Z: source0a1327c816086576b3d741e785c2cfbf7e3d8c42, renderer3513154928, schema2,82 globals, packagee1c9b802a4d738fb5d091c420f031c23775d86ea43aaef04e384028a34a046c6. The post-install check found a live-object difference after restart and restored ONLY archived object0 through the existing validated protocol. All13 objects then exactly matched pre-install capture. Saved project slots, presets, samples and settings were not rewritten by that restoration. This is a workbench-harness safeguard; the public installer does not yet implement automatic live-state restoration.

## Hardware results, same91 BPM scene

| Metric | Baseline81bae2a | ROAR worker run1 | ROAR worker run2 |
| --- | ---: | ---: | ---: |
| Instrument stage mean, short playing window, us/32 frames | 322.0922 | 286.8797 | 286.8185 |
| Instrument stage mean, USB window, us/32 frames | 304.7938 | 285.7395 | 285.9258 |
| New late blocks, surrounding USB interval | 1 | 0 | 0 |
| New overload voice cuts, same interval | 26 | 0 | 0 |
| USB packet underruns,30-second capture | 0 | 0 | 0 |

Surrounding intervals were34.677 /34.761 /34.723 seconds. Instrument-stage time improved6.25% /6.19% in the longer USB windows, and10.93% /10.95% in the short playing windows. This is aggregate main-processor instrument time including its worker join, not a ROAR-only or whole-device percentage. Per-track profile rows remain disabled when core2 is ON. Live windows are not sample/phase aligned, but repeated results agree and no voices were culled in the candidate windows. Other stage means remained close: effects95.72 ->96.26us, master77.31 ->76.95us, events26.76 ->26.88us per32 frames during capture.

Read-only mailbox samples during both active runs returned mask0x05: track1 ROAR and track3 FM6 were selected together. After stop a sample was0x04, consistent with remaining FM6 work; final stopped masks are not used to infer active ROAR participation. No worker faults/fallbacks; maximum wait17us and reported stack use784 bytes within existing allocations.

An additional60-second unprofiled USB capture ran alongside continuous command96/29 readback pairs:561 pairs (1122 requests), zero exceptions, maximum successful pair59ms. The surrounding64.723-second interval had zero late blocks, zero overload voice cuts, zero USB underruns, overruns, missed packets or stalls. Raw fill minimum200 frames, maximum publication gap1951us; no empty-ring underrun occurred. Successful capture10587528 PCM bytes. Worker mask0x05,292657 cumulative worker blocks, faults/fallbacks0, stack784 bytes, maximum wait17us. Recorded lifetime render/wall maxima2533/2742us against2902us deadline; do not compare these peaks as a speedup against the baseline's different lifetime.

## Decision and final state

Retain this experimental build: actual ROAR offload is confirmed, repeated headroom improvement is useful, and overload cuts disappeared in the measured workload without extra RAM or changed sound equations. This is bounded hardware evidence, not a general stability/release certification. Real external MIDI-clock/note input, manual control changes, every ROAR preset and longer multihour playback/recording interactions remain release gates; control transitions were covered in host tests. PHYS still disables splitting, and VOICE/SAURUS remain main-core-only. Twelve-voice budget and SCAPE behavior remain unchanged.

A fresh final backup again matches ALL13 original objects by size, CRC and encoded data. Fresh serial: playing0,91 BPM, second-core preference and active worker ON, idle CPU30%; fresh profiler query profile_on0. No worker faults or fallbacks. Current installed source0a1327c816086576b3d741e785c2cfbf7e3d8c42. Current matching display https://fantasmoslam.github.io/Paleotone/webapp/screen-roar-worker/ . No public installer firmware update or merge; PR21 remains draft in the experimental stack.

Raw evidence: comparison.json, before-ui-backup.json, post-install-before-live-check.json, restored-live-backup.json, post-profiling-backup.json, saved-state-comparison.json, final-status.txt/final-profile.txt and ELF mailbox resolution in the trial directory above. Repeated phase files are in hardware/repeat-1793098183253; unprofiled/MIDI capture evidence in hardware/midi-usb-1793099230991. Keep archives private.

Next: use this validated headroom baseline for independent SCAPE state/voice-budget design and the remaining recording/control soak checks. This improvement alone does not implement independent SCAPE or establish unlimited additional synthesis capacity.
