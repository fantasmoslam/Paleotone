#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 PALEOTONE contributors
"""Check two full flash backups of an SMK-37 before installing PALEOTONE (the fm1-to-smk37 kit's
docs/02-update-and-recovery.md, section 3): both exactly 1 MiB, the same bytes, and the unit's calibration record
in place ("trimdat" at 0xC1224).

  py -3.12 tools/smk_backup_check.py smk37-dump-1.bin smk37-dump-2.bin

Prints OK and the SHA-256 when the backup can be trusted; otherwise says what is wrong (read again)."""
import hashlib
import sys
from pathlib import Path

SIZE = 0x100000
TRIM_AT, TRIM = 0xC1224, b"trimdat"


def check(a, b):
    da, db = Path(a).read_bytes(), Path(b).read_bytes()
    problems = []
    for name, d in ((a, da), (b, db)):
        if len(d) != SIZE:
            problems.append(f"{name}: {len(d)} bytes, not {SIZE}")
    if da != db:
        problems.append("the two reads differ")
    if da[TRIM_AT:TRIM_AT + len(TRIM)] != TRIM:
        problems.append(f"no calibration record at {TRIM_AT:#x} (expected {TRIM.decode()})")
    if da[:SIZE] == b"\xFF" * len(da[:SIZE]):
        problems.append("the read is blank")
    return problems, hashlib.sha256(da).hexdigest()


def main(argv):
    if len(argv) != 2:
        print(__doc__)
        return 2
    problems, sha = check(*argv)
    if problems:
        for p in problems:
            print("NOT OK:", p)
        print("Do not install yet: read the flash again.")
        return 1
    print(f"OK: 1 MiB, both reads equal, calibration present. SHA-256 {sha}")
    print("Keep a copy in two places: 0xC0000-0xC1FFF and 0xFF000-0xFFFFF are unique to this keyboard.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
