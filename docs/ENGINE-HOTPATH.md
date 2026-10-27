# Exact-arithmetic ROAR / SAURUS helper candidate

Based on the last hardware-verified effects optimization, source
ba586018234cd8be5fbce09a3c3c7306c4c1389f, renderer 696079837.

SAURUS now explicitly uses the existing always-inline clipping implementation in
its per-throat sample loop. ROAR's two per-sample body filters now force inline
their existing helper. The clipping table, interpolation, filter equations,
rounding, state updates, random draws and their order are unchanged. Other
engines retain their existing clipping wrapper. No clocks, buffers, reserve,
voice limits, overload thresholds or second-core exclusions change.

The target compiler previously outlined these helpers. Target assembly and the
unchanged instruction budgets must establish that the intended calls disappear
without excessive code growth. Existing sound hashes, health checks, ROAR
behavior tests, concurrent mixer tests and memory-race checks remain gates.
Static loop counts include newly inlined arithmetic but do not expand the old
callees; they do not independently establish runtime improvement.

## Hardware gate

The current cloud workspace cannot access the Windows computer's FM-1, COM6 or
WinMM MIDI/audio ports. No device installation or hardware timing measurement is
part of this checkpoint. Do not describe a software pass as a measured speedup.

Once the home computer connection is restored:

1. Verify the connected device identity and actual current renderer/settings.
2. Retain the last verified package (SHA256
   449b95d46eaa27f38ea11eabc539c4e33c4044436bfa419b83418dbb38884541)
   and capture a fresh consistent backup before any installation.
3. Confirm all exact-source build, target, sound/concurrency and companion gates.
4. Stage an identity-pinned installer for the exact candidate; do not reuse an
   older installer's renderer or package guards blindly.
5. Compare the same loaded 84 BPM scene, initially with the second core OFF.
   Assert actual playback in each loaded phase. Compare T1/ROAR, T2/SAURUS,
   aggregate parts, effects/master, render/wall deadline counters and voice
   shedding. Individual track rows overlap aggregate parts.
6. Capture USB audio for at least 30 seconds. The last effects build's clean
   eight-second capture was followed by 19 underruns in a 30-second capture;
   one clean short capture does not establish a fix. Packet underruns are not
   counts of separate audible clicks.
7. Compare all 13 backup objects in size, CRC and data; leave playback stopped
   and profiling OFF. Record the second-core state explicitly.

This candidate does not implement independent SCAPE. It first seeks additional
rendering headroom while retaining the current music and saved projects.
