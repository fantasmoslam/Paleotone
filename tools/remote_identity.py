#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact renderer/source identity shared by device and WebAssembly builds."""
import hashlib
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def render_id():
    files = list((ROOT / "firmware").rglob("*.c")) + list((ROOT / "firmware").rglob("*.h"))
    files += list((ROOT / "assets/fonts").glob("*.ttf"))
    files += [ROOT / "tools" / n for n in ("gen_aa_font.py", "gen_aa_icons.py", "gen_aa_keycaps.py",
              "gen_ui_palettes.py", "gen_ghoulbox_art.py", "remote_identity.py")]
    files += [ROOT / "web/emu/felucca_web.c"]
    h = hashlib.sha256()
    for p in sorted(files, key=lambda x: x.relative_to(ROOT).as_posix()):
        h.update(p.relative_to(ROOT).as_posix().encode() + b"\0")
        h.update(p.read_bytes().replace(b"\r\n", b"\n") + b"\0")
    return int.from_bytes(h.digest()[:4], "little")
if __name__ == "__main__":
    print(render_id())
