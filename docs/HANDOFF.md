> Historical record. Current integration: [UNIFIED-SCAPE.md](UNIFIED-SCAPE.md).

# DINOBOX: project summary and handoff (2026-10-23)

## What it is

DINOBOX is a dinosaur-themed firmware for the **M-VAVE FM-1** pocket synth: a fork of GHOULBOX 2.3.2 (Jason
Persinger), itself built on Felucca 1.4.1 (Leo Kuroshita, Hügelton Instruments). GPL-3.0-only. Owner: Dylan
(GitHub `fantasmoslam`, Windows only, prefers not to touch GitHub himself).

- **Repository:** `github.com/fantasmoslam/Paleotone` (**private**), branch `main`. Commits are authored as
  `fantasmoslam <dylan.peter.spaniel@gmail.com>`.
- **Builds:** GitHub Actions only (`.github/workflows/build-firmware.yml`); the JieLi toolchain and SDK can't be
  fetched locally. Every push to `main` builds; the result (`dinobox.fwsc`, `build.log`, `felucca.bin`, `ota.bin`,
  `felucca.dis`, `RESULT`) goes to the **`build-output`** branch.
- **Last firmware delivered and installed on the owner's FM-1:** commit `81e2f1b` (2026-10-23; owner confirms it works).
  Includes the Active Sensing clock-fallback correction and the MIDI output controls described below. Before it:
  `db51439` (clock fallback, track selection and track colours; image 547,504 B), then `945e00c` (all host tests passed, GitHub
  build OK). Image 547,104 B in an app slot of ~581 KB; the build refuses less than 16 KB spare (about 18 KB of
  room left above that floor).
- **Ground rules from the owner:** no copyrighted film audio, logos, lettering or screens (all sound is synthesized
  or CC0); no Sloop code; ask before big changes when he says so; he judges by ear, so send previews (MP3) before
  building anything into the firmware.

## How to install (Windows)

The FM-1 Firmware Hub and Felucca's web installer only take their own packages, so DINOBOX installs with
`tools/fm1_install.py` (sent to the owner):

```
winget install Python.Python.3.12          (once; 3.13 / 3.15 have no python-rtmidi wheels)
py -3.12 -m pip install --only-binary :all: mido python-rtmidi
cd $HOME\Downloads
py -3.12 fm1_install.py dinobox.fwsc       (check `dir dinobox*`: the browser renames repeats "dinobox (1).fwsc")
```

Rescue: hold **OCT−** while switching on (USB rescue mode), then install again (or M-VAVE's `FM-1.fwsc`).

## What has been done

### Step 1: the interface (commit `881dde3`)

A "1993 park-operations terminal": the TERMINAL palette (phosphor green, amber accents) as the default, JUNGLE and
VOLCANO beside GHOULBOX's; green-screen windows with scanlines and amber corner brackets (`firmware/src/gfx.c`
`cv_stone`, `cv_window`, `cv_sword`); a prompt cursor; electric fence posts with blinking lamps beside HOME's scope
(`ui_graph.c` `torch_draw`); Share Tech Mono (SIL OFL) as crisp pixel type (`tools/gen_aa_font.py` preset
`dinobox`); a Dilophosaurus skull and DINOBOX title on the boot and ABOUT screens (`tools/gen_ghoulbox_art.py`); new
icons (`tools/gen_aa_icons.py` OWN: T. rex head, reptile eye, sauropod). GHOULBOX's GURDY engine is shown as **ROAR**,
DREAM as **EPOCH**.

### Step 2: SAURUS, the creature voice (engine 15, `firmware/src/eng_saurus.c`)

One note is one call: a buzzing throat (saw with subharmonic and jitter), breath, three throat formants, GROWL's
drive. Controls CALL (HOLD BELLOW ROAR BARK HOOT TRILL CHIRP MOAN HISS SNORT) SIZE SWOOP LEN | GROWL BRTH MOUTH WILD.
Since the rhythms step: **GROWL above 64 adds a rolled-R throat flutter** (~26 Hz, uneven; in over ~0.3 s while the
call is loud, out over ~0.5 s as it drops; in `saurus_amp`, per control tick, no per-sample cost). 20 creatures.

### Step 3: all-new sounds (commits `8aa25fd`, `3f72f06`)

- **107 presets, all DINOBOX's own**; Felucca's and GHOULBOX's stock is hidden (`engines.c` `GB_HIDDEN`, 68 names;
  projects that use them still load and play them).
- **Presets are one table:** `tools/ghoulbox_presets.py` (name, 8 EDIT values, ADSR, filter env, mono, sends,
  suggested pattern) for ANALOG VOICE WHEEL PHYS TRIO LOFI PHASE NOISE ROAR SAURUS GRAIN DRUM FM6. Run it and it
  writes the engines' arrays and `web/editor.html`. Categories: `firmware/src/category.c`; EPOCH roles by name
  words: `firmware/src/dream.c` (`KW`).
