# PALEO speed test (Step 0)

Times every PALEO instrument, the hall reverb, the echo, sample playback and the busiest moment of STAMPEDE on
a tablet, so the engine can be planned around real numbers. It is a command-line program run through adb; it
changes nothing on the tablet.

- `dsp/`: the Faust designs (the prehistoric kit, gut fiddle, throat choir, hall and echo).
- `gen/`: C++ generated from them by `gen.sh` (Faust 2.70), committed so builds need no Faust.
- `bench.cpp`: the test. `faust_min.h`: the declarations the generated code expects.
- `build.sh`: builds `build/paleo-bench/<abi>/paleo-bench` with the NDK (or `build.sh host` for a computer).

Run on the Tab S6 Lite (Windows PowerShell, from the folder holding the arm64-v8a build):

```
adb push .\paleo-bench /data/local/tmp/paleo-bench
adb shell chmod 755 /data/local/tmp/paleo-bench
adb shell /data/local/tmp/paleo-bench
```

It takes about two minutes and prints its results. The SM-T510 uses the armeabi-v7a build.
