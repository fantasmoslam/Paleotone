# T-WRECKS

## Current development direction

The installed candidate is working, but Dylan wants stronger dinosynth identity, explicit instrument voice selection and on-demand dinosaur calls, plus an aggressive bass-and-marching-drums scene. Implementation is paused for the [dinosynth research](docs/TWRECKS-DINOSYNTH-RESEARCH.md) and [revised completion plan](docs/TWRECKS-NEXT-PLAN.md). The descriptions below document the installed candidate, rather than the final musical target.

A dark, sparse generative dinosynth for the M-VAVE FM-1. The factory bank contains only the twenty dinosynth presets below. Fresh startup is EXTINCTION. Previous project formats reset to the new bank, as requested; no old-preset migration or original factory pieces are included in device firmware.

## Controls

Home shows four track rows at a time. SELECT chooses Drone, Sequence, Lead, Sequence 2, Atmosphere, Skin or Calls. The four right-hand encoders edit the selected row: Level, Filter/tone, LFO depth (Chance on Skin/Calls), Reverb send. Values are the approved amber/cyan/pink/green Paleotone Pac-Man fill indicators, with readable labels and no numeric parameter readouts.

HOME cycles the track overview, generative controls and key/tempo. The layer buttons select their visible control rows. SAVE cycles Atmosphere, Atmosphere sound, Atmosphere motion, Skin drums, Creature calls. On Home with Calls selected, ALGORITHM chooses the creature; it also does so on the Creature calls page. PRESETS turns through the twenty scenes. GLO held half a second saves edits; held longer resets the current preset to its recipe. Performance retains its existing key controls. A solo also silences the two auxiliary layers.

## Audio

Paleotone provides the existing curated voices and eight SAURUS creatures: Herd, Raptor, Hoot, Moan, Nest, Croak, Hadro and Snort. Calls are mono, share the melodic eight-voice budget, use a three-second gate and level trims measured from renders. A call is considered only at a bar line, with four to eight bars between possible calls. Chance zero disables new calls.

Skin uses the exact BOOM, SKIN and LOG models from Paleotone's TRIBE kit. Three compact drum states avoid adding another large synth arena. The scheduler considers one earth-drum strike on the downbeat, an occasional skin strike later in the bar, and a log strike on some fourth bars: at most three hits in a bar, with no four-on-the-floor pattern. Chance zero disables new hits. Hold stops new auxiliary events; overload suppresses new events above 75% reported audio load. Level zero clears that layer's voices.

## Factory bank

| Preset | Main character |
|---|---|
| EXTINCTION | Deep tundra drone, sparse geode tones, distant herd |
| STAMPEDE | Earth drums, log percussion, tar bass, snorts |
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

## Storage and build

Twenty edited presets occupy two independently atomic ten-preset A/B banks at 0x98000..0x9BFFF. Globals keep their original pair at 0xFC000 and panel calibration remains at 0xDC000. External sample storage is untouched. CRC validation, torn-write recovery, bank isolation and save/reload behavior have host tests. Only changed banks are rewritten. Old ten-piece recipes exist solely as host regression fixtures; a compile guard prevents including them in an FM-1 build.

Source branch: twrecks-completion. Product identity remains FM-1_942. CI runs host checks, builds the package and publishes candidate output to twrecks-completion-output. The known installer in the PC's twrecks-install directory remains the installer to use.
