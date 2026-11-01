# MIDI path trace candidate

Adds 13 diagnostic 32-bit counters (52 bytes before alignment) to existing serial status. RX fields count SysEx starts, starts rejected while the single frame is occupied, accepted frames and their last command/argument. Foreground fields count command 29/96 handling, reply enqueue attempts/completions, and send-timeout/last-failed-command. Completion means queued into the USB-MIDI ring, not host receipt. Status fields are individually atomic, not a coherent snapshot; sample at quiescent boundaries. No counters are reset during operation.

No sound equations, queue dimensions, MIDI behavior or persistent settings change. Host test exercises actual SysEx assembly and sender success/timeout paths. Hardware trial must correlate request failures with counter deltas and native fragment logs. Full exact-source gates and matching companion required before installation.
