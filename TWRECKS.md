# T-WRECKS

## Current development direction

The controls/manual-call candidate is installed, but its musical direction was rejected: hollow bass and upbeat/noir character do not meet Dylan's **apocalyptic, consequential** target. The [audio study](docs/TWRECKS-APOCALYPTIC-AUDIO-STUDY.md) measures eleven reference streams and candidate diagnostics, alongside a source audit. The [completion plan](docs/TWRECKS-NEXT-PLAN.md) now prioritizes one desktop musical prototype before further flashing or broad preset revision. The documentation below describes the existing candidate, not an accepted final sound.

A dark, sparse generative dinosynth for the M-VAVE FM-1. The factory bank contains only the twenty dinosynth presets below. Fresh startup is EXTINCTION. Previous project formats reset to the new bank, as requested; no old-preset migration or original factory pieces are included in device firmware.

## Controls

Home shows four track rows at a time. SELECT chooses Drone, Sequence, Lead, Sequence 2, Atmosphere, Skin or Calls. The four right-hand encoders edit the selected row: Level, Filter/tone, LFO depth (Chance on Skin/Calls), Reverb send. Values are the approved amber/cyan/pink/green Paleotone Pac-Man fill indicators, with readable labels and no numeric parameter readouts.

HOME cycles the track overview, generative controls and key/tempo. The layer buttons select their visible control rows. SAVE cycles Atmosphere, Atmosphere sound, Atmosphere motion, Skin drums, Creature calls. On Home, ALGORITHM changes the selected instrument voice, Atmosphere character or Calls creature; the name stays in the header. Skin uses the fixed TRIBE kit. Voice choices save with each preset. With Skin or Calls selected on Home, SAVE opens its control page immediately. PRESETS turns through the twenty scenes. GLO held half a second saves edits; held longer resets the current preset to its recipe. Performance retains its existing key controls. A solo also silences the two auxiliary layers.

## Audio

Paleotone provides the existing curated voices and eight SAURUS creatures: Herd, Raptor, Hoot, Moan, Nest, Croak, Hadro and Snort. Calls are mono, share the melodic eight-voice budget, use a three-second gate and level trims measured from renders. Calls start in Manual mode at each boot. Select Calls on Home, or open Creature calls with SAVE: the first eight white keys trigger Herd, Raptor, Hoot, Moan, Nest, Croak, Hadro and Snort. The first black key toggles Manual/Auto; the second stops the current call. Auto considers a call at bar lines, with four to eight bars between possible calls; Chance zero disables automatic calls. Manual triggers bypass that cooldown and work with music stopped or held. Level zero or an auxiliary mute silences them. Mode is a live choice, not a saved preset parameter. Leaving call mode releases held instrument-key ownership.

Skin now uses four recorded taiko samples with six overlapping playback voices. All eleven Paleotone/Gurdy rhythm families are independent of the Gurdy instrument: GALOP, TRES, WAR, RAIN, QUAKE, WAR2, TALK, SEVEN, PENTA, SPIRAL and CHAOS. Select Skin on Home (or open its page) and turn ALGORITHM to choose a beat; the choice saves per preset. Ghosts adjusts optional accents while retaining the core beat. Source timing, ghost and fill logic passed 22,528 comparisons against Gurdy. This is an event-logic extraction, not separation of a mixed recording.

STAMPEDE defaults to 54 BPM, GALOP, resonant taiko, sustained deep sub bass and clean supporting organ fifths. Automatic lead notes are off, with the keyboard lead ready to play. The recorded drum direction was approved in desktop Prototype 05; native playback and device CPU still need listening validation. Above 75% reported audio load, optional ghost accents and automatic calls are suppressed; core drum hits remain. Level zero clears the sampler. Sample credits and CC BY-SA 4.0 terms are in [the attribution notice](tools/research/taiko-bank-05/ATTRIBUTION.md).

## Factory bank

| Preset | Main character |
|---|---|
| EXTINCTION | Deep tundra drone, sparse geode tones, distant herd |
| STAMPEDE | Resonant taiko, deep sub bass, clean organ fifths, live lead |
| RAPTOR NIGHT | Primeval bed, kalimba, whistle and raptor chatter |
| ASHEN SKY | Star-sky pad, harp and wind lead |
| BONE TEMPLE | Temple organ, tusk tines and sparse skins |
| FERN SHADOWS | Jungle pad, kalimba and nest calls |
| TAR PIT | Earthquake drone, tar growl bass and skins |
| DISTANT HERD | Low tundra, valley harp and herd calls |
| AMBER CAVERN | Resin pad and geode bells |
| PRIMEVAL | Primeval drone, log marimba and croaks |
| FROZEN BONES | Glacier pad, ice crystal and canyon horn |
| VOLCANIC DUSK | Quake, primal stabs, tar bass and drums |
| CRESTED MOON | Choir, harp, whistle and crest hoots |
| SWAMP RITUAL | Swamp fiddle, log bars, skins and croaks |
| STONE SENTINEL | Temple drone, slit drum and tar bass |
| HOLLOW NEST | Ice-age pad, water drops and nest calls |
| MAMMOTH PASS | Strings, tusk tines, mammoth lead and hadro calls |
| ECHOING RUINS | Temple organ, amber bells and cave echoes |
| LAST SUNRISE | Aurora pad, valley harp and canyon horn |
| DEEP TIME | Tundra drone, sparse geode bells and herd moans |

These are first-pass listening candidates. Complete arrangements were levelled together from measured renders, preserving each scene's internal balance. Musical quality, relative call/drum balance and device CPU headroom still require listening and the FM-1 Setup reading.

Existing saved STAMPEDE settings are preserved across an update. Select STAMPEDE and hold GLO until RESET (about 1.5 seconds) to load the revised factory recipe.

## Storage and build

Twenty edited presets occupy two independently atomic ten-preset A/B banks at 0x98000..0x9BFFF. Globals keep their original pair at 0xFC000 and panel calibration remains at 0xDC000. External sample storage is untouched. CRC validation, torn-write recovery, bank isolation and save/reload behavior have host tests. Only changed banks are rewritten. Old ten-piece recipes exist solely as host regression fixtures; a compile guard prevents including them in an FM-1 build.

Source branch: twrecks-completion. Product identity remains FM-1_942. CI runs host checks, builds the package and publishes candidate output to twrecks-completion-output. The known installer in the PC's twrecks-install directory remains the installer to use.

## Additional selectable instruments

The current source adds Cello, Cello Low, Sub Pad and Fat Pad at the end of each melodic voice list. Select a track on Home and turn ALGORITHM to choose one; save normally. These synthesized voices are independent of drum patterns. Existing presets and Theremin remain unchanged. No factory reset is needed. See [instrument validation and installation status](docs/TWRECKS-INSTRUMENT-ADDITIONS.md).
