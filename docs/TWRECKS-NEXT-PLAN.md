# NightClaw completion plan

## Current checkpoint

Installed firmware source: **f9b11d6**. Its [full CI run](https://github.com/fantasmoslam/Paleotone/actions/runs/43139217653) passed. The published package matches the installed package by SHA-256; see the [completion checklist](TWRECKS-COMPLETION-CHECKLIST.md).

Dylan reports “Works much much better” after the physical-detent correction. This is the accepted control response: one full detent produces one item or parameter step, with no velocity acceleration. Earlier progressive-response tuning is superseded.

## Completed and to preserve

- Twenty dinosynth scenes, including six approved western replacements. RAPTOR NIGHT with user-selected Theremin Lead remains the signature reference.
- Fourteen independent beats: eleven Paleotone/Gurdy families plus TRAIL, OUTRIDER and HUNT. Beat selection stays independent of melodic instruments.
- Cello, Cello Low, Sub Pad and Fat Pad appended without moving existing voice IDs.
- Eight manual creature calls, with optional Auto and manual mode at boot.
- Saved beat/instrument choices, compatible format-11 headers, scoped scene migration and interrupted-save recovery.
- Guarded second-core taiko playback. The formerly overloading HIGH FRONTIER live-Lead test played cleanly at about 65% average / 91% peak on the tested device.
- Approved saturated bone/crimson/sulfur/violet palette, single NightClaw font family and static jungle title held at least three seconds.
- Home track names at 24–32 px and 36 px gauges; four columns and up to four visible track rows. Preserve these sizes.
- Seven-encoder/four-phase decoder coverage, eleven-cut font checks, all 39 UI scenarios, storage/updater checks and twenty audio baselines.

The sound target remains consequential and apocalyptic: grandiose low percussion, distinct voices, restrained distortion and space for live Lead. No further rewrite of approved scenes is planned.

## Remaining docket

| Work | Current status / next evidence |
|---|---|
| Release preparation | Published candidate and installed checksum agree; documentation and PR describe the current firmware. PR #41 remains open for review. |
| Long physical-device playback | Short live-Lead overload reproduction is accepted. Longer sessions and wider instrument combinations remain unverified. |
| Physical readability | Large Home labels and the single-family correction are installed and pass render/fit checks. Readability across all pages still needs direct user feedback. |
| Final release decision | Review remaining physical results before treating the candidate as a finished release. |

The [72-minute host pass](NIGHTCLAW-ENDURANCE.md) covers twenty scenes and the four appended Lead voices. It establishes host selection/output coverage, not physical-device endurance. No firmware reinstall is needed for the remaining device checks.

## Practical device coverage

During normal use, include RAPTOR NIGHT with Theremin and HIGH FRONTIER with a live Lead, then try the cello and bass-pad voices with drum accents and effects. A longer session should include scene changes and manual calls. Record audible glitches, CPU average/peak, the dropout count and any labels that are hard to read. These are coverage goals, not a claim that they have already passed.

## References

- [Current detent and Home readability correction](NIGHTCLAW-DETENT-FIX.md)
- [Audio headroom measurements and worker adaptation](TWRECKS-AUDIO-HEADROOM.md)
- [Western scene mapping and migration](TWRECKS-WESTERN-SCENES.md)
- [Instrument additions](TWRECKS-INSTRUMENT-ADDITIONS.md)
- [Quick start](TWRECKS-QUICKSTART.md)
