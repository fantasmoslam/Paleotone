# SCAPE: shared music and habitat

HOME cycles MIXER, CLOCK and KEYS, then every SCAPE page in one continuous group before returning home. The first three pages cover movement, role mixing and habitat; keep tapping HOME for setup and detail.

| Page | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- | --- |
| SCAPE MUSIC | ACTV: note density | EVOL: variation speed | MOTN: breathing depth | HOLD: chord time |
| SCAPE MIX | Bed | Melody | Bass | Pulse |
| SCAPE | Biome | Day/night | Distance | Creature call activity |

Choose sounds first. Open SCAPE MUSIC and press a physical key to compose in that key. OCT+ confirms replacement of existing patterns; OCT− cancels. Composing enables evolution and starts playback. These remain ordinary editable sequences using the existing twelve-voice budget.

The musical roles use independent 1/8-note loops: bed 29 beats, melody 13, bass 23, pulse 17. BEAST uses a held 64-step phrase. They follow one four-chord progression; HOLD selects 20, 30, 45, 60, 90, 120, 180 or 300 seconds. Chord changes transpose existing notes, preserving their rhythms and velocities. Held notes finish naturally; new notes adopt the new harmony. AUTO drone follows the current chord. A separate fixed drone key remains available.

MOTN gives layers separate breathing periods inside a shared two-minute sparse/full arc. Zero removes breathing. Mix controls attenuate each role without rewriting track levels, with smoothed gain changes. Bed also controls the held BEAST role. ACTV changes phrase density on the next loop; EVOL changes later material on each track's own loop boundary, keeping its opening motif. The pulse distributes hits across its loop. Melody notes leave real rests; new calls and wildlife events wait for these gaps. Calls follow the current chord root. Existing calls finish naturally.

## More settings in the same SCAPE group

Keep tapping HOME after the habitat page for SETUP, ROLES, DRONE, LIFE and the two layer-level pages. All SCAPE settings stay together; GLO remains SONG. Normal PLAY, REC and instrument editing keep their existing behavior.

| Page | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- | --- |
| SCAPE SETUP | Evolution on/off | Mood | Environment level | Extra drone level |
| SCAPE ROLES | T1 role | T2 role | T3 role | T4 role |
| SCAPE DRONE | Extra drone level | Fixed key / AUTO | Drone movement | Creature pitch |
| SCAPE LIFE | Creature track / OFF | Passing-beast activity | Beast size | Creature pitch |
| SCAPE LEVELS | Wind | Water | Rain | Insects |
| SCAPE LEVELS 2 | Frogs | Birds | Drips | Rumble |

Roles: AUTO, BED, MELO, BASS, PULSE, BEAST. AUTO uses T1 bass, T2 bed, T3 melody and T4 pulse, with SAURUS/ROAR as BEAST and drum kits as PULSE. Kits always use PULSE. No preset-name guesses. Changing a role takes effect on that track's next loop. Selecting a different scene clears the cached director.

Evolution OFF freezes progression, variation and breathing clocks; playback and layer levels continue. Stopping transport pauses generation. Song-chain playback suspends the director. With no active composition the mix is an exact bypass. Environment-only use remains available: raise the environment level on SCAPE SETUP; creature track OFF prevents track-triggered calls and roars.

Expanded companion: HOME+OCT− selects the left page, HOME+OCT+ the right. An effective pairing is SCAPE MUSIC on one side and SCAPE MIX on the other; both use the real firmware renderer.

## Storage and compatibility

Existing globals 0..68 and track parameter IDs are unchanged. Ten new globals 69..78 occupy previously unused bytes 33..42 of each existing 48-byte SCAPE settings sidecar. Old tails load defaults; project slots A–D and autosave retain new values. The sidecar is included in a full editor backup; a standalone legacy project payload does not contain these SCAPE settings. The transient chord position and phase restart after loading.

SLICER is retired in this candidate. Its four stored track slots remain reserved and ignored by DSP. Repeat, reverse, tape stop, freeze, octave, flanger and phaser remain. Their dedicated 32 KiB buffer remains allocated. Older firmware can still be managed by the editor using its legacy layout.
