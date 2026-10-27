# ROAR oscillator experiment hardware validation

Candidate source 321cd55ddf444578de6e8393bb28ffca4a899c3b, renderer 3291600579, package SHA256 86ee83f7f8b3f9227bb98378978ab44a032061b2c1fa44d963d8da6174193d9e, 610098 bytes. Based on installed dispatch fix81bae2a.

Only four ROAR saw call sites use a local exact helper. Target disassembly confirms no osc_saw/blep/gurdy_saw calls in gurdy_render; total sample-loop calls8 ->4. Image549736 ->550076 (+340), staticRAM96184 unchanged, pool332900 unchanged. Unchanged static budget passes: gurdy_render cost323 ->450 (budget548 plus existing tolerance). The higher static count includes formerly out-of-line arithmetic; it is not measured runtime cost.

Actual production functions passed one million random plus126 branch/wrap boundary comparisons at -Os and -O2 with attributes intact. Other engine helpers are unchanged. Target run40483574832/job122120620848 passed after a compiler-download SSL failure on the first attempt; retry changed no source. Companion run40483574741/job122119655339 passed. Host40483574837/job122119655688 passed the full suite, including concurrent/race checks and213 unchanged sound goldens. Host runtime instruction counters were unavailable/zero and are not measured runtime budget evidence. Candidate is not installed.

Fresh baseline on81bae2a, core ON, same84 BPM scene: aggregate instrument means during USB capture303.4978 and303.4356 us/32 frames; short playing windows329.0387 and328.5642. USB captures each30 seconds, zero underruns; surrounding~34.6-second intervals late1/1, voices shed23/24. Per-track profiling is intentionally disabled with worker ON, so track rows are not evidence of zero cost. Candidate will be compared on aggregate instrument time and overload counters; do not claim a ROAR-only percentage from that aggregate. Live windows are not sample/phase aligned.

Private baseline paths: work/paleotone-midi-dispatch/hardware/roar-baseline-1793094619144 and roar-baseline-repeat-1793094808866. Trial: work/paleotone-roar-oscillator/hardware/trial-1793094794735. Repeat: hardware/repeat-1793094927826. Guarded rollback to81bae2a prepared in hardware/rollback-dispatch. All raw backups stay private.

Matching candidate page https://fantasmoslam.github.io/Paleotone/webapp/screen-roar-oscillator/. Existing installed screen-midi-dispatch remains available. Draft PR20 https://github.com/fantasmoslam/Paleotone/pull/20 . No public installer update or merge.

Target stack prologue check: gurdy_render local reservation284 ->324 bytes (+40), with the same saved-register prologue. Static RAM allocation and fixed stack reservations are unchanged; this is not a measurement of total runtime stack high-water. ROAR remains excluded from the worker.

## Hardware blocked before any write

Install attempt stopped during discovery: no FM-1 MIDI input/output answered. No backup or firmware transfer began. Serial enumeration returned no COM ports, including no COM6; WinMM scan returned ports:[] for the allowed FM-1/Felucca names. Windows still lists FM-1 Bluetooth/MEDIA entries, which do not establish a USB MIDI/serial connection. No port reassignment was found.

Last verified installed source remains81bae2a3b9a0e2157e7a781ff7daa9015c3d279f, renderer3177467445, package4507db3422c64a380fe43f9fbde96991243405cf334b536126e0e8af06295773. Its baseline runs finished stopped, profiling OFF, core ON. Current device state cannot be freshly read while USB is unavailable. No candidate speedup or adoption decision is established.

Next physical step: reconnect/power on FM-1 USB so its MIDI ports and serial console enumerate. Then verify baseline identity/state, run the prepared fresh-backup guarded install, measure candidate twice using run-profile.mjs in trial/repeat directories, compare aggregate timing and overload counters, and verify all13 saved objects. Reject/restore via rollback-dispatch if there is no useful gain. Use screen-roar-oscillator only after the candidate identity is installed; currently use screen-midi-dispatch. No permission question is pending; hardware presence is the blocker.
