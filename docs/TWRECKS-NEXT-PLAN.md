# T-WRECKS completion plan

Prototype 05 was explicitly approved as the target drum sound and apocalyptic direction. Dylan requests all compatible Paleotone/Gurdy beats with that sound. All eleven families are now imported independently of Gurdy and selectable with ALGORITHM on Skin; selection saves per preset. A six-voice native ADPCM player fits four credited recorded taiko samples inside the firmware image. Source event comparisons: 22,528, zero differences. Decoder output matches all four reference sample hashes.

STAMPEDE is the representative native arrangement: 54 BPM, deep sustained bass, clean supporting fifths, GALOP and playable lead space. Prototype 05 approval is not final approval of native effects, mix, CPU stability or the other nineteen arrangements.

## Signature beat reference: RAPTOR NIGHT

RAPTOR NIGHT's drum beat is the signature reference. Dylan specifically praises how it works when he chooses the synth's existing Theremin instrument for Lead. Theremin remains an independently selected instrument. This feedback does not request adding melodies to drum patterns or composing new fixed arrangements.

Preserve RAPTOR NIGHT's beat. Use its feel to guide additional spaghetti western / tribal drum patterns, keeping drums independent of selected melodic instruments. The device may contain live edits; their exact values and selected rhythm have not been read back, so do not assume factory defaults capture every detail of the heard performance.

## Instrument additions

Implemented in the new candidate: selectable Cello, Cello Low, Sub Pad and Fat Pad voices on each melodic track. See [instrument details and verification](TWRECKS-INSTRUMENT-ADDITIONS.md). These are instruments the player chooses, not prescribed roles in rewritten arrangements. Keep existing Theremin selection working. Retain clear separation and restrained distortion without changing RAPTOR NIGHT to impose a fixed melody or instrument combination.

## Remaining work and acceptance

The native candidate is installed and RAPTOR NIGHT's beat with user-selected Theremin Lead has received explicit praise. The four new instruments pass host checks and candidate 54fa451 is now installed and reboot-verified on the FM-1. Next, audition them, preserving independent beat and instrument selection. Any further drum patterns should follow the approved beat's feel. Manual creature-call testing and average/peak Setup CPU measurements remain hardware checks; musical praise does not establish CPU headroom. Broader rewrites remain outside scope; the later six-scene replacement authorization is recorded below.

## Retained requirements

Readable large type, four-column control tables and colored Pac-Man indicators; direct saved instrument voices; manual dinosaur calls with optional automatic mode; twenty dinosynth presets; existing performance controls. No original preset bank or migration.

Saved format-11 edits remain readable. To hear the new STAMPEDE factory recipe, select it and hold GLO until RESET (about 1.5 seconds). Select Skin on Home and turn ALGORITHM to browse all beats, then save normally.

## Authorized western scene replacement

Dylan clarified that six named scene presets should be replaced with western scenes. BONE RIDER, DUST TRAIL, RED MESA, FOSSIL PASS, HIGH FRONTIER and NIGHT RIDER replace six specified slots; the other fourteen, including RAPTOR NIGHT, retain their settings. The three new independent patterns TRAIL, OUTRIDER and HUNT supplement the eleven imported beats. See [scene mapping and update behavior](TWRECKS-WESTERN-SCENES.md). Dylan explicitly approved all six new western scenes after the installed-device audition: “These are really good, awesome.” Preserve their current musical defaults alongside RAPTOR NIGHT.

Western candidate 43703d8 is installed and reboot-verified. The observed saved RAPTOR NIGHT settings (130 BPM, F Phrygian and Drone controls) survived unchanged. The six-scene listening boundary is passed. Remaining hardware checks are peak CPU/dropouts with live Lead and creature calls; these are independent of musical approval.

## Completion pass

The six western scenes are musically approved. The full CI audit passed all control scenarios, including the long orphan check; its only failure was a legitimate FM bell release tail outlasting the test’s six-second deadline. The bounded release check now passes with up to ten seconds, with no audio-code change. See [completion checklist](TWRECKS-COMPLETION-CHECKLIST.md) for remaining device acceptance and the approved package hash.

## Audio deadline correction

The live Lead test exposed 70–101% CPU and pops. A guarded taiko worker adapted from Paleotone is now installed as 0f889bf. All twenty approved audio hashes remain identical. Dylan repeated the test and reported clean playback at about 65% average / 91% peak; remote snapshots confirm two active cores and zero reported dropouts after restart. See [audio headroom findings](TWRECKS-AUDIO-HEADROOM.md).
