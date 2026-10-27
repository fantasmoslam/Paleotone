# Editor/updater request ownership

Hardware MIDI trace source86fa328 accepted969 of969 mixed polling requests, but only966 reached editor handling/reply enqueue; all three host timeouts match the missing handler counts. Busy-frame rejections and send timeouts stayed zero.

main.c calls ed_service then ota_service. A frame arriving between these calls was consumed and released by ota_take even when its header belonged to the editor. The fix lets the normal firmware handshake preserve editor-owned frames inside the same frame-taking operation. The loader/rescue default remains unchanged, and active update-session reads still consume unrelated frames, avoiding a blocked update.

Deterministic ota_test regression injects command29 and96 after the editor's empty pass and before updater service; verifies retained frame bytes, real identity reply, active-session consumption, and short malformed prefix disposal. Full target/updater/loader, host and matching companion validation required before hardware adoption.
