#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
python3 tools/build.py --gen-only
mkdir -p build/android-tests
cc -O2 -fwrapv -ffp-contract=off -std=gnu11 -w -Ibuild/gen -Ifirmware/src \
    android/native/engine.c android/tests/engine_test.c -lm -o build/android-tests/engine_test
PT_SAVE_PATH=build/android-tests/session.bin build/android-tests/engine_test
PT_LOAD_PATH=build/android-tests/session.bin build/android-tests/engine_test
cc -O2 -std=c11 android/tests/output_test.c -lm -o build/android-tests/output_test
build/android-tests/output_test
javac -d build/android-tests android/java/org/paleotone/synth/AudioRecovery.java android/tests/AudioRecoveryTest.java
java -ea -cp build/android-tests org.paleotone.synth.AudioRecoveryTest
javac -d build/android-tests android/java/org/paleotone/synth/KioskSession.java android/tests/KioskSessionTest.java
java -ea -cp build/android-tests org.paleotone.synth.KioskSessionTest
cc -O2 -std=c11 android/tests/sample_test.c -lm -o build/android-tests/sample_test
build/android-tests/sample_test
javac -d build/android-tests android/java/org/paleotone/synth/WavSample.java android/tests/WavSampleTest.java
python3 -c 'from android.build import starter_samples; from pathlib import Path; starter_samples(Path("build/android-tests/starter"))'
java -ea -cp build/android-tests org.paleotone.synth.WavSampleTest "build/android-tests/starter/Flute C4.wav" "build/android-tests/starter/Jungle C3.wav"
cc -O2 -fwrapv -ffp-contract=off -std=gnu11 -w -Ibuild/gen -Ifirmware/src \
    android/tests/scape_test.c -lm -o build/android-tests/scape_test
build/android-tests/scape_test
