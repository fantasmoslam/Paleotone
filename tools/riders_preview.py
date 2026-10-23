#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# RIDERS OF THE VALLEY, four-track native arrangement (tools/riders_native_plan.py) auditioned through the
# firmware's own engines (build/host/stem, dry). Light mix only: no tape wow/flutter, no chorus, no hiss, and ONE
# fixed master gain shared by every clip, so that lead-level options are compared at their real levels.
#   python3 tools/riders_preview.py OUTDIR
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np
from scipy import signal

sys.path.insert(0, str(Path(__file__).resolve().parent))
import riders_native_plan as plan  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
STEM = ROOT / "build/host/stem"
SR = 44100
BPM = plan.BPM
BEAT = 60.0 / BPM
BAR = 4 * BEAT
N = int((plan.BARS * BAR + 6.0) * SR)
TMP = Path(tempfile.mkdtemp(prefix="riders_pv"))
E = {"ROAR": 14, "FM6": 12, "DRUM": 10}
MASTER = 1.58         # fixed, never re-normalised per clip (peaks about -3.5 dBFS)


def render(eng, preset, notes, secs=None, edits=()):
    secs = secs or max(b + l for b, _, l, _ in notes) * BEAT + 5.0
    score = [f"{E[eng]} {preset}|{BPM}|{secs}"] + [f"e {k} {v}" for k, v in edits]
    score += [f"n {b:.4f} {nt} {l:.4f} {v}" for b, nt, l, v in notes]
    out = TMP / f"{preset.replace(' ', '_')}_{len(notes)}_{secs:.0f}.wav"
    subprocess.run([str(STEM), str(out)], input="\n".join(score) + "\n", text=True, check=True,
                   stdout=subprocess.DEVNULL)
    w = wave.open(str(out))
    assert w.getframerate() == SR, w.getframerate()
    return np.frombuffer(w.readframes(w.getnframes()), np.int16).reshape(-1, 2).astype(float).mean(1) / 32768


def filt(x, kind, f, order=2):
    return signal.sosfilt(signal.butter(order, f, kind, fs=SR, output="sos"), x)


def put(bus, x, at):
    m = min(len(x), len(bus) - at)
    if m > 0:
        bus[at:at + m] += x[:m]


def at(bar, beat=0.0):
    return int((bar * 4 + beat) * BEAT * SR)


def evs(track):
    return [(e["bar"] * 4 + e["beat"], e["note"], e["duration"], e["velocity"]) for e in track["events"]]


def stems():
    d = plan.build()
    t = {x["track"]: x for x in d["tracks"]}
    roar = np.zeros(N)
    for s in d["sections"]:                         # one held D per section, rhythm preset switching
        nb = s["bars"]
        x = render("ROAR", s["roar_preset"], [(0, 38, nb * 4 + 0.25, 110)], secs=nb * BAR + 3)
        fade = np.ones(len(x))
        fi = int(0.05 * SR)
        fade[:fi] = np.linspace(0, 1, fi)
        end = int(nb * BAR * SR)
        if end < len(x):
            fade[end:] = np.exp(-np.arange(len(x) - end) / (0.25 * SR))
        put(roar, x * fade, at(s["start_bar"]))
    lead = render("ROAR", "PRIMEVAL", evs(t[2]))
    bass = render("FM6", "TAR PIT BASS", evs(t[3]))
    drums = render("DRUM", "WAR DRUMS", evs(t[4]), edits=[(1, 14)])
    pad = lambda x: np.pad(x, (0, max(0, N - len(x))))[:N]
    return {"roar": pad(roar), "lead": pad(lead), "bass": pad(bass), "drums": pad(drums)}


def db(x):
    return 20 * np.log10(np.sqrt(np.mean(np.square(x))) + 1e-12)


def band(x, lo, hi):
    return signal.sosfilt(signal.butter(3, [lo, hi], "bandpass", fs=SR, output="sos"), x)


# ------------------------------------------------------------------ audition clips (track volumes only: device-reproducible)
LEAD_OPTIONS = {"A_lead_-8dB": -8.0, "B_lead_-12dB": -12.0, "C_lead_-16dB": -16.0}


def mix(s, lead_db, parts=("roar", "bass", "drums", "lead")):
    g = {"roar": 0.0, "bass": 0.0, "drums": 0.0, "lead": lead_db}
    return sum(s[k] * 10 ** (g[k] / 20) for k in parts) * MASTER


def clip(x, b0, b1, fade=0.4):
    y = x[at(b0):at(b1)].copy()
    f = int(fade * SR)
    y[:f] *= np.linspace(0, 1, f)
    y[-f:] *= np.linspace(1, 0, f)
    return y


def write(path, y):
    assert np.abs(y).max() < 1.0, f"{path} clips: {np.abs(y).max():.3f}"
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((y * 32767).astype("<i2").tobytes())


def clips(s, outdir):
    write(outdir / "1_intro_bars0-6_roar_only.wav", clip(mix(s, 0, ("roar",)), 0, 6))
    for name, g in LEAD_OPTIONS.items():
        write(outdir / f"2{name}_climax_bars32-38.wav", clip(mix(s, g), 32, 38))
    lo, hi = 500, 2500
    print("lead minus backing, 200-500 / 500-2500 Hz, bars 32-40:")
    bk = sum(s[k] for k in ("roar", "bass", "drums"))[at(32):at(40)]
    for name, g in LEAD_OPTIONS.items():
        ld = s["lead"][at(32):at(40)] * 10 ** (g / 20)
        print(f"  {name:14s} {db(band(ld, 200, 500)) - db(band(bk, 200, 500)):+5.1f}"
              f" / {db(band(ld, lo, hi)) - db(band(bk, lo, hi)):+5.1f} dB")


# ------------------------------------------------------------------ approved balance (owner chose option B, 2026-10-23)
LEAD_DB = -12.0


def full(s, outdir):
    y = mix(s, LEAD_DB)
    y = y[: at(plan.BARS) + int(4 * SR)]
    y[-int(3 * SR):] *= np.linspace(1, 0, int(3 * SR))
    write(outdir / "3_full_song_lead_-12dB.wav", y)
    bk = sum(s[k] for k in ("roar", "bass", "drums"))
    ld = s["lead"] * 10 ** (LEAD_DB / 20)
    print("section  lead-minus-backing 500-2500 Hz")
    for a, b in ((10, 16), (16, 24), (32, 40), (40, 44), (44, 48)):
        print(f"  {a:2d}-{b:2d}  {db(band(ld[at(a):at(b)], 500, 2500)) - db(band(bk[at(a):at(b)], 500, 2500)):+5.1f} dB")


if __name__ == "__main__":
    outdir = Path(sys.argv[1] if len(sys.argv) > 1 else "build/riders_preview")
    outdir.mkdir(parents=True, exist_ok=True)
    s = stems()
    np.savez_compressed(outdir / "stems.npz", **s)
    clips(s, outdir)
    full(s, outdir)
    for k, v in s.items():
        print(f"{k:6s} " + " ".join(f"{db(v[at(a):at(b)]):6.1f}" for a, b in ((0, 8), (10, 16), (16, 24), (32, 40))))
