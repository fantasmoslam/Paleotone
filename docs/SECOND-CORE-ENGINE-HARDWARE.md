# Second-core hardware comparison on the engine hotpath build

Test completed 2026-10-27 (timestamps in raw logs are UTC). Installed source 3da5576a9971191eef44d027f2cc1727203eb9f6, renderer 2162762987, package SHA256 0899f068b59cf46783e0a0fe96b6636aa82abe37de94f65114213df1568ec607. No firmware was changed for this comparison.

The ON setting was saved through MENU_SET after verifying the descriptor and a fresh 13-object backup. The user physically restarted the FM-1. Fresh serial status confirmed cpu2_set=1, cpu2=1, playing=0 before testing. Same saved 84 BPM scene: ROAR / STAMPEDE, SAURUS / BRACHIO CALL, FM6 / MAMMOTH, VOICE / DEEP CHANT.

## Results

Compared with the earlier core-OFF baseline on this exact firmware and scene. These are live windows, not musically phase-aligned or repeated statistical benchmarks.

| Measure | Core OFF | Core ON |
| --- | ---: | ---: |
| Instrument rendering mean, us/32 frames, loaded profile | 342.40 | 311.16 |
| Instrument rendering mean, us/32 frames, USB profile | 343.13 | 311.15 |
| USB packet underruns during 30-second capture | 21 | 6 |
| Late audio blocks in surrounding ~34.6-second USB interval | 8 | 2 |
| Voices shed in same interval | 24 | 24 |
| Late blocks in unprofiled ~6.2-second interval | 2 | 0 |
| Voices shed in unprofiled interval | 4 | 5 |

Instrument rendering on the main processor improved about 9% in both profile windows. This is a stage-specific gain, not a 9% whole-device speedup or doubled capacity. Effects/master remain around 96/75 us per 32 frames. Per-track profiler rows are intentionally disabled during worker operation; their zeros do not mean those engines consume no CPU.

The ON run processed 85,189 worker blocks with zero worker faults and zero fallbacks, maximum wait 13 us, reported worker stack use 784 bytes. The loaded scene's FM6 track is eligible for worker synthesis; ROAR, SAURUS and VOICE remain excluded. Shared voice limits and overload policy are unchanged.

The ON run still reached render/wall maxima 2761/2982 us against a 2902 us deadline. Ring raw fill reached zero, and maximum publication gap was 3756 us. USB underruns and deadline overruns therefore persist. Packet underruns are not a count of distinct audible clicks. OFF cumulative maxima cover a longer device lifetime and are not valid matched-window peak comparisons. LCD timeouts stayed zero.

## Saved state and final device state

Post-test backup has 13 objects. All 12 non-settings objects match the pre-setting backup exactly by size, CRC and encoded data, including runtime song, all project slots, presets and samples. Settings differ at exactly one decoded byte: offset 556 changed 0x01 to 0x81. The only bit change is 0x80, matching PREF_CPU2_ON (0x80 shifted by 16 into ui_prefs2). No music data was changed.

Playback stopped; profiling OFF verified by a fresh profile query. Second-core preference and active worker remain ON. No firmware was flashed and no project was saved or overwritten.

## Interpretation and next work

Keep the opt-in worker available: this workload shows useful headroom and no worker errors in the measured interval. This is not a general stability certification. It does not remove the expensive excluded engines, stop overload shedding, or establish enough room for independent SCAPE.

Next useful work is a controlled USB queue-target experiment to test the remaining publication-gap/buffer-margin mismatch, with explicit latency measurements and unchanged audio equations. It must be evaluated separately from I2S deadline failures: buffering alone cannot repair over-budget synthesis. Longer worker-on playback and recording/MIDI/UI interaction tests also remain release gates. Do not merge the optimization stack or declare USB fixed on this short comparison alone.

Raw private evidence under work/paleotone-engine-hotpath/hardware/dualcore-compare-1793086667843: setting-result.json, before-setting-backup.json, hardware-profile-results.json, comparison.json, profile-usb-playing.wav, post-profiling-backup.json and saved-state-comparison.json. OFF evidence: hardware/cache-baseline-1793068624700. Do not commit raw backups.

Matching companion: https://fantasmoslam.github.io/Paleotone/webapp/screen-engine-hotpath/.
