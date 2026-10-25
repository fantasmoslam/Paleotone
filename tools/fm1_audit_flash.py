#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only comparison of a recovery flash dump against two .fwsc packages.

No USB access, writes, recovery commands or device imports. Prints JSON only.
  python tools/fm1_audit_flash.py FAILED.bin OLD.fwsc NEW.fwsc [--second-backup FAILED2.bin]
"""
import argparse
import hashlib
import json
import struct
from binascii import crc_hqx
from pathlib import Path

APP_LO, APP_HI, SECTOR = 0x4000, 0x93000, 0x1000


def decode(data, key=0xFFFF):
    out = bytearray(data)
    for i in range(len(out)):
        out[i] ^= key & 255
        key = ((key << 1) ^ (0x1021 if key & 0x8000 else 0)) & 65535
    return bytes(out)


def package_flash(raw):
    if len(raw) < 960:
        raise ValueError("truncated identity blocks")
    logical = b"".join(raw[i * 48:i * 48 + 47] for i in range(20)) + raw[960:]
    h = decode(logical[:64])
    total, count = struct.unpack_from("<IH", h, 4)
    directory = logical[64:64 + count * 80]
    if (len(h) != 64 or total != len(logical) or not 1 <= count <= 11 or
            crc_hqx(h[2:], 0) != int.from_bytes(h[:2], "little") or
            crc_hqx(directory, 0) != int.from_bytes(h[2:4], "little")):
        raise ValueError("invalid package header or directory")
    flash = None
    for i in range(count):
        e = decode(directory[i * 80:(i + 1) * 80])
        kind, _, crc, _, off, size = struct.unpack_from("<HHHHII", e)
        if not size or off < 64 + count * 80 or off + size > len(logical):
            raise ValueError("invalid file extent")
        content = logical[off:off + size]
        if crc_hqx(content, 0) != crc:
            raise ValueError("package file CRC mismatch")
        if kind == 0:
            if flash is not None:
                raise ValueError("duplicate flash entry")
            flash = content
    if flash is None or not APP_HI <= len(flash) <= 0x100000:
        raise ValueError("missing or undersized flash entry")
    return flash


def app_checks(flash, key):
    encrypted = flash[APP_LO:APP_HI]
    area = b"".join(decode(encrypted[i:i + 32], key ^ (i >> 2))
                    for i in range(0, len(encrypted), 32))
    area_size = int.from_bytes(area[8:12], "little")
    off, size = struct.unpack_from("<II", area, 36)
    if not 32 <= area_size <= len(area) or off < 64 or off + size > len(area):
        raise ValueError("invalid decrypted app extents (wrong key or damaged header)")
    return {
        "area_header_crc_ok": crc_hqx(area[2:32], 0) == int.from_bytes(area[:2], "little"),
        "app_entry_crc_ok": crc_hqx(area[34:64], 0) == int.from_bytes(area[32:34], "little"),
        "area_crc_expected": int.from_bytes(area[2:4], "little"),
        "area_crc_actual": crc_hqx(area[32:area_size], 0),
        "app_crc_expected": int.from_bytes(area[34:36], "little"),
        "app_crc_actual": crc_hqx(area[off:off + size], 0),
    }


def audit(backup, old_raw, new_raw, key=0x980F):
    if len(backup) != 0x100000:
        raise ValueError("backup must be exactly 1 MiB")
    old, new = package_flash(old_raw), package_flash(new_raw)
    groups = []
    for off in range(APP_LO, APP_HI, SECTOR):
        part = backup[off:off + SECTOR]
        is_old, is_new = part == old[off:off + SECTOR], part == new[off:off + SECTOR]
        kind = "both" if is_old and is_new else "new" if is_new else "old" if is_old else "neither"
        if groups and groups[-1]["kind"] == kind:
            groups[-1]["end_exclusive"] = off + SECTOR
            groups[-1]["sectors"] += 1
        else:
            groups.append({"start": off, "end_exclusive": off + SECTOR, "sectors": 1, "kind": kind})
    record = backup[0xE4F00:0xE4F50]
    return {
        "backup_sha256": hashlib.sha256(backup).hexdigest(),
        "old_package_sha256": hashlib.sha256(old_raw).hexdigest(),
        "new_package_sha256": hashlib.sha256(new_raw).hexdigest(),
        "application_sector_groups": groups,
        "backup_checks": app_checks(backup, key),
        "old_checks": app_checks(old, key),
        "new_checks": app_checks(new, key),
        "update_record_crc_ok": crc_hqx(record[2:], 0) == int.from_bytes(record[:2], "little"),
        "update_record_type": int.from_bytes(record[6:8], "little"),
        "update_record_loader_offset": int.from_bytes(record[72:76], "little"),
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("backup", type=Path)
    ap.add_argument("old_package", type=Path)
    ap.add_argument("new_package", type=Path)
    ap.add_argument("--second-backup", type=Path)
    ap.add_argument("--key", type=lambda s: int(s, 0), default=0x980F)
    a = ap.parse_args()
    try:
        backup = a.backup.read_bytes()
        result = audit(backup, a.old_package.read_bytes(), a.new_package.read_bytes(), a.key)
        if a.second_backup:
            result["independent_backups_identical"] = backup == a.second_backup.read_bytes()
        print(json.dumps(result, indent=2))
    except (OSError, ValueError, struct.error) as e:
        ap.exit(2, f"audit failed: {e}\n")


if __name__ == "__main__":
    main()
