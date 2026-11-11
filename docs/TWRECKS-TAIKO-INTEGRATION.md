# Native taiko and Gurdy rhythm import

All eleven Gurdy beat families are selectable independently of the melodic voice: GALOP, TRES, WAR, RAIN, QUAKE, WAR2, TALK, SEVEN, PENTA, SPIRAL, CHAOS. Seed-zero source event logic passed 22,528 comparisons with no differences, including ghosts and fourth-cycle fills. Four compact recordings use six concurrent ADPCM players. Assets retain their separate CC BY-SA 4.0 notice; firmware code remains GPL-3.0-only.

Select Skin on Home, then turn ALGORITHM. Save normally to retain each preset's beat. Ghosts changes optional accents; zero retains the core grid. STAMPEDE's factory recipe is 54 BPM with sub bass, supporting organ fifths and no automatic lead melody. Hold GLO until RESET to load it.

## Verification, 2026-11-11

- Exact decoded PCM hashes for all four samples; all eleven rhythms audible and finite in host renders; overlap and panic checks passed.
- Twenty production audio baselines repeat exactly; ten host-only legacy baselines unchanged.
- All twenty arrangements finite with measured headroom; STAMPEDE RMS 0.0955, peak 0.541 in the tested render.
- Saved beat round trip, old format-11 header loading and 35 control scenarios passed. The separate long-running orphan scenario was not rerun for this change.
- Hardware image 576,648 / 581,564 bytes; static RAM 91,972 / 98,304; pool 333,448 / 344,064.
- OTA, loader and installer checks passed, including loader write bounds and recovery cases.

Device CPU/dropouts and native musical balance still require hardware listening. The other nineteen arrangements have not received final musical acceptance.
