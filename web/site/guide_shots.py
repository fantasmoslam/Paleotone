#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The user guide's screen images: tests/guide_shots.c (the firmware's UI on the host test harness, driven by the
panel) renders each screen; here they become 2x PNGs in web/site/guide/ (committed: the site's workflow needs no
compiler for them). Run after one ./build.sh or tools/build.py's generate() (build/gen):

  python3 web/site/guide_shots.py
"""
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "web" / "site" / "guide"
TMP = ROOT / "build" / "guide"


def main():
    TMP.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    exe = ROOT / "build" / "host" / "guide_shots"
    exe.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["cc", "-O1", "-w", "-Ibuild/gen", "-Ifirmware/src", "-Itests", "-o", str(exe),
                    "tests/guide_shots.c", "-lm"], cwd=ROOT, check=True)
    subprocess.run([str(exe), str(TMP)], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
    for f in sorted(TMP.glob("*.ppm")):
        im = Image.open(f).convert("RGB").resize((480, 480), Image.NEAREST)
        im.save(OUT / (f.stem + ".png"), optimize=True)
    print(f"guide: {len(list(TMP.glob('*.ppm')))} screens -> {OUT}")


if __name__ == "__main__":
    sys.exit(main())
