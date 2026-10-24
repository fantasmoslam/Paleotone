# PALEOTONE generator candidate

PALEOTONE is the new name for this DINOBOX firmware fork. The Oviraptor mascot and bitmap wordmark appear for roughly three seconds at startup; watchdog servicing continues during the display.

## Soundscape controls
Hold HOME to open MENU. Choose SYSTEM with ALGORITHM, choose SCAPE with PRESETS, then press OCT+.
- SCAPE: BIOME, LEVEL, DENS, DANGER.
- Tap GLO to cycle WORLD, DRONE, LAYER 1..8, then back to SCAPE.
- WORLD: time of day, weather, key (FOLLOW EPOCH or C..B), reverb send.
- DRONE: type, level and movement.
- Each layer: sound kind, level, event density and distance.
- HOME leaves these pages. Ordinary GLO still opens SONG outside SCAPE.

Fresh/older projects start with SCAPE level at zero. Raise LEVEL to hear it. It continues when the sequencer is stopped; set LEVEL to zero to silence it. DANGER above about two-thirds enables the heavy encounters. They are occasional rather than continuous. Switching BIOME applies that biome's layer and drone defaults.

All four music tracks remain available. Creature calls and procedural events count against the shared twelve-voice limit; music allocations make ambience fade first. If the measured audio load crosses the existing overload threshold, SCAPE fades and pauses before the firmware's music-voice shedding runs.

The initial release offers the prototype's procedural sound kinds and curated biome creature calls. Routing arbitrary music-engine presets into layers is not included.

## Projects and rollback
Stop playback to save a project as usual. Edited SCAPE settings are saved with the music in a versioned FUNB project, still 3840 bytes. All older project formats remain importable. The FM patches are packed losslessly into seven-bit values to make room.

Older firmware cannot read FUNB projects. Keep a backup made with the existing firmware before installing the candidate, and keep its working FWSC file. No existing user project slot is rewritten merely by flashing this firmware. Explicit project saves and autosave use the new version when SCAPE settings need it.

## Installing
Keep the candidate FWSC and the existing fm1_install.py in the same folder:
```powershell
py -3.12 .\fm1_install.py --info
py -3.12 .\fm1_install.py .\paleotone-generator-candidate.fwsc
```
Confirm the installer prompt and let writing finish before disconnecting. Hold OCT- while powering on for USB rescue if needed.

## Hardware checks still needed
The actual device must confirm startup visibility, SCAPE navigation, save/reload, and CPU behaviour with your heaviest four-track projects and USB audio/MIDI. Host rendering and static target budgets do not measure real hardware deadlines. This package is a candidate until that check is complete. The earlier working firmware downloads remain unchanged.
