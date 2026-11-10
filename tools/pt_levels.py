#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""T-WRECKS: Paleotone's sounds levelled against each layer's own voice (firmware/src/pt/pt.c, pt_snd_t g).

Runs build/host/twr_voice_test (tests/run_tests.sh builds it), reads each sound's level against its layer's own
voice (dB, by RMS, the layer alone) and writes each sound's g in pt.c so that it sits at 0 dB: g times 10^(-dB/20),
within x1/8 .. x8. The Sequence's sounds are Sequence 2's too: their g from the Sequence's run.
  tools/pt_levels.py [SECS]      (twr_voice_test's seconds a sound; default 8)"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PT = ROOT / "firmware/src/pt/pt.c"
TABLES = [("DRONE_SND", 0, 0), ("SEQ_SND", 1, 0), ("LEAD_SND", 2, 0), ("BASS_SND", 3, 0)]   # (table, layer, first k)


def main():
    secs = sys.argv[1] if len(sys.argv) > 1 else "8"
    out = subprocess.run([str(ROOT / "build/host/twr_voice_test"), secs], capture_output=True, text=True).stdout
    db = {}
    for m in re.finditer(r"\(level (\d+) (\d+) ([-+]\d+\.\d+)\)", out):
        db[(int(m[1]), int(m[2]))] = float(m[3])
    if not db:
        raise SystemExit("pt_levels: no levels from twr_voice_test (built? build/host/twr_voice_test)")
    src = PT.read_text()
    for table, layer, k0 in TABLES:
        a = src.index(f"static const pt_snd_t {table}[")
        b = src.index("};", a)
        body = src[a:b]
        k = k0
        def one(m):
            nonlocal k
            g = float(m[4]) if m[4] else 1.0
            d = db.get((layer, k))
            k += 1
            if d is None:
                return m[0]
            g = min(max(g * 10 ** (-d / 20), g / 8, 0.05), g * 8)
            return f'{{{m[1]}, "{m[2]}", "{m[3]}", {g:.3f}f}}'
        body = re.sub(r'\{(\d+), "([^"]*)", "([^"]*)"(?:, ([0-9.]+)f)?\}', one, body)
        src = src[:a] + body + src[b:]
    PT.write_text(src)
    for (l, k), d in sorted(db.items()):
        print(f"layer {l} sound {k:2d}: {d:+.1f} dB")


if __name__ == "__main__":
    main()
