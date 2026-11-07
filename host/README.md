# Android instrument host prototype

This implementation adds the real Paleotone firmware as a native `fm1core` instrument alongside Felucca in the pinned RemoteFM1 Android host. It reuses FM1VST's existing ABI and Felucca-family adapter rather than introducing another protocol or replacing the synthesis engines.

The prototype installs as **Instrument Host Prototype**, package `org.paleotone.host.prototype`. It is separate from existing Paleotone and RemoteFM1 installations. It retains the upstream host interface and controller transport while the project-specific tablet and controller experience is developed.

## Build

The GitHub workflow `Android instrument host prototype` builds the two actual instrument modules, runs the desktop integration test and Android JVM tests, packages both modules in an APK and verifies its signature. SDK dependencies are pinned in that workflow. The initial APK is arm64/API 29+, matching this selected upstream host; this is not a claim about the tablet's installed ABI or physical compatibility.

For local desktop verification, install CMake 3.22+, a C/C++ compiler, Python, Pillow/fontTools/numpy, then:

```sh
python3 host/prepare_remote.py --out build/instrument-host/RemoteFM1Android
cmake -S host -B build/instrument-host/desktop \
  -DFM1VST_DIR="$PWD/build/instrument-host/RemoteFM1Android/vendor/FM1VST" \
  -DREMOTE_ANDROID_DIR="$PWD/build/instrument-host/RemoteFM1Android"
cmake --build build/instrument-host/desktop --target paleotone_core felucca_core host_core_integration -j 2
ctest --test-dir build/instrument-host/desktop --output-on-failure
```

With the SDK packages and JDK 21 from the workflow installed, set `ANDROID_HOME` and run:

```sh
python3 host/build_android.py --remote build/instrument-host/RemoteFM1Android
```

Use a dedicated comparison checkout: packaging changes its application ID/label and adds Paleotone notices. Native core building is skipped only in the upstream Gradle task because this project's preceding CMake step already built and supplied both actual modules. The final APK is explicitly checked for both modules and its native host.

## Scope and validation

The desktop test loads each real module, rejects a wrong ABI, boots, plays a key, checks for audio and screen output, captures its flash image, unloads, and repeats alternating instruments with separate saved images. It does not establish physical latency, hardware feedback or Android lifecycle correctness.

The generic adapter currently provides the original firmware panel and flash persistence. Paleotone's separate Android sample-import layer, redesigned Scape controls and kiosk shell are not automatically included in this firmware-core adapter. Richer tablet controls and complete FM-1/SMK capability profiles remain subsequent work. Hardware recovery is not emulated inside a sound module. No controller flashing is performed by these build scripts.

Upstream sources remain pinned; their notices and source pointers apply. Paleotone notices are embedded in the instrument descriptor and packaged under `assets/host-licenses`. This is a prototype implementation, not a qualified release or a compatibility promise for every instrument or SMK variant.
