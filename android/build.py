#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build the standalone APK with SDK/NDK tools; no Gradle or Maven dependencies.

Requires JDK 17, Android platform 35, build-tools 35.0.0, NDK 27.2.12479018.
ANDROID_HOME selects the SDK. ANDROID_KEYSTORE may supply a persistent signing
key (alias androiddebugkey, standard development password android).
The default development key is generated once in build/android-signing/.
"""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SDK = Path(os.environ.get("ANDROID_HOME", os.environ.get("ANDROID_SDK_ROOT", "")))
OUT = ROOT / "build/android"
TOOLS = SDK / "build-tools/35.0.0"
PLATFORM = SDK / "platforms/android-35/android.jar"
NDK = SDK / "ndk/27.2.12479018/toolchains/llvm/prebuilt/linux-x86_64/bin"

def run(*args):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True)

def main():
    if not PLATFORM.is_file() or not (NDK/"armv7a-linux-androideabi28-clang").is_file():
        raise SystemExit("Install the documented Android SDK/NDK packages and set ANDROID_HOME.")
    if OUT.exists(): shutil.rmtree(OUT)
    for name in ("lib/armeabi-v7a", "classes", "dex", "assets/licenses"):
        (OUT/name).mkdir(parents=True, exist_ok=True)
    run("python3", "tools/build.py", "--gen-only")
    run(NDK/"armv7a-linux-androideabi28-clang", "-std=gnu11", "-O2",
        "-ffp-contract=off", "-fwrapv", "-fPIC", "-shared", "-fvisibility=hidden",
        "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections", "-Wl,-z,max-page-size=16384",
        "-Wno-incompatible-pointer-types-discards-qualifiers", "-Wno-unused-function",
        "-Ibuild/gen", "-Ifirmware/src", "android/native/engine.c", "android/native/android_audio.c",
        "-laaudio", "-llog", "-lm", "-o", OUT/"lib/armeabi-v7a/libpaleotone.so")
    run(NDK/"llvm-strip", "--strip-unneeded", OUT/"lib/armeabi-v7a/libpaleotone.so")
    java = sorted((ROOT/"android/java").rglob("*.java"))
    run("javac", "-encoding", "UTF-8", "--release", "8", "-classpath", PLATFORM,
        "-d", OUT/"classes", *java)
    run(TOOLS/"d8", "--min-api", "28", "--lib", PLATFORM, "--output", OUT/"dex",
        *sorted((OUT/"classes").rglob("*.class")))
    shutil.copy(ROOT/"LICENSE", OUT/"assets/licenses/GPL-3.0.txt")
    for file in (ROOT/"LICENSES").iterdir():
        if file.is_file():shutil.copy(file,OUT/"assets/licenses"/file.name)
    shutil.copy(ROOT/"LICENSING.md",OUT/"assets/licenses/LICENSING.md")
    shutil.copy(ROOT/"assets/samples-cc0/ATTRIBUTION.txt",OUT/"assets/licenses/SAMPLES.txt")
    (OUT/"assets/BUILD.txt").write_text(
        "Paleotone Android 0.1.2-alpha\nGPL-3.0-only\n"+
        subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,text=True).strip()+
        "\nBase: 0390a03f732c5eeec9f62cb2b139df8738396766\n"+
        "Local modifications are included in the corresponding source archive.\n")
    run(TOOLS/"aapt2", "compile", "--dir", ROOT/"android/res", "-o", OUT/"resources.zip")
    run(TOOLS/"aapt2", "link", "-o", OUT/"unsigned.apk", "-I", PLATFORM,
        "--manifest", ROOT/"android/AndroidManifest.xml", "-A", OUT/"assets", OUT/"resources.zip")
    with zipfile.ZipFile(OUT/"unsigned.apk","a",zipfile.ZIP_DEFLATED) as archive:
        archive.write(OUT/"dex/classes.dex","classes.dex")
        archive.write(OUT/"lib/armeabi-v7a/libpaleotone.so","lib/armeabi-v7a/libpaleotone.so")
    run(TOOLS/"zipalign", "-f", "-P", "16", "4", OUT/"unsigned.apk", OUT/"aligned.apk")
    key=Path(os.environ.get("ANDROID_KEYSTORE",str(ROOT/"build/android-signing/debug.keystore")))
    if not key.exists():
        key.parent.mkdir(parents=True,exist_ok=True)
        run("keytool","-genkeypair","-keystore",key,"-storepass","android","-keypass","android",
            "-alias","androiddebugkey","-keyalg","RSA","-keysize","2048","-validity","10000",
            "-dname","CN=Paleotone Development,O=Paleotone,C=US")
    apk=OUT/"Paleotone-Android-0.1.2-alpha.apk"
    run(TOOLS/"apksigner","sign","--ks",key,"--ks-pass","pass:android","--key-pass","pass:android",
        "--out",apk,OUT/"aligned.apk")
    run(TOOLS/"apksigner","verify","--verbose",apk)
    (OUT/"SHA256SUMS").write_text(hashlib.sha256(apk.read_bytes()).hexdigest()+"  "+apk.name+"\n")
    print(apk)

if __name__=="__main__":main()
