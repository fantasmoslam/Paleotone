# NightClaw completion checkpoint

Current installed firmware source: **f9b11d61af8c05598089cea8ad5c6360470fe13a**. The envelope implementation is a newer source candidate; its device installation is not yet recorded.

| Item | Evidence / status |
|---|---|
| Musical direction | RAPTOR NIGHT and all six western scenes explicitly approved on the device |
| Physical knobs | Full-detent counting, no velocity acceleration; Dylan reports “Works much much better” |
| UI | Approved saturated palette; one NightClaw typeface family with no original-font fallback; Home names 24–32 px and gauges 36 px |
| Instruments | Cello, Cello Low, Sub Pad and Fat Pad available on melodic layers; existing voice IDs retained |
| Drums | Eleven imported beat families plus three western variations; independent of instrument selection |
| Creature calls | Eight manual calls; optional Auto; manual mode at boot |
| Preset bank | Twenty scenes; six authorized replacements; fourteen other saved slots preserved |
| Storage | Beat/instrument save and reboot, interrupted replacement write/retry and old-header compatibility checks pass |
| Audio regression | Twenty production baselines match in serial, concurrent and partial-worker-failure modes; ten legacy fixtures retained |
| Decoder and UI checks | All seven encoders/four starting phases; eleven font cuts; all 39 control scenarios pass |
| Updater | Entry, loader bounds/recovery and installer protocol checks pass |
| Installation | Installer exit 0; NightClaw status handshake answered with RAPTOR NIGHT playing at 130 BPM |
| Resource fit | Image 544,364 / 581,564 bytes; RAM 95,596 / 98,304; pool 334,472 / 344,064 |
| Full CI | Source f9b11d6 passed run 43139217653 |
| Extended host playback | 72 minutes, 24 selection/output-peak checks passed |
| Envelope editing | Implemented: unified ADSR, native/imported audio shaping, six editable tracks and per-scene persistence. Host/WASM checks pass; physical validation pending. [Behavior and checks](NIGHTCLAW-ENVELOPE-AUDIT.md). |
| Browser / tablet | Physical-panel player and guide published; source 1398257 keypad accepted; browser CI 43537466329 passed. |
| Remaining device coverage | Longer sessions, wider voice/effect combinations and direct readability feedback across pages |

## Verified candidate publication

[Full CI run 43139217653](https://github.com/fantasmoslam/Paleotone/actions/runs/43139217653) completed successfully.

Fixed output commit: **1086a04d6dbda6ca95f6b8cc07ec67a75ca310f2**. Its RESULT identifies the installed source f9b11d61af8c05598089cea8ad5c6360470fe13a with success.

[Candidate firmware](https://github.com/fantasmoslam/Paleotone/blob/1086a04d6dbda6ca95f6b8cc07ec67a75ca310f2/nightclaw-FM-1_942.fwsc) is 609,759 bytes. The published checksum and local installed-package checksum agree:

```text
641cd0a0784ca05baf0b17c6878b479c7a30fcd82f1510a99ffae805f37337ed
```

The candidate remains product FM-1_942 and retains the compatible preset format. PR #41 is open; this is not a final release announcement.

## Hardware evidence and limits

The original HIGH FRONTIER live-Lead test produced pops at 70–101% CPU. The guarded second-core taiko candidate repeated that performance cleanly at about 65% average / 91% peak. Read-only snapshots showed zero reported dropouts or worker fallback after restart. Those observations are evidence for that tested performance, not a worst-case or long-session benchmark.

The latest detent correction has explicit physical user acceptance. Automated render/fit checks pass for the larger labels; direct readability feedback across every page remains separate.

## Detailed records

- [Physical detent correction and installation](NIGHTCLAW-DETENT-FIX.md)
- [Typeface correction](NIGHTCLAW-TYPEFACE.md)
- [Approved palette](NIGHTCLAW-PALETTE.md)
- [Audio-worker measurements](TWRECKS-AUDIO-HEADROOM.md)
- [Host endurance evidence and limits](NIGHTCLAW-ENDURANCE.md)
- [Remaining completion plan](TWRECKS-NEXT-PLAN.md)

Earlier branding, progressive-response and visibility checkpoints remain in their individual historical records. Their binaries, resource figures and pending-CI statements do not describe the current candidate.
