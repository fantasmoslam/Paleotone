#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Install project-owned UI sources over the pinned runtime checkout."""
import argparse
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent

def change(path, old, new):
    text = path.read_text()
    if new in text and old not in text:
        return
    if text.count(old) != 1:
        raise SystemExit(f"Unexpected UI entry point: {path}")
    path.write_text(text.replace(old, new, 1))

def apply(remote):
    package = Path('org/paleotone/host/ui')
    sources = remote / 'app/src/main/java' / package
    tests = remote / 'app/src/test/java' / package
    sources.mkdir(parents=True, exist_ok=True)
    tests.mkdir(parents=True, exist_ok=True)
    for name in ['InstrumentHostScreen.kt', 'HostKeyboard.kt', 'HostSession.kt']:
        shutil.copy2(ROOT / 'android' / name, sources / name)
    shutil.copy2(ROOT / 'android/HostKeyboardTest.kt', tests / 'HostKeyboardTest.kt')
    for kind, name in [('debug', 'HostLayoutActivity.kt'), ('androidTest', 'HostLayoutTest.kt')]:
        target = remote / 'app/src' / kind / 'java' / package
        target.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / 'android' / kind / name, target / name)
    debug_manifest = remote / 'app/src/debug/AndroidManifest.xml'
    text = debug_manifest.read_text()
    marker = '<activity android:name="org.paleotone.host.ui.HostLayoutActivity" android:exported="false" />'
    if marker not in text:
        debug_manifest.write_text(text.replace('<application>', '<application>' + marker, 1))
    change(remote / 'app/build.gradle.kts', 'ndk { abiFilters += "arm64-v8a" }',
           'ndk { abiFilters += (if (project.hasProperty("hostLayoutOnly")) "x86_64" else "arm64-v8a") }')
    draw = remote / 'app/src/main/res/drawable'
    draw.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / 'android/host_icon.xml', draw / 'host_icon.xml')
    change(remote / 'app/src/main/java/com/quixotic7/remotefm1/MainActivity.kt',
           'setContent { MainScreen(appUi) { askNotifications() } }',
           'setContent { org.paleotone.host.ui.InstrumentHostScreen(appUi) { askNotifications() } }')
    manifest = remote / 'app/src/main/AndroidManifest.xml'
    change(manifest, 'android:icon="@mipmap/ic_launcher"', 'android:icon="@drawable/host_icon"')
    change(manifest, 'android:roundIcon="@mipmap/ic_launcher_round"', 'android:roundIcon="@drawable/host_icon"')
    # Rebrand runtime-generated media text, retaining source copyright/comments.
    for name in ['PlaybackService.kt', 'AppModel.kt']:
        p = remote / 'app/src/main/java/com/quixotic7/remotefm1' / name
        text = p.read_text()
        text = text.replace('"RemoteFM1', '"Instrument Host')
        p.write_text(text)

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--remote', type=Path, required=True)
    apply(p.parse_args().remote.resolve())
