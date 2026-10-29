# Paleotone Android — native alpha 0.1.1

Standalone synthesis for Dylan's Samsung SM-T510. Initial measured device configuration:
Android 15 / unofficial LineageOS 22.2-20260910, `armeabi-v7a,armeabi`, 48 kHz
stereo, FAST hardware output at 192 frames. MIDI and USB host are advertised;
the low-latency/pro-audio feature flags are absent. A hardware buffer spans 4 ms;
this is **not** a physical latency measurement or an accepted synth performance result.

## Source baseline and scope

Based on integration PR #26, `0390a03f732c5eeec9f62cb2b139df8738396766`.
Firmware, browser and release routing source are unchanged. This is a native
Android platform adapter around `web/emu/felucca_web.c`, which already compiles
the real firmware engines, UI, SCAPE, effects, sequencer and flash-storage hooks
outside the FM-1. It does not use WebView, JavaScript or a network server.

Android's C AAudio API is used directly in this first build (API 28+), with
low-latency/exclusive requests, a shared fallback, and a query of actual stream
settings. Oboe remains a future option if measured driver behavior warrants its
quirk handling. No claim is made that exclusive or MMAP operation is obtained.
The stream selects its native rate; a continuous linear converter preserves the
44.1 kHz firmware engine. Higher-quality conversion is a follow-up before treating
this as a finished production instrument.

## Install on the tablet

Download the APK to the PC. In the same `platform-tools` PowerShell window:

```powershell
.\adb.exe -s 192.168.0.46:33862 install -r "$env:USERPROFILE\Downloads\Paleotone-Android-0.1.1-alpha.apk"
.\adb.exe -s 192.168.0.46:33862 shell am start -n org.paleotone.synth/.MainActivity
```

The wireless debugging address/port can change after reboot or toggling debugging.
Use the current main Wireless debugging page's connection address if needed.
Start with a low media volume and the tablet speaker or wired headphones.
Bluetooth is not a suitable route for assessing playable latency.

## Controls and storage

- Landscape panel: real 240x240 firmware display, 14 panel buttons, seven encoders,
  master slider and a multitouch F3–G5 keyboard. Drag encoders vertically or tap −/+.
- Firmware button holds retain their alternate behavior. SELECT, ENGINE and PRESET
  correspond to the physical SELECT, ALGORITHM and PRESETS encoders.
- **SAVE SESSION** briefly stops/closes the stream, captures the live portable
  project plus the 1 MiB simulated flash, writes an Android `AtomicFile`, and restarts.
- Going to another app stops audio and saves. A fresh process restores the project
  with the transport stopped. Unexpected power loss, a crash or force-stop can lose
  unsaved edits. This alpha does not promise periodic disk autosave.
- Firmware SAVE retains the firmware project menu and writes simulated flash in
  RAM; SAVE SESSION or leaving the app makes that flash durable on Android.
- Sessions are in app-private storage (`session.ptandroid`). Uninstalling removes them.
- MIDI selects Android MIDI output ports from attached controllers. Channel-message
  parsing handles fragments, running status and interspersed real-time bytes.
  External clock, SysEx, BLE discovery and file/sample import are not implemented.
- Audio reconnects automatically after stream errors while foreground and playing,
  with a short debounce and at most five retries. Stop, backgrounding and lost focus
  cancel retries. If recovery is exhausted, START provides an explicit retry.
- AUDIO offers Original (1x), Normal (2x / +6 dB, default), and Boost (4x / +12 dB).
  Normal restores the FM-1 codec reserve; a stereo-linked limiter bounds digital
  peaks at 0.98. Gain changes ramp and the limiter releases over 50 ms. Android media
  volume remains independent. Output level is retained in app preferences.
- AUDIO also offers 2/3/4/6 buffer bursts; changing it pauses audio and preserves
  whether playback was stopped. PANIC releases local/controller notes; an active sequencer
  can subsequently trigger more notes. PLAY controls transport.

## Thread and timing boundaries

The audio callback is the sole live engine owner. JNI uses atomic key/button states,
atomic encoder deltas and a bounded MIDI queue; MIDI producers serialize outside
the audio callback. Queue overflow requests a panic. There is no allocation, disk
I/O, Java callback or blocking mutex acquisition on the audio thread. Display
publication uses trylock and skips a snapshot when the UI owns the buffer.

**The portable firmware's control loop and UI drawing still execute on the engine
owner inside render calls.** This first parity port has not separated firmware
UI rendering from DSP. The Java panel refreshes at about 30 Hz. Real-time deadline
measurements on the tablet must determine whether a snapshot renderer or other
refactoring is needed; the firmware's simulated CPU diagnostics are not physical
Android timing. The app provides real callback-duration and AAudio underrun counters.

No all-core pinning, root, governor changes, thermal-limit overrides, package
removal, device-owner provisioning or kiosk lockdown is performed by this app.
Kiosk configuration follows functional/audio acceptance and a working exit path.

## Build

Linux x86-64, JDK 17, Python 3 and Android command-line tools:

```sh
sdkmanager 'platforms;android-35' 'build-tools;35.0.0' 'ndk;27.2.12479018'
export ANDROID_HOME=/path/to/android-sdk
python3 -m pip install pillow==12.3.0 fonttools==4.61.1 numpy==2.3.5
python3 android/build.py
sh android/test.sh
```

The APK is `build/android/Paleotone-Android-0.1.1-alpha.apk`.
This dependency-light build calls javac, D8, aapt2, the NDK compiler, zipalign and
apksigner directly. It requires no Gradle/Maven artifacts or proprietary FM-1 SDK.
Generated tables/fonts/samples use the repository's existing generator.

The development signing key is generated in `build/android-signing/debug.keystore`
and must be retained for updates to the installed APK. It is a **development** key,
with the conventional password `android` and alias `androiddebugkey`, not a production
release key. A different build environment generates a different key: retain it
with `ANDROID_KEYSTORE` or uninstall the old app before installing a differently
signed build (uninstalling deletes its private session).

## Validation and remaining device checks

Host tests exercise variable-sized output callbacks, 44.1→48 kHz conversion, finite
audio, note on/off, MIDI channel messages, sequencer controls, screen conversion,
save sizing and restoration of the exact portable project in a separate process.
Output tests cover gain, finite/stereo-linked limiting and recovery; Java policy tests
cover route debounce, bounded retry backoff and cancellation.
APK validation checks signing, launch activity, API levels, 32-bit ARM ABI and
native dependencies. The original 0.1.0 APK passed user-reported multitouch playing, display/sequencer
tracking and save/restore. The 0.1.1 volume and automatic recovery changes still
need physical verification. See DEVICE-TESTS.md for exact acceptance boundaries.

Device acceptance, in order:

1. Launch, play a note, release it; check multitouch and all panel controls.
2. Confirm sound and readable pixels; collect the displayed actual stream settings.
3. Run a sequencer pattern while moving macros; watch/listen for underruns.
4. Try a larger buffer if needed; compare sustained operation and heat.
5. SAVE SESSION, exit, reopen; verify project/preset retention and stopped transport.
6. Test audio-focus interruption, headphones/USB routing and controller detach.
7. Measure physical latency with suitable loopback equipment if required.
8. Add kiosk/Home integration and provision device owner only after these pass.

## Licensing

GPL-3.0-only, retaining Felucca/GHOULBOX authorship and bundled component licenses.
The APK includes the repository license notices and sample attribution in assets.
The corresponding source is this repository at the exact Android implementation
commit together with its documented baseline; no binary-only engine is used.
