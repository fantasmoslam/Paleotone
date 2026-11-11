# NightClaw extended playback check

Source: 3cf8a1a (installed firmware); host engine and UI from the same working tree. Run on 2026-11-11 with:

```sh
mkdir -p build/nightclaw-endurance
build/host/amb_host tests/listen/nightclaw-endurance.amb build/nightclaw-endurance
```

The repeatable [script](../tests/listen/nightclaw-endurance.amb) runs each of the twenty production scenes for three minutes, holding three Lead keys for the first minute and releasing them for the next two. It then reboots the simulated project and runs Cello, Cello Low, Sub Pad and Fat Pad on Lead for three minutes each, holding the same three keys for two minutes and releasing them for one. Scene and voice IDs are asserted; each segment asserts an output peak below -0.1 dBFS and above -50 dBFS.

Result: exit 0, 4,329,488 ms simulated (72 minutes plus control changes), all 24 segments passed. Scene output peaks ranged from -26.6 to -4.6 dBFS at the simulator default master setting; appended instrument segments ranged from -19.8 to -17.9 dBFS.

This confirms successful extended host execution, correct selection and bounded, non-silent peak output in each segment. It does not establish continuous audibility throughout a segment, subjective sound quality, physical-device CPU deadlines, hardware USB stability, maximum-master headroom or concurrent-worker performance. Concurrent and fallback worker behavior are covered separately by the full CI audio-hash tests. The FM-1 remained running HIGH FRONTIER and answered a read-only status request during this pass. Longer physical-device performance coverage remains open.
