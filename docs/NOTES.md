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
  saved with the project, locked to the tempo and the EPOCH key.
- 8 layers, each pointable at any sound (SAURUS, the sample sets, ROAR, ...): drone, insects, frogs, birds,
  rain / wind, distant creatures, footsteps / encounters (approach and pass), thunder. Per layer: level, density,
  distance.
- The drone on its own page: type, key (C..B or follow EPOCH), movement.
- Globals: time of day, danger, weather, density; a biome picker.
- A layer can be sent to a track (a SCAPE choice on a track) to sequence it.
- Voices: shares the 12; the quietest layers give way first.

## The growl (agreed from growl_preview_3)

- A rolled-R flutter ~26 Hz, a little uneven, a rough sub rumble under it.
- In over ~0.3 s, out over ~0.5 s once the call drops: gone before the bellow ends; the bellow keeps its hall.
- Predators: TYRANT ROAR, MONSTER ROAR, LOW GROWL, BABY REX, TITAN APE (REX ROAR, REX GROWL in other engines).
