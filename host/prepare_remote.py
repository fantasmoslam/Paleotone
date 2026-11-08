#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare a pinned, dedicated upstream checkout for the host prototype."""
import argparse
import subprocess
from pathlib import Path

REMOTE_URL = "https://github.com/Quixotic7/RemoteFM1Android.git"
REMOTE_COMMIT = "3e3ea79b729da53c11196513e0ebb9a56ef42ab5"

def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    out = args.out.resolve()
    if not out.exists():
        out.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "clone", "--no-checkout", "--filter=blob:none", REMOTE_URL, str(out)], check=True)
        subprocess.run(["git", "-C", str(out), "checkout", "--detach", REMOTE_COMMIT], check=True)
    if git(out, "rev-parse", "HEAD") != REMOTE_COMMIT:
        raise SystemExit("Existing comparison checkout has another revision; use a fresh --out directory.")
    subprocess.run(["git", "-C", str(out), "submodule", "update", "--init", "--depth", "1",
                    "vendor/RemoteFM1", "vendor/FM1VST"], check=True)
    subprocess.run(["git", "-C", str(out / "vendor/FM1VST"), "submodule", "update", "--init",
                    "--depth", "1", "--recursive", "cores/Felucca"], check=True)
    print(out)

if __name__ == "__main__":
    main()
