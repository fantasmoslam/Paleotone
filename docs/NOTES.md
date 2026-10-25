# DINOBOX: notes for later steps

## Biomes and water (asked 2026-10-23)

The soundscape maker needs biomes, and sounds to support them, water most of all:

- Biomes to build: jungle (have), swamp / marsh, river, lake shore, coast and surf, waterfall, cave (drips,
  echo), volcano (have some), plains / savanna, cold steppe (ice age).
- Water material to make (synthesized, like `tools/gen_dino_samples.py`): river flow, stream over stones, waterfall,
  surf and waves, lapping shore, swamp bubbles and mud pops, cave drips, rain on water, splashes of several sizes,
  something big surfacing, wading footsteps.
- Each biome: which layers, which sounds, how dense, which creatures, which drone.

## The soundscape maker (planned, after the rhythms step)

- An ambience layer beside the four tracks (not a fifth track): a SCAPE fader in the MIXER with its own send,
  saved with the project, locked to the tempo and the SCAPE MUSIC key.
- 8 layers, each pointable at any sound (SAURUS, the sample sets, ROAR, ...): drone, insects, frogs, birds,
  rain / wind, distant creatures, footsteps / encounters (approach and pass), thunder. Per layer: level, density,
  distance.
- The drone on its own page: type, key (C..B or follow SCAPE MUSIC), movement.
- Globals: time of day, danger, weather, density; a biome picker.
- A layer can be sent to a track (a SCAPE choice on a track) to sequence it.
- Voices: shares the 12; the quietest layers give way first.
- Biomes (previewed 2026-10-23, scratchpad work/biomes.py): WATER, JUNGLE, TUNDRA, FOREST, PLAINS, MOUNTAINS, CANYONS,
  CALDERA. Feedback: wind too loud everywhere: start wind layers ~8 dB lower by default; every layer's level adjustable.

## The growl (agreed from growl_preview_3)

- A rolled-R flutter ~26 Hz, a little uneven, a rough sub rumble under it.
- In over ~0.3 s, out over ~0.5 s once the call drops: gone before the bellow ends; the bellow keeps its hall.
- Predators: TYRANT ROAR, MONSTER ROAR, LOW GROWL, BABY REX, TITAN APE (REX ROAR, REX GROWL in other engines).

## External clock (found 2026-10-23)

With CLK on USB and the FM-1 plugged into a computer that sends no (or a stalled) MIDI clock, ROAR's rhythms
froze into a flat drone. The cause on the host: with CLK on USB or TRS, PLAY waited for pulses that never came
(the sequencer held its first step), and after 500 ms without one the transport stopped.

Fixed (the clock fallback, `midi_clock.c` `midi_clock_quiet` / `_go_free` / `_rejoin`, `seq.c` `events_block`):
- the source is *quiet* when nothing has come from it for a beat (at least 250 ms), or it has said nothing since
  CLK was set;
- PLAY with a quiet source runs at once on the internal tempo; a clock lost while playing (a cable pulled) carries
  on at the last measured tempo (BPM holds it, the BPM knob moves it from there) instead of stopping;
- the source's next pulse takes the transport back from the step it is on; a Start puts it on the master's step 0;
  a Stop stops it as ever;
- stopped, with a source that never spoke, the ARP runs on the internal tempo too (after a Stop from the source it
  waits for the clock as before, so a DAW's stop does not set it going).
Tests: `tests/midi_control_test.c` (the lost clock, PLAY with no clock, the ARP).
