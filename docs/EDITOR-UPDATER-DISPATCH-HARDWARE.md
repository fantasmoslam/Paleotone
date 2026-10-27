# Editor/updater dispatch fix hardware validation

Source 81bae2a3b9a0e2157e7a781ff7daa9015c3d279f, renderer 3177467445, package 4507db3422c64a380fe43f9fbde96991243405cf334b536126e0e8af06295773, 610098 bytes. Based on the diagnostic build86fa328. Draft PR19.

Target run40479669210/job122106511367 and companion run40479669230/job122106511510 passed; host run40479669212/job122106514758 passed all checks (ALL HOST TESTS PASSED 2026-10-27T09:31:52.560Z). Sound regression preserved213 goldens; concurrent/ThreadSanitizer61 scenes unchanged. Host runtime instruction counters were unavailable/zero; static target budgets passed. Target image549736, staticRAM96184/98304, pool332900/344064; no budget limits changed.

Baseline diagnostic extended run accepted969 requests, handled966 and replied966; the other three match three host timeouts (one command96, two command29). Receive-busy and TX-timeout counters remained zero. Code audit identified updater consumption of editor frames arriving between ed_service and ota_service.

Fix: only the normal loop calls ota_service_editor; the frame-taking helper leaves editor-prefixed frames for the next editor pass. Loader/rescue retain original ota_service; active update sessions retain ota_take without preservation. Unit regression injects command29/96 into the inter-service arrival window and checks preservation plus identity reply, active-session disposal, malformed-prefix disposal and loader/rescue disposal.

Hardware plan: fresh backup and exact identity installation, isolated600-request comparison during stopped/playback/USB capture, then up to1000 mixed pairs (2000 commands) during playback. Log firmware counter deltas, native fragments and send acknowledgments. Capture timeout exceptions without hiding them with retries. Retain installed diagnostic package as rollback. Compare all13 saved objects and leave playback stopped, profiler OFF, core ON.

Private candidate path: work/paleotone-midi-dispatch/hardware/trial-1793092955837; mixed path: hardware/mixed-1793092978450. Matching display: https://fantasmoslam.github.io/Paleotone/webapp/screen-midi-dispatch/. No public installer change or merge.

## Completed hardware results

Fresh backup preceded guarded installation. Identity verified at 2026-10-27T09:34:29.585Z: source81bae2a3b9a0e2157e7a781ff7daa9015c3d279f, renderer3177467445, schema2, globals82. Matching display metadata independently verified HTTP200.

Isolated comparison: command96 and29 each100 requests while stopped, playing and playing with USB capture: all600 succeeded, no exceptions. The 60-second 44.1 kHz stereo USB capture completed successfully with zero new underruns or overruns. Surrounding62.3-second interval still had two late blocks and44 voices shed, raw fill minimum104 frames, maximum publication gap3714us. This fix does not resolve synthesis pressure.

Extended playing test:1000 mixed pairs,2000 requests total, all2000 host replies succeeded. Firmware deltas: RX starts2000, accepted frames2000, handler29/96 each1000, reply attempts and completed enqueues each1000 per command, busy rejections0, send timeouts0. No retry hid a failure. Last-command/argument fields are markers and are not included as count deltas.

| Measurement | Diagnostic baseline | Fixed mixed run |
| --- | ---: | ---: |
| Host requests | 969 | 2000 |
| Accepted by firmware | 969 | 2000 |
| Editor handled / replied | 966 | 2000 |
| Host timeouts | 3 | 0 |

These are bounded live runs, not a general reliability certification. The deterministic interleaving regression plus hardware counter agreement supports the ownership fix. The test covers editor control/readback commands29/96; external MIDI note/clock performance and longer interaction tests remain separate release gates.

All13 post-test backup objects match the fresh pre-install archive by size, CRC and encoded data. Fresh final serial confirms playing0, cpu2_set1, cpu2=1, idle CPU30%; fresh profile query confirms profile_on0. Worker fault/fallback counters remain zero. The fix remains installed. No public installer firmware update or merge was performed. Retain PR19 as draft while the optimization stack awaits release checks.

Private raw evidence includes results.json/native JSONL/capture.wav in the trial directory; mixed results.json and logs in hardware/mixed-1793092978450; saved-state-comparison.json, final-status.txt, final-profile.txt and validation-manifest.json in the trial directory. Keep raw backups private.

Next engineering work: return to the focused ROAR oscillator-call optimization described in NEXT-SYNTHESIS-HOTPATH-AUDIT.md, keeping this dispatch fix and USB queue margin constant. Longer MIDI notes/clock, physical controls and recording interaction soak tests remain required before release.
