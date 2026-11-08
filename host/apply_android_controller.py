#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the source-dependent MIDI ownership fix to the pinned Android runtime."""
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PIN = "3e3ea79b729da53c11196513e0ebb9a56ef42ab5"
PACKAGE = "app/src/main/java/com/quixotic7/remotefm1/"

def apply(remote: Path):
    # Always restore patch targets, making repeated builds deterministic.
    for name in ("AppModel.kt", "RfmMidi.kt", "midi/ControllerInput.kt"):
        data = subprocess.check_output(["git", "show", f"{PIN}:{PACKAGE}{name}"], cwd=remote)
        (remote / PACKAGE / name).write_bytes(data)
    subprocess.run(["git", "apply", "--check", str(ROOT / "android-controller/routing.patch")], cwd=remote, check=True)
    subprocess.run(["git", "apply", str(ROOT / "android-controller/routing.patch")], cwd=remote, check=True)
    for kind in ("main", "test"):
        dest = remote / f"app/src/{kind}/java/com/quixotic7/remotefm1"
        dest.mkdir(parents=True, exist_ok=True)
        for path in (ROOT / "android-controller" / kind).glob("*.kt"):
            shutil.copy2(path, dest / path.name)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--remote", type=Path, required=True)
    apply(parser.parse_args().remote.resolve())
