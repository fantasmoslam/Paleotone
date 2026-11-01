#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""SAURUS's CALL recipes (eng_saurus.c), as designed: each call is up to three throats (a low rumble, the body, a
scream or whistle) with their own pitch and loudness curves, made by ear against CC0 animal / creature recordings
(Freesound: CaveboyTup "T-Rex Calls", quadraslayer "Monster Roar 2", JarredGibb "Dinosaur 3/4", JhennaSide
"dinosaur", SieuAmThanh "Dinosaur 0", ItsThatRoboGuy "Underwater Crab-like Monster Growl"; no sample data is used:
only their measured pitch, resonances, roughness and timing). Writes firmware/src/saurus_calls.h.
  python3 tools/saurus_design.py
Units as the engine reads them: pitch points in semitones (relative to the throat's OFF), times 0..1 of LEN,
formants and widths in Hz, rates in Hz, depths 0..1, drive x (1 = none)."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NAMES = ["HOLD", "BELLOW", "ROAR", "BARK", "HOOT", "TRILL", "CHIRP", "MOAN", "HISS", "SNORT", "SHRIEK", "HONK", "BOOM", "RATTLE"]


def T(off, lvl, fc, fo, bw, amp, pit, bright=0.5, breath=0.2, sub=0.0, growl=0.0, grate=20, burst=0.0, gfloor=0.0,
      jit=0.02, shim=0.1, drive=1.5, hp=0, lp=0, vib=(0, 0)):
    """a throat: amp / pit are [(t, v)] (v: 0..1 / semitones), resampled onto the call's six times"""
    return dict(off=off, lvl=lvl, fc=fc, fo=fo, bw=bw, amp=amp, pit=pit, bright=bright, breath=breath, sub=sub,
                growl=growl, grate=grate, burst=burst, gfloor=gfloor, jit=jit, shim=shim, drive=drive, hp=hp, lp=lp, vib=vib)


CALLS = {
    # as played: a drone, the body and a rumble an octave down, steady
    "HOLD": dict(t=[0, .1, .3, .6, .9, 1], mouth=[(0, .6), (1, .6)], throats=[
        T(0, 1.0, [450, 900, 1500], [600, 1100, 1800], [90, 120, 180], [(0, 1), (1, 1)], [(0, 0), (1, 0)],
          bright=.55, breath=.2, sub=.2, growl=.2, grate=18, jit=.015, shim=.12, drive=1.8, hp=150),
        T(-12, .45, [250, 560, 1100], [300, 700, 1400], [100, 150, 220], [(0, 1), (1, 1)], [(0, 0), (1, 0)],
          bright=.35, breath=.15, sub=.3, growl=.35, grate=26, jit=.02, shim=.2, drive=1.8)]),
    # a bellow: swells, rises a little, sinks; body and rumble, the mouth opening slowly (CaveboyTup's 26-30 s)
    "BELLOW": dict(t=[0, .12, .35, .6, .85, 1], mouth=[(0, .1), (.5, .9), (1, .4)], throats=[
        T(0, 1.0, [420, 820, 1300], [560, 1150, 1700], [80, 110, 170], [(0, 0), (.12, .55), (.35, .9), (.6, 1), (.85, .8), (1, 0)],
          [(0, 0), (.35, 2), (.6, 1), (1, -5)], bright=.55, breath=.25, sub=.3, growl=.3, grate=14, jit=.02, shim=.2,
          drive=2.2, hp=200),
        T(-12, .5, [240, 520, 1050], [300, 680, 1400], [100, 150, 230], [(0, 0), (.12, .5), (.6, .8), (.85, .6), (1, 0)],
          [(0, 0), (.35, 2), (1, -5)], bright=.35, breath=.2, sub=.45, growl=.5, grate=24, jit=.03, shim=.25, drive=2.0)]),
    # the tyrant's roar: rumble an octave down, the body, a scream up ~20 st swelling in (prototype "tyrant")
    "ROAR": dict(t=[0, .08, .25, .5, .8, 1], mouth=[(0, .2), (.25, .7), (.5, 1), (.8, .7), (1, .4)], throats=[
        T(-12, .3, [260, 560, 1150], [320, 720, 1500], [110, 160, 240], [(0, 0), (.08, .7), (.5, .8), (.85, .6), (1, 0)],
          [(0, .6), (.2, 4.5), (.6, 3.6), (1, -.6)], bright=.35, breath=.3, sub=.45, growl=.55, grate=28, jit=.03,
          shim=.25, drive=2.2),
        T(0, 1.0, [500, 950, 1450], [600, 1100, 1700], [80, 110, 160], [(0, 0), (.12, .8), (.55, 1), (.9, .75), (1, 0)],
          [(0, 0), (.25, 5.5), (.6, 3.9), (1, -.7)], bright=.7, breath=.25, sub=.25, growl=.35, grate=15, jit=.025,
          shim=.2, drive=2.6, hp=320),
        T(20, .6, [1000, 1900, 3000], [1250, 2300, 3400], [160, 240, 320], [(0, 0), (.2, .15), (.45, .6), (.8, .75), (1, 0)],
          [(0, 0), (.3, 5.4), (.7, 6.7), (1, 2.1)], bright=.7, breath=1.0, sub=.1, growl=.25, grate=40, jit=.05,
          shim=.35, drive=1.8, hp=600)]),
    # a raptor's bark: short, the raspy body and a whistle, ending by itself
    "BARK": dict(t=[0, .04, .15, .4, .7, 1], mouth=[(0, 1), (.4, .6), (1, .2)], throats=[
        T(0, 1.0, [750, 1600, 2500], [900, 1800, 2700], [200, 280, 360], [(0, 0), (.04, 1), (.15, .8), (.4, .35), (.7, .08), (1, 0)],
          [(0, 3), (.15, 0), (1, -6)], bright=.6, breath=1.2, sub=.3, growl=.5, grate=98, jit=.06, shim=.4, drive=2.4, hp=150),
        T(41, .5, [2600, 3700, 5000], [2700, 3850, 5200], [140, 160, 400], [(0, 0), (.04, .8), (.15, .5), (.4, .1), (1, 0)],
          [(0, 2), (.15, 0), (1, -5)], bright=.2, breath=.1, jit=.008, shim=.05, drive=1.2, vib=(11, .025))]),
    # a closed-mouth hoot: one round tone and its octave below, soft
    "HOOT": dict(t=[0, .1, .3, .6, .9, 1], mouth=[(0, 0), (1, .1)], throats=[
        T(0, 1.0, [300, 700, 1100], [330, 760, 1200], [40, 70, 100], [(0, 0), (.1, .9), (.6, 1), (.9, .8), (1, 0)],
          [(0, -.5), (.1, 0), (1, -.5)], bright=.25, breath=.15, sub=.05, growl=.05, jit=.008, shim=.05, drive=1.3),
        T(-12, .35, [150, 380, 700], [170, 420, 760], [40, 60, 100], [(0, 0), (.15, .7), (.8, .7), (1, 0)], [(0, 0), (1, 0)],
          bright=.2, breath=.05, jit=.006, shim=.05, drive=1.2)]),
    # a raptor's trill: a whistle fluttered at ~23 Hz, a dark body under it (JhennaSide's)
    "TRILL": dict(t=[0, .05, .3, .6, .9, 1], mouth=[(0, .8), (1, .8)], throats=[
        T(0, 1.0, [1000, 2400, 3500], [1100, 2600, 3700], [120, 200, 300], [(0, 0), (.05, 1), (.6, .9), (.9, .6), (1, 0)],
          [(0, 0), (.3, .5), (.6, -.3), (1, -2)], bright=.4, breath=.15, growl=.8, grate=23, jit=.01, shim=.1, drive=1.4,
          hp=500, vib=(6, .02)),
        T(-12, .4, [600, 1300, 2200], [700, 1500, 2400], [150, 220, 300], [(0, 0), (.05, .6), (.9, .4), (1, 0)], [(0, 0), (1, -2)],
          bright=.5, breath=.5, growl=.6, grate=23, jit=.02, shim=.2, drive=1.6, hp=250)]),
    # a fast rising squeal: a whistle sweeping up, a little rasp
    "CHIRP": dict(t=[0, .05, .25, .5, .75, 1], mouth=[(0, .3), (.4, 1), (1, .6)], throats=[
        T(0, 1.0, [1300, 2600, 3800], [1500, 2900, 4000], [140, 200, 300], [(0, 0), (.05, .8), (.25, 1), (.5, .7), (.75, .3), (1, 0)],
          [(0, -12), (.25, 3), (.5, 5), (1, 2)], bright=.4, breath=.2, jit=.01, shim=.08, drive=1.4, hp=600, vib=(9, .015)),
        T(0, .4, [800, 1700, 2600], [950, 1900, 2800], [220, 300, 380], [(0, 0), (.05, .6), (.5, .4), (1, 0)], [(0, -12), (.25, 3), (1, 2)],
          bright=.6, breath=1.0, growl=.4, grate=90, jit=.05, shim=.35, drive=2.0, hp=300)]),
    # a herd's moan: a slow falling groan, body and rumble, the mouth half open
    "MOAN": dict(t=[0, .15, .35, .6, .85, 1], mouth=[(0, .3), (.4, .6), (1, .2)], throats=[
        T(0, 1.0, [380, 760, 1250], [480, 950, 1500], [70, 100, 150], [(0, 0), (.15, .7), (.35, 1), (.6, .9), (.85, .6), (1, 0)],
          [(0, 1), (.35, 0), (.6, -2), (1, -5)], bright=.45, breath=.25, sub=.2, growl=.2, grate=12, jit=.015, shim=.15,
          drive=1.8, hp=120),
        T(-12, .55, [200, 450, 900], [240, 540, 1100], [60, 100, 160], [(0, 0), (.15, .6), (.6, .8), (1, 0)], [(0, 1), (1, -5)],
          bright=.3, breath=.15, sub=.3, growl=.35, grate=20, jit=.02, shim=.2, drive=1.6)]),
    # a hiss: breath only, bright, through an open mouth
    "HISS": dict(t=[0, .08, .3, .6, .85, 1], mouth=[(0, 1), (1, .8)], throats=[
        T(19, 1.0, [2000, 3200, 4500], [2400, 3600, 5000], [400, 600, 800], [(0, 0), (.08, 1), (.6, .8), (.85, .5), (1, 0)],
          [(0, 0), (1, 0)], bright=0, breath=1.6, growl=.3, grate=40, jit=.02, shim=.3, drive=1.3, hp=1200)]),
    # a snort: a short burst of noise through the nose, low
    "SNORT": dict(t=[0, .03, .12, .3, .6, 1], mouth=[(0, .3), (1, 0)], throats=[
        T(0, 1.0, [400, 900, 1800], [500, 1100, 2000], [250, 350, 450], [(0, 0), (.03, 1), (.12, .5), (.3, .1), (.6, 0), (1, 0)],
          [(0, 0), (1, 0)], bright=0, breath=1.6, growl=.5, grate=35, jit=.03, shim=.4, drive=1.8, hp=150)]),
    # a raptor's shriek: two whistles not in tune with each other (a bird's two-sided syrinx) wobbling at 11 Hz, over a
    # raspy body (JarredGibb's "Dinosaur 4"); from B3 (the body ~250 Hz) its whistles at ~2.6 and ~3.7 kHz
    "SHRIEK": dict(t=[0, .04, .4, .5, .7, 1], mouth=[(0, .5), (1, .8)], throats=[
        T(0, 1.0, [750, 1600, 2500], [900, 1800, 2700], [200, 280, 360], [(0, 0), (.06, .6), (.6, .85), (1, 0)],
          [(0, 0), (.5, 1.9), (1, -.7)], bright=.6, breath=1.4, sub=.3, growl=.5, grate=98, jit=.06, shim=.4, drive=2.4, hp=150),
        T(41, .5, [2600, 5200, 7800], [2650, 5300, 7900], [140, 400, 600], [(0, 0), (.04, 1), (.4, .85), (.5, .3), (.7, .12), (1, 0)],
          [(0, -1.2), (.06, 0), (.45, .1), (1, -.8)], bright=.15, breath=.08, jit=.006, shim=.05, drive=1.2, vib=(11, .022)),
        T(47, .3, [3700, 7400, 9000], [3750, 7500, 9000], [160, 400, 600], [(0, 0), (.04, .7), (.4, .6), (.5, .2), (.7, .08), (1, 0)],
          [(0, -1.0), (.06, 0), (.45, .1), (1, -.7)], bright=.15, breath=.08, jit=.006, shim=.05, drive=1.2, vib=(11, .022))]),
    # a hadrosaur's honk: a resonant crest, a long horn-like call with a slight rise, a rumble an octave down
    "HONK": dict(t=[0, .12, .4, .75, .9, 1], mouth=[(0, 0), (.3, .8), (1, .5)], throats=[
        T(0, 1.0, [230, 700, 1150], [260, 760, 1250], [35, 60, 90], [(0, 0), (.12, .9), (.75, 1), (1, 0)],
          [(0, 0), (.1, 2.0), (.7, 2.6), (1, -.9)], bright=.25, breath=.12, sub=.08, growl=.1, grate=20, jit=.008,
          shim=.05, drive=1.4, lp=1800),
        T(-12, .6, [110, 350, 580], [130, 380, 620], [30, 50, 80], [(0, 0), (.15, .5), (.8, .5), (1, 0)], [(0, 0), (1, -.7)],
          bright=.2, breath=.05, growl=.05, grate=20, jit=.006, shim=.05, drive=1.3)]),
    # a sauropod's boom: closed mouth, deep, swelling and fading; its harmonics carry it on a small speaker
    "BOOM": dict(t=[0, .2, .4, .7, .9, 1], mouth=[(0, 0), (.5, .4), (1, 0)], throats=[
        T(0, 1.0, [90, 250, 520], [110, 300, 600], [30, 50, 90], [(0, 0), (.3, .9), (.7, 1), (1, 0)],
          [(0, 0), (.4, 2.8), (1, -1.0)], bright=.2, breath=.1, sub=.2, growl=.3, grate=9, jit=.01, shim=.1, drive=1.8, lp=900),
        T(12, .55, [160, 420, 800], [190, 480, 900], [40, 70, 120], [(0, 0), (.35, .4), (.7, .5), (1, 0)],
          [(0, 0), (.4, 2.8), (1, -1.0)], bright=.25, breath=.1, sub=.15, growl=.2, grate=12, jit=.01, shim=.1, drive=1.5, lp=1200)]),
    # a deep rattling growl: bursts about 5.5 a second, its pitch flipping an octave (ItsThatRoboGuy's)
    "RATTLE": dict(t=[0, .06, .15, .5, .75, 1], mouth=[(0, .2), (.5, .8), (1, .3)], throats=[
        T(0, 1.0, [200, 480, 1000], [240, 600, 1250], [130, 200, 300], [(0, 0), (.06, .9), (.6, 1), (.85, .7), (1, 0)],
          [(0, -1.5), (.15, 0), (.5, -.4), (.75, -2.3), (1, -4.1)], bright=.4, breath=.35, sub=.5, growl=1.0, grate=5.5,
          burst=3.0, gfloor=.06, jit=.03, shim=.2, drive=1.5, hp=70, lp=1300)]),
}
assert list(CALLS) == NAMES
# each call's output gain (16 = x1), set by tools/saurus_design.py --calibrate: at its reference note (REF) with the
# knobs at 64, a peak of -4 dBFS through a track at LEVEL 110 (3 dB above the old calls: about as loud to the ear)
GAIN = {"HOLD": 54, "BELLOW": 58, "ROAR": 55, "BARK": 55, "HOOT": 130, "TRILL": 84, "CHIRP": 99, "MOAN": 61, "HISS": 107,
        "SNORT": 49, "SHRIEK": 60, "HONK": 124, "BOOM": 65, "RATTLE": 111}
REF = {"HOLD": 48, "BELLOW": 45, "ROAR": 47, "BARK": 59, "HOOT": 52, "TRILL": 67, "CHIRP": 72, "MOAN": 48, "HISS": 60,
       "SNORT": 48, "SHRIEK": 59, "HONK": 45, "BOOM": 25, "RATTLE": 54}


def at(curve, t):
    """[(t, v)] at t, linear, held at the ends"""
    if t <= curve[0][0]:
        return curve[0][1]
    for (t0, v0), (t1, v1) in zip(curve, curve[1:]):
        if t <= t1:
            return v0 + (v1 - v0) * (t - t0) / (t1 - t0) if t1 > t0 else v1
    return curve[-1][1]


def u8(x, lo=0, hi=255):
    return int(max(lo, min(hi, round(x))))


def throat_c(th, tm):
    fc = [u8(f / 16) for f in th["fc"]]
    fo = [u8(f / 16) for f in th["fo"]]
    bw = [u8(b / 4) for b in th["bw"]]
    amp = [u8(at(th["amp"], t) * 255) for t in tm]
    pit = [u8(at(th["pit"], t) * 4, -127, 127) for t in tm]
    f = [th["off"], u8(th["lvl"] * 255), fc, fo, bw, u8(th["bright"] * 127), u8(th["breath"] * 160), u8(th["sub"] * 163, 0, 127),
         u8(th["growl"] * 127, 0, 127), u8(th["grate"]), u8(th["burst"] * 16), u8(th["gfloor"] * 255), u8(th["jit"] * 1000),
         u8(th["shim"] * 163, 0, 127), u8((th["drive"] - 1) * 16), u8(th["hp"] / 8), u8(th["lp"] / 32), u8(th["vib"][0] * 4),
         u8(th["vib"][1] * 1731 / 2), amp, pit]   # (vibrato depth: x of the pitch -> cents / 2; 1731 = 1200 / ln 2)
    def c(v):
        return "{" + ", ".join(map(str, v)) + "}" if isinstance(v, list) else str(v)
    return "{" + ", ".join(c(v) for v in f) + "}"


def main():
    out = ["/* SPDX-License-Identifier: GPL-3.0-only */",
           "/* SAURUS's CALL recipes: written by tools/saurus_design.py (edit that, run it again) */",
           "static const scall_t SAUR_CALL[] = {"]
    for name in NAMES:
        c = CALLS[name]
        tm = c["t"]
        ths = [throat_c(th, tm) for th in c["throats"]]
        while len(ths) < 3:
            ths.append("SAUR_NONE")
        out.append(f"    /* {name} */")
        out.append("    {{" + ", ".join(str(u8(t * 255)) for t in tm) + "}, {" +
                   ", ".join(str(u8(at(c["mouth"], t) * 255)) for t in tm) + f"}}, {len(c['throats'])}, {GAIN[name]}, {{")
        for th in ths:
            out.append(f"        {th},")
        out.append("    }},")
    out.append("};")
    (ROOT / "firmware/src/saurus_calls.h").write_text("\n".join(out) + "\n")
    print("firmware/src/saurus_calls.h:", len(NAMES), "calls")


def calibrate():
    """render every call at REF with the knobs at 64 (build/host/saurus_render), set GAIN for a -4 dBFS peak (the layered calls are spikier: as loud as the old ones to the ear)"""
    import re, subprocess, tempfile, wave, struct
    import numpy as np
    main()
    subprocess.run(["cc", "-O2", "-w", '-DFELUCCA_VERSION="x"', "-Ibuild/gen", "-Ifirmware/src", "-Itests", "-o",
                    "build/host/saurus_render", "tests/saurus_render.c", "-lm"], cwd=ROOT, check=True)
    src = (ROOT / "tools/saurus_design.py").read_text()
    for k, name in enumerate(NAMES):
        with tempfile.NamedTemporaryFile(suffix=".wav") as f:
            subprocess.run(["build/host/saurus_render", f.name, str(REF[name]), "4", "2.5"], cwd=ROOT, check=True,
                           env={"DRY": "1", "CALL": str(k), "LEN": "106"})
            w = wave.open(f.name)
            x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(float) / 32768
        pk = 20 * np.log10(np.abs(x).max() + 1e-9)
        g = max(4, min(255, round(GAIN[name] * 10 ** ((-4 - pk) / 20))))
        print(f"{name:7s} peak {pk:6.1f} dB  gain {GAIN[name]} -> {g}")
        src = re.sub(rf'"{name}": \d+', f'"{name}": {g}', src, count=1)
    (ROOT / "tools/saurus_design.py").write_text(src)


if __name__ == "__main__":
    import sys
    calibrate() if "--calibrate" in sys.argv else main()
