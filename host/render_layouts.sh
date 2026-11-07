#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Emulator-only; no user hardware is accessed. Capture the actual Compose UI.
set -euo pipefail
REMOTE=$1
OUT=$2
mkdir -p "$OUT"
cd "$REMOTE"
bash gradlew :app:assembleDebug :app:assembleDebugAndroidTest -PrfmSkipCores=true -PhostLayoutOnly=true --no-daemon --max-workers=2
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell settings put secure immersive_mode_confirmations confirmed
for profile in '2560 1600 320 1.0' '1600 2560 320 1.0' '1920 1200 320 1.0' '2560 1600 320 1.3'; do
    read -r width height density font <<< "$profile"
    adb shell wm size "${width}x${height}"
    adb shell wm density "$density"
    adb shell settings put system font_scale "$font"
    result=$(adb shell am instrument -w -r -e class org.paleotone.host.ui.HostLayoutTest org.paleotone.host.preview.test/androidx.test.runner.AndroidJUnitRunner)
    printf '%s\n' "$result"
    adb pull /sdcard/Android/data/org.paleotone.host.preview/files/layout "$OUT/" || true
    if [[ "$result" != *"OK (2 tests)"* ]]; then exit 1; fi
done
