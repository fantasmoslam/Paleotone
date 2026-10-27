# ROAR oscillator experiment: not adopted

Candidate source321cd55ddf444578de6e8393bb28ffca4a899c3b, renderer3291600579, package SHA25686ee83f7f8b3f9227bb98378978ab44a032061b2c1fa44d963d8da6174193d9e,610098 bytes. Based on the verified MIDI dispatch fix81bae2a. Trial completed; previous build restored.

## Change and software validation

Only four ROAR saw call sites used a local always-inline helper with the original phase wrapping, integer division and branch precedence. Other engine helpers, RNG order, state, voice limits, USB queue targets and worker policy were unchanged. Target disassembly confirmed no osc_saw/blep/gurdy_saw calls left in gurdy_render; sample-loop calls8 ->4.

Image549736 ->550076 bytes (+340); staticRAM96184/98304 and pool332900/344064 unchanged. ROAR local stack reservation284 ->324 bytes (+40), same saved-register prologue. Fixed stack reservations unchanged; this was not total runtime stack high-water measurement. Static loop cost323 ->450 within unchanged budget548 plus existing10% tolerance; it includes formerly out-of-line arithmetic and is not runtime timing.

Actual production functions passed1000126 comparisons at each of -Os and -O2 with attributes intact. Host run40483574837/job122119655688 passed all existing checks,213 unchanged sound goldens,61 concurrent scenes and ThreadSanitizer. Host runtime instruction counters were unavailable/zero and are not runtime CPU-budget evidence. Target run40483574832/job122120620848 passed after an external compiler-download SSL failure; retry changed no source. Companion run40483574741/job122119655339 passed.

## Matched hardware comparison

USB was briefly absent before the first install attempt, which ended before any write. On reconnection the live tempo was91 BPM. A fresh baseline was captured at those settings. Candidate identity verified2026-10-27T10:20:20.678Z.

The restart reloaded84 BPM and one other serialized live value (consistent with loading an older saved state). Archive checks caught this: saved objects1..9,32..34 were unchanged; live object0 differed at offsets8 and173 plus its checksum. The two initial84 BPM candidate runs are excluded from the adoption comparison.

Using the existing validated backup protocol, restored ONLY live object0 from the fresh pre-install archive. This applies project_restore_runtime without writing saved project slots. Fresh capture then matched all13 original objects exactly by size, CRC and encoded data. Two new candidate measurements asserted91 BPM. Same loaded scene and core ON throughout the comparison.

| Matched91 BPM metric | Baseline81bae2a | Candidate run1 | Candidate run2 |
| --- | ---: | ---: | ---: |
| Playing instrument stage mean, us/32 frames | 325.3079 | 324.0174 | 321.7656 |
| USB-window instrument stage mean, us/32 frames | 303.3864 | 302.3967 | 302.1112 |
| USB-window instrument stage max, us | 619 | 650 | 619 |
| New late blocks in surrounding interval | 2 | 3 | 1 |
| New voices shed in surrounding interval | 26 | 27 | 27 |
| USB packet underruns during30-second capture | 0 | 0 | 0 |

Intervals were about34.7 seconds around each30-second capture. USB-window stage means improved only0.33% /0.42%; short playing windows0.40% /1.09%. These are aggregate instrument costs on the main processor, not isolated ROAR percentages or whole-device gains. Per-track profiling is disabled with core ON. Windows were live, not phase aligned. Neither late blocks nor overload cuts consistently improved. No worker faults/fallbacks or LCD timeouts were observed.

## Decision and verified rollback

Not adopted: less than half a percent in the longer instrument-stage windows, no reduction in overload cuts, plus extra code/local stack and duplicate oscillator maintenance do not justify retaining this experiment. The cause of the modest gain was not isolated; register spills/cache effects remain hypotheses. Draft PR20 is closed without merging; branch and tests retained for reference.

Restored package4507db3422c64a380fe43f9fbde96991243405cf334b536126e0e8af06295773. Identity verified2026-10-27T10:31:10.531Z: source81bae2a3b9a0e2157e7a781ff7daa9015c3d279f, renderer3177467445, schema2,82 globals. Restored the captured live object0 after this restart too. Final fresh archive matches ALL13 original pre-update objects in size, CRC and encoded data. Final serial: stopped,91 BPM, second-core preference and active worker ON, idle CPU30%; fresh profile query profile_on0. The MIDI dispatch fix and USB margin remain installed.

Current matching display: https://fantasmoslam.github.io/Paleotone/webapp/screen-midi-dispatch/ . The screen-roar-oscillator page is only for the rejected candidate. No public installer firmware change or merge.

## Evidence and next work

Private Windows root C:/Users/Dylan/Documents/Codex/2026-10-25/new-chat-9. Fresh91 BPM baseline: work/paleotone-midi-dispatch/hardware/roar-baseline-reconnected-1793096236987. Candidate artifacts, pre-install archive, initial mismatched captures and matched-comparison.json: work/paleotone-roar-oscillator/hardware/trial-1793094794735. Matched runs: hardware/matched-91-1793096782836 and matched-91-repeat-1793096878282. Rollback identity, restore-live script, restored-live-backup.json, saved-state-comparison.json, final-status.txt and final-profile.txt: hardware/rollback-dispatch. Keep raw backups private.

Next useful performance investigation is making an expensive currently excluded engine safe for the second core, beginning with a ROAR shared-state/clock/crank audit, rather than adopting this marginal inlining result. This requires snapshots/ownership work and race/sound validation; do not simply change the engine-eligibility flag. Also add live-state preservation across updater restarts to the installer backlog: this trial demonstrates why the pre-update archive must be verified/restored as needed. Neither enhancement was implemented here.