- **Sample sets synthesized** (no recordings): `tools/gen_dino_samples.py` makes `assets/samples-cc0/JUNGLE` (night
  jungle loop), `STEPS` (the ENCOUNTER kit, one sound a key from F3: giant step, far step, mud stomp, claw tap,
  branch snap, tree fall, splash, rock fall, leaves, bone clack, ripple, thunder, huff), `STONE` (lithophone),
  `STORM` (downpour loop), in GHOULBOX's four recordings' places (SET 8..11), 12 KB smaller.
- **FM6 patches F9..F15** (`tools/gen_fm6_patches.py`): TAR PIT BASS, SLIT DRUM, MAMMOTH, GLACIER PAD, METEOR,
  CICADA BUZZ, AMBER BELLS. (FM6's AMS made the core clip: avoid `ams` in patches.)
- **The TRIBE drum kit** (`drum_voice.c`, KIT 9, GHOULBOX's CRYPT slot): BOOM SKIN CLAP SEEDS RATTL LOG BONE ROCK.
- **Power-on scenes** (`engines.c` `GB_SCENES`): JUNGLE (default), VOLCANO, STAMPEDE, VALLEY, BLANK.

### The rhythms step (commits `59e870e` .. `945e00c`)

- **ROAR's PULS control** now has 13 drum settings after GHOULBOX's six: DRM4 DRM8 (a deep skin drum on every
  quarter / eighth) and GALOP TRES WAR RAIN QUAKE WAR2 TALK SEVEN PENTA SPIRL CHAOS (rhythms on sixteenths: a low
  drum, a high drum a fifth up, a talking drum bending up; every fourth bar a fill; WOBL adds ghost notes and moves
  each hit's pitch). The drum is at the drone's pitch (DRN), its pitch falling from 2.5x in 40 ms; the strings duck
  under it; no trompette chatter in drum modes. Rhythm bars count from the note-on (hold a note). Code:
  `eng_gurdy.c` (`GURDY_RHY`, `gurdy_hit`, `gurdy_drum_at`; per block, no divides in the sample loop: target cost
  547 of a 548 budget, so any addition there needs the budget looked at).
- **13 rhythm presets on ROAR:** STAMPEDE (GALOP), WAR PARADE, JUNGLE CAMP, RITUAL FIRE, TITAN MARCH, HEARTBEAT,
  HERD RUN, TRIBAL WAR, TALK DRUMS, ODD RITUAL, FIVE MOONS, SPIRAL, CHAOS DRUMS.

### Found on the hardware

With **CLK on USB** and the FM-1 plugged into a PC that sends no clock, the rhythms froze into a flat drone (they
take their tempo from the last external clock). Workaround: set CLK to INT (HOME > CLOCK). **Fixed** by the
clock fallback (docs/NOTES.md "External clock"): PLAY with no clock coming runs on the internal tempo.

### Detour: demo music (not firmware)

- `tests/dino_song.c` **"TITAN RISING"**: a song composed through the firmware itself: four project slots (A..D),
  an 8-section song chain with lanes and repeats, a SAURUS track that changes creature by automation (brachiosaur,
  raptors, tyrant), half-time WAR DRUMS. `DS_SOLO=k` renders one track (levels measured under the limiter).
- `tests/stem.c` renders any preset's part through the firmware, dry, from a text score.
- `tools/biome_preview.py`: the 8 biome previews (procedural water, wind, insects, frogs, birds, lava ... plus the
  firmware's SAURUS and the ENCOUNTER kit).

## Files delivered to the owner

`dinobox.fwsc` (the installed firmware), `fm1_install.py`, sound previews (`growl_preview_3.mp3` was approved),
`rhythms_and_growl.mp3`, `jungle_drums_v2.mp3`, `stampede_emulator.mp3`, `dinobox_biomes.mp3`,
`dinobox_titan_rising_v3.mp4` (+ `_with_jungle.mp3`). A sound
gallery page for step 3 was built but not published.

## Working on it

```
python3 tools/build.py --gen-only                  # the generated headers (fonts, icons, samples, FM6, tables)
HOST_ONLY=1 sh tests/run_tests.sh                  # the whole host suite, ~40 min; must end "ALL HOST TESTS PASSED"
GOLDEN_UPDATE=1 build/host/regress tests/golden.txt tests/cpu_baseline.txt      # after an intended sound change
GOLDEN_UPDATE=1 build/host/union_test tests/union_golden.txt
python3 tests/target_budget.py <build-output>/felucca.dis tests/target_budget.txt  # the render loops' cost (CI runs it)
```

Things that broke before and what fixes them:
- Compile host tests with `-O1` at least (the HAL's inline asm fails at `-O0`).
- After changing presets: `tools/ghoulbox_presets.py`, then `tools/gen_fm6_patches.py --js` into `web/editor.html`
  (its `INIT_PK` / `FACTORY_PK`), `tests/descdump.c` > `build/host/desc.json`, then `node web/test_web.mjs`.
- Preset counts are checked in `tests/ui_test.c` (`99u + ENG_SAURUS.npresets` visible), categories per engine in
  `category.c` (lengths must match), DREAM roles in `tests/dream_test.c` (the scenes), the drum kits' measured
  targets in `tests/drum_test.c` (`KTGT=1` prints them), PHYS demo phrases in `tests/phys_test.c` (one a preset).
- An engine's cost budget (`tests/target_budget.txt`) is measured on GitHub's build only.
- To hear the real firmware without the hardware: `fm1-emu` (github.com/joakimeriksson/fm1-emu, Rust; `cargo build
  --release -p fm1-emu`, then `fm1-emu --fwsc dinobox.fwsc --ms 8000 --wav out.wav --keys "1500:PRESET:11,2500-7500:D4"`
  renders a key script; ~3.5 min for 8 s).

## What comes next (agreed order)

1. **The clock fallback** (small): **done** (`midi_clock.c`, `seq.c`; docs/NOTES.md "External clock"): with no
   clock for a beat the transport runs on the internal tempo; the next pulse takes it back. In the next firmware.
2. **The soundscape maker: dropped by the owner (2026-10-23, too costly).** The generator and its approved previews
   are kept on the branch `scape-preview` (`firmware/src/scape.c`, never on the device). The plan as it was:
   - an ambience layer beside the four tracks (a SCAPE fader in the MIXER, its own send), saved with the project,
     following the tempo and the EPOCH key;
   - 8 layers, each pointable at any sound, with level, density and distance; the drone on its own page (type, key,
     movement); globals: time of day, danger, weather, density, biome;
   - biomes: WATER, JUNGLE, TUNDRA, FOREST, PLAINS, MOUNTAINS, CANYONS, CALDERA (previews done); water sounds made
     procedurally on the device (stream, waves, drips, bubbles, splashes);
   - generative encounters (steps approach, the jungle hushes, a growl, a roar, the steps leave);
   - **feedback to honour:** the previews were far too loud and busy: it is meant to be *ambient*: quiet by
     default, sparse, wind ~8 dB lower than the previews; every layer's level adjustable;
   - flash: ~18 KB above the floor, so procedural synthesis, not new samples; voices: shares the 12 with the tracks.
3. **Easier track selection** (original step 4): **done**, approved and installed (commit `db51439`, image 547,504 B). HOME held + F3 G3 A3 B3 (the first four white keys, GLO's SOLO keys): track 1..4 at once, the key
   silent, HOME then neither changes the page nor opens the menu (`ui_input.c` `home_keys`, `ui_layer.c`
   `layer_masks`). Each track its colour: the selected track's is ACCENT (badge, corners, cursor, playhead, lamps;
   `gfx.c` `ux_track`), the MIXER and HOME TRACKS show each track's; T1 keeps the palette's ACCENT, T2..T4 three
   hues per palette checked for contrast as ACCENT (`tools/gen_ui_palettes.py` `track_colours`, `UI_TRK_ACCENT`);
   GREY and MONO one colour. Tests: `tests/ui_test.c` `test_home_track_keys`.
4. **MIDI** (original step 5): built on the branch `midi-out`. MENU > MIDI: T1..T4 OUT (OFF, AUTO = the channel in
   MIDI IN's block as the keys always had, CH 1..16) and CLOCK OUT (OFF / ON). Device settings, not the project's (the
   project format has 2 bytes left): bytes 1..5 of the retired DIGITAL engine's favourites (`menu_items.c` `MO_BYTES`).
   The keys and now the sequencer's notes (steps, chords, ratchets, DRUM lanes as GM notes) go out on each track's
   channel, USB only (the TRS jack is an input); every note gets its note-off (STOP, panic, mute, a channel change:
   CC123 on the old one). CLOCK OUT: 24 a quarter with CLK INT, always; START with PLAY, STOP with STOP; none on an
   external clock. Code: `seq.c` `mo_*`, `seq_on` / `seq_off`; editor ids 26..30. Tests: `tests/midi_control_test.c`
   `midi_out_test`.
5. **Each step:** previews to the owner first, then the full host suite, the GitHub build, the firmware sent with
   short Windows install notes.

Optional: turn TITAN RISING into a project the owner can load; publish the sound gallery.

