# Android instrument host prototype

This implementation adds the real Paleotone firmware as a native `fm1core` instrument alongside Felucca in the pinned RemoteFM1 Android host. It reuses FM1VST's existing ABI and Felucca-family adapter rather than introducing another protocol or replacing the synthesis engines.

The prototype installs as **Instrument Host**, package `org.paleotone.host.preview`. It is separate from existing Paleotone and RemoteFM1 installations. The project-owned Compose interface replaces the upstream faceplate, navigation, instrument browser, audio pages and launcher icon. Runtime/audio/persistence internals remain attributed upstream. Our controller bridge is still in development; this build defaults to touchscreen operation and does not offer an upstream firmware installer.

## Build

The GitHub workflow `Android instrument host prototype` builds the two actual instrument modules, runs the desktop integration test and Android JVM tests, packages both modules in an APK and verifies its signature. SDK dependencies are pinned in that workflow. The initial APK is arm64/API 29+, matching this selected upstream host; this is not a claim about the tablet's installed ABI or physical compatibility.

For local desktop verification, install CMake 3.22+, a C/C++ compiler, Python, Pillow/fontTools/numpy, then:

```sh
python3 host/prepare_remote.py --out build/instrument-host/RemoteFM1Android
cmake -S host -B build/instrument-host/desktop \
  -DFM1VST_DIR="$PWD/build/instrument-host/RemoteFM1Android/vendor/FM1VST" \
  -DREMOTE_ANDROID_DIR="$PWD/build/instrument-host/RemoteFM1Android"
cmake --build build/instrument-host/desktop --target paleotone_core felucca_core host_core_integration host_audio_fidelity host_preset_audition -j 2
ctest --test-dir build/instrument-host/desktop --output-on-failure
```

With the SDK packages and JDK 21 from the workflow installed, set `ANDROID_HOME` and run:

```sh
python3 host/build_android.py --remote build/instrument-host/RemoteFM1Android
```

Use a dedicated comparison checkout: packaging changes its application ID/label and adds Paleotone notices. Native core building is skipped only in the upstream Gradle task because this project's preceding CMake step already built and supplied both actual modules. The final APK is explicitly checked for both modules and its native host.

## Scope and validation

The desktop integration test loads each real module, rejects a wrong ABI, boots, plays a key, checks for audio and screen output, captures its flash image, unloads, and repeats alternating instruments with separate saved images. A second test compares the new Paleotone module against the unchanged original firmware unit, sample for sample and pixel for pixel, while operating notes, buttons and encoders. Neither test establishes physical latency, hardware feedback or Android lifecycle correctness.

The generic adapter currently provides the original firmware panel and flash persistence. Paleotone's separate Android sample-import layer, redesigned Scape controls and kiosk shell are not automatically included in this firmware-core adapter. Richer tablet controls and complete FM-1/SMK capability profiles remain subsequent work. Hardware recovery is not emulated inside a sound module. No controller flashing is performed by these build scripts.

Upstream sources remain pinned; their notices and source pointers apply. Paleotone notices are embedded in the instrument descriptor and packaged under `assets/host-licenses`. This is a prototype implementation, not a qualified release or a compatibility promise for every instrument or SMK variant.

## 0.1.1: held-key preset audition

On the SM-T860, the user verified both instruments, switching between them, and Paleotone settings surviving a switch and app restart in 0.1.0. A further check found that browsing Paleotone presets while holding a key released the old voice without starting the new sound. Fresh presses continued to play; this was not an app freeze. Sound loads now replay physically held keys on the affected track through the firmware's normal engine fade and ownership paths. Ordinary panic stays silent. Audition does not write extra notes into live recording. Felucca is unchanged. The regression test covers repeated browsing, chords, cross-engine fades, release timing, panic, recording and user presets. Physical confirmation of this fix is still needed.

The original CI runner's debug signing key was ephemeral. This preview uses a separate package identity and a stable, intentionally public development key (`prototype-debug-key.b64`, password `android`), so it can install alongside 0.1.0 and later preview updates can retain their own test state. Never use this debug identity for production. Original prototype settings remain in the original app and are not migrated automatically.

## 0.1.2: project-owned tablet interface

Authored sources live under `host/android`; `apply_android_ui.py` installs them into the pinned runtime checkout. The main activity launches `InstrumentHostScreen`, not the upstream `MainScreen`. Five explicit destinations replace the faceplate/sheet interaction: Play, Instruments, Controllers, Audio and About. The original instrument framebuffer and panel semantics remain available in a new performance layout with large increment/decrement controls, momentary panel buttons and a separately authored multitouch keyboard. Touch cancellation releases notes/buttons. Library selection releases touch ownership before switching, and instrument ID scopes the playable surface. Named snapshots save/load through the existing per-instrument state store. The sound enable/mute action remains visible throughout navigation; it is labelled mute, not sequencer stop.

The warm charcoal/amber/mint palette, typography, workbench layout, browser, connection cards and vector launcher mark are new. About credits the reused runtime and instrument sources. Legacy install prompts, skin controls, catalogue downloads and bridge setup pages are not reachable from this interface. FM-1 and SMK-37 cards explicitly state bridge work is pending. No controller firmware is changed. Hardware integration remains the next separate milestone.

This build updates 0.1.1 using the same package and public development signing identity, retaining its app data. The SM-T860 playback/state/preset-audition checks reported by the user apply to 0.1.1; this new UI still requires device confirmation. Keyboard geometry tests cover every firmware key index at multiple tablet widths, black-key priority and leaving the touch area.

## 0.1.3: fixed, scroll-free tablet layouts

The 0.1.2 interface was rejected by the user as cluttered and requiring scrolling. The replacement reduces the header to navigation and mute, gives the original display most of the play area, fixes the keyboard below it, and presents four gesture-driven dials in Sound/Edit banks or the original momentary buttons in a Panel bank. All main pages are bounded layouts. Instrument, snapshot and output collections paginate rather than extending a page. Source and runtime responsibilities are unchanged.

`HostViewState` and `HostActions` separate rendering from the engine adapter. A debug-only activity renders those same composables with a real Paleotone framebuffer exported by the desktop reference unit and a recording panel backend. The emulator suite checks every page/bank for complete bounds, text clipping, absence of horizontal/vertical scrolling, and key/encoder events. CI captures the rendered layouts at 1280×800, 800×1280 and 960×600 dp, plus larger text at 1280×800 dp. Those are layout fixtures, not hardware audio/controller qualification. The x86 layout APK stays separate from the arm64 user APK copied into release output.
