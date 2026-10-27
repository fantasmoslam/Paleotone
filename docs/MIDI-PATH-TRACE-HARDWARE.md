# MIDI path counter hardware trial

Candidate source 86fa328f43a89f6a6c821a39ca301bf2f6621112, renderer 3220445640, package SHA256 8e872ce8fa484a3dbea6eb312d2e813984005a90efc27821430761d8b5374ca3, 610098 bytes. Target run 40476412100/job 122095626818 passed. Companion run 40476412151/job 122095626934/artifact 12720398609 passed. Host run 40476411983/job 122095626142 passed all checks, including 213 unchanged sound hashes, 61 concurrent scenes and ThreadSanitizer. Installed and identity verified at 2026-10-27T09:08:13.705Z.

First source 3b86fa3 failed the unchanged USB static budget because diagnostic updates expanded inside nested loops. Diagnostic helpers were moved out of the loop body; a feature-disabled build error was then fixed under FELUCCA_OTA. Final source keeps unchanged budgets; usb_poll cost 21060 vs budget26037. This is static bookkeeping, not measured runtime acceleration. Image549708, staticRAM96184/98304, pool332900/344064. The 52-byte counter object adds 48 bytes to total static allocation after alignment.

13 status counters: mt_rx_start, mt_rx_busy, mt_rx_done, mt_rx_cmd, mt_rx_track, mt_handle29, mt_handle96, mt_tx29, mt_done29, mt_tx96, mt_done96, mt_tx_timeout, mt_tx_fail_cmd. TX done means enqueued, not received by the host. Each field has one writer; status observations are not a coherent snapshot. Sample after settling. Counters are not persisted. No music/settings/voice limits/USB targets change.

Private trial directory: work/paleotone-midi-trace/hardware/trial-1793090636178. Guarded templates now pin final source/renderer/package and size. Native WinMM fragment/partial-drop logs retained; request failures capture serial counter state. Previous USB package and guarded rollback prepared in hardware/rollback-ready. Matching companion staged at https://fantasmoslam.github.io/Paleotone/webapp/screen-midi-trace/; publishing may still be in progress.

Draft PR18: https://github.com/fantasmoslam/Paleotone/pull/18 . No merge/public installer update.

## Hardware outcome

First isolated-command comparison: all600 requests succeeded across stopped, playback and USB capture. A subsequent mixed-pair run reproduced three timeouts after969 total requests. Before/after deltas: RX starts969, accepted complete frames969, busy rejections0; handler29 +482 and handler96 +484, total966. Reply attempts and successful enqueues match those966 handles; TX timeouts0. The host logged one command96 timeout and two command29 timeouts. At each timeout its command/argument had reached the receive path but its handler count had not advanced. Native fragments and WinMM send acknowledgments were logged.

This locates the missing three between receive acceptance and editor handling, rather than inside the host SysEx assembler or reply enqueue loop. main.c runs ed_service then ota_service; ota_take consumed/released frames without excluding the editor header. An IRQ publishing an editor request between those service calls lets updater service consume and reject it. A deterministic interleaving regression and targeted ownership fix are prepared on fix/editor-updater-dispatch, source81bae2a, draft PR19. Hardware confirmation of that fix is pending exact-source validation.

A first extended-run startup encountered WinMM7 (input allocated) before polling; no lingering test process was found and a retry connected normally. It is excluded from request counts.

All13 backup objects match pre-install state by size, CRC and encoded data. Playback stopped, profiling OFF, core ON. Diagnostic build remains installed; use screen-midi-trace. Private extended-run evidence: work/paleotone-midi-trace/hardware/mixed-1793092275754. Keep raw backups private.

## Prepared fix validation checkpoint

Fix source81bae2a3b9a0e2157e7a781ff7daa9015c3d279f, renderer3177467445, PR19. Target run40479669210/job122106511367 passed, including the expanded real updater regression and loader test; instruction budgets unchanged. Host run40479669212/job122106514758 and companion run40479669230/job122106511510 still running at checkpoint. Fix is NOT installed. Normal main calls ota_service_editor, while loader/rescue retain ota_service and active sessions retain ota_take. The editor-preservation decision occurs within the same frame-taking operation; checking before calling the old service would leave another arrival window.

Current installed firmware remains diagnostic source86fa328, renderer3220445640, package8e872ce8fa484a3dbea6eb312d2e813984005a90efc27821430761d8b5374ca3. Current matching display screen-midi-trace. No permission question is pending; complete remaining exact-source gates before a fresh-backup install and repeat mixed polling to verify the fix on hardware.

Fix follow-through completed: source81bae2a installed and verified; 2600 editor readback requests succeeded, including2000 mixed requests with all firmware receive/handle/reply counts matching. See EDITOR-UPDATER-DISPATCH-HARDWARE.md for full validation, saved-state proof and current device identity. The installed display is now screen-midi-dispatch.
