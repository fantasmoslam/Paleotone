#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build the two real modules and package them in an isolated Android prototype."""
import argparse
import base64
import hashlib
import json
import os
import shutil
import subprocess
import zipfile
from pathlib import Path
from apply_android_ui import apply as apply_android_ui

ROOT = Path(__file__).resolve().parents[1]
REMOTE_COMMIT = "3e3ea79b729da53c11196513e0ebb9a56ef42ab5"

def run(*args, cwd=ROOT):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), cwd=cwd, check=True)

def replace_once(path, old, new):
    text = path.read_text()
    if old not in text and new in text:
        return
    if text.count(old) != 1:
        raise SystemExit(f"Expected upstream build setting changed: {path}")
    path.write_text(text.replace(old, new, 1))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--remote", type=Path, required=True, help="Dedicated checkout from prepare_remote.py")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    remote = args.remote.resolve()
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=remote, text=True).strip()
    if revision != REMOTE_COMMIT:
        raise SystemExit("Unexpected upstream checkout; run prepare_remote.py in a dedicated directory")
    sdk_value = os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT")
    if not sdk_value:
        raise SystemExit("Set ANDROID_HOME to the SDK containing the documented packages")
    sdk = Path(sdk_value).resolve()
    cmake = sdk / "cmake/3.31.6/bin/cmake"
    ninja = sdk / "cmake/3.31.6/bin/ninja"
    ndk = sdk / "ndk/29.0.14206865"
    build = ROOT / "build/instrument-host/android"
    run(cmake, "-S", ROOT / "host", "-B", build, "-G", "Ninja",
        f"-DCMAKE_MAKE_PROGRAM={ninja}", f"-DCMAKE_TOOLCHAIN_FILE={ndk}/build/cmake/android.toolchain.cmake",
        "-DANDROID_ABI=arm64-v8a", "-DANDROID_PLATFORM=android-29", "-DCMAKE_BUILD_TYPE=Release",
        f"-DFM1VST_DIR={remote}/vendor/FM1VST", f"-DREMOTE_ANDROID_DIR={remote}")
    run(cmake, "--build", build, "--target", "paleotone_core", "felucca_core", "-j", args.jobs)
    libs = remote / "build/cores/android/jniLibs/arm64-v8a"
    libs.mkdir(parents=True, exist_ok=True)
    wanted = {"libfm1core_paleotone.so", "libfm1core_felucca.so"}
    if {p.name for p in libs.glob("libfm1core_*.so")} - wanted:
        raise SystemExit("Comparison directory has other instrument builds; use a fresh dedicated checkout")
    for name in wanted:
        shutil.copy2(build / "cores" / name, libs / name)

    # Distinct install identity: never replace the accepted Paleotone or RemoteFM1 app.
    replace_once(remote / "app/build.gradle.kts", 'applicationId = "com.quixotic7.remotefm1"',
                 'applicationId = "org.paleotone.host.preview"')
    replace_once(remote / "app/build.gradle.kts", 'versionCode = 3', 'versionCode = 5')
    replace_once(remote / "app/build.gradle.kts", 'versionName = "0.2.1"', 'versionName = "0.1.2"')
    replace_once(remote / "app/src/main/res/values/strings.xml", '>RemoteFM1</string>',
                 '>Instrument Host</string>')
    # Public development key, never a release identity. Stable across CI runners
    # so subsequent preview builds can update without losing test state. The
    # first prototype used an ephemeral runner key; the new ID preserves it.
    key = remote / "build/prototype-debug.p12"
    key.parent.mkdir(parents=True, exist_ok=True)
    key.write_bytes(base64.b64decode((ROOT / "host/prototype-debug-key.b64").read_text()))
    replace_once(remote / "app/build.gradle.kts", '    signingConfigs {', '''    signingConfigs {
        getByName("debug") {
            storeFile = rootProject.file("build/prototype-debug.p12")
            storePassword = "android"
            keyAlias = "androiddebugkey"
            keyPassword = "android"
            storeType = "PKCS12"
        }''')
    apply_android_ui(remote)
    notice_root = remote / "app/src/main/assets/host-licenses"
    for instrument, source_root in [("paleotone", ROOT), ("felucca", remote / "vendor/FM1VST/cores/Felucca")]:
        notices = notice_root / instrument
        notices.mkdir(parents=True, exist_ok=True)
        for source in [source_root / "LICENSE", source_root / "LICENSING.md", *sorted((source_root / "LICENSES").glob("*.txt"))]:
            shutil.copy2(source, notices / source.name)
    manifest = {
        "prototype": True, "application_id": "org.paleotone.host.preview", "version": "0.1.2",
        "paleotone_source": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "remote_source": REMOTE_COMMIT,
        "fm1vst_source": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=remote / "vendor/FM1VST", text=True).strip(),
        "felucca_source": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=remote / "vendor/FM1VST/cores/Felucca", text=True).strip(),
        "instruments": sorted(wanted), "device_qualified": False,
    }
    (notice_root / "BUILD.json").write_text(json.dumps(manifest, indent=2) + "\n")
    run("bash", "gradlew", ":app:testDebugUnitTest", ":app:assembleDebug", "-PrfmSkipCores=true",
        "--no-daemon", f"--max-workers={args.jobs}", cwd=remote)
    original = remote / "app/build/outputs/apk/debug/app-debug.apk"
    with zipfile.ZipFile(original) as apk:
        required = {"lib/arm64-v8a/" + name for name in wanted} | {"lib/arm64-v8a/librfmcore.so"}
        if not required.issubset(apk.namelist()):
            raise SystemExit("APK is missing an actual instrument or native host library")
    output = ROOT / "build/instrument-host/release"
    output.mkdir(parents=True, exist_ok=True)
    apk_path = output / "Instrument-Host-0.1.2.apk"
    shutil.copy2(original, apk_path)
    run(sdk / "build-tools/36.0.0/apksigner", "verify", "--verbose", apk_path)
    manifest["apk_sha256"] = hashlib.sha256(apk_path.read_bytes()).hexdigest()
    (output / "BUILD.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(apk_path)

if __name__ == "__main__":
    main()
