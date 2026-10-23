#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DINOBOX soundscape maker: biome previews (2026-10-23; the plan: docs/NOTES.md). python3 tools/biome_preview.py [BIOME ..]
# -> build/biomes/biome_<NAME>.wav and dinobox_biomes.mp3 (needs build/host/stem, ffmpeg). The creatures come from
# tests/stem.c (the firmware's SAURUS, dry). Procedural layers (each one cheap enough to run on the FM-1: noise
# through filters, decaying sine chirps, random event timing) plus DINOBOX's own sounds (SAURUS calls rendered dry by
# the firmware's code, the ENCOUNTER kit). Generative: every event's time, pitch, level and distance is drawn by
# chance under the biome's rules. 30 s each.
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np
from scipy import signal

ROOT = Path(__file__).resolve().parent.parent
S = ROOT / "build/biomes"
S.mkdir(parents=True, exist_ok=True)
KIT = ROOT / "assets/samples-cc0/STEPS"
SR = 44100
DUR = 30.0
N = int(SR * DUR)
TAU = 2 * np.pi
rng = np.random.default_rng(65)


# ------------------------------------------------------------------ tools
def sos(kind, f, order=2):
    return signal.butter(order, f, kind, fs=SR, output="sos")


def filt(x, kind, f, order=2):
    return signal.sosfilt(sos(kind, f, order), x)


def noise(n):
    return rng.standard_normal(n)


def slow(n, rate, lo=0.0, hi=1.0, seed=None):
    """a smooth random curve (rate: changes a second)"""
    k = max(4, int(n / SR * rate) + 4)
    pts = rng.random(k)
    x = np.interp(np.linspace(0, k - 3, n), np.arange(k), pts)
    x = filt(x, "lowpass", max(0.05, rate), 2)
    x = (x - x.min()) / (np.ptp(x) + 1e-9)
    return lo + (hi - lo) * x


def env_ad(n, a, d):
    t = np.arange(n) / SR
    return np.where(t < a, t / max(a, 1e-4), np.exp(-(t - a) / d))


def place(buf, ev, at, pan=0.0, gain=1.0):
    """mix a mono event into the stereo buffer at sample `at`, panned -1..1"""
    at = int(at)
    if at >= len(buf) or at + len(ev) <= 0:
        return
    s = max(0, -at)
    ev = ev[s:]
    at = max(0, at)
    m = min(len(ev), len(buf) - at)
    l, r = np.cos((pan + 1) * np.pi / 4), np.sin((pan + 1) * np.pi / 4)
    buf[at:at + m, 0] += ev[:m] * gain * l
    buf[at:at + m, 1] += ev[:m] * gain * r


def far(ev, d):
    """distance 0 (near) .. 1 (far): darker, quieter"""
    if d > 0.05:
        ev = filt(ev, "lowpass", 9000 * (1 - d) ** 2 + 500)
    return ev * (1 - 0.8 * d)


def verb(x, t60, wet, damp=4000, pre=0.02, echoes=None):
    n = int(SR * t60)
    t = np.arange(n) / SR
    ir = np.zeros((n + int(pre * SR), 2))
    for c in range(2):
        e = noise(n) * np.exp(-6.9 * t / t60)
        e = filt(e, "lowpass", damp)
        ir[int(pre * SR):, c] = e / np.sqrt((e ** 2).sum())
    if echoes:                                         # canyon walls: distinct slaps
        for dt, g in echoes:
            k = int(dt * SR)
            if k < len(ir):
                ir[k, 0] += g
                ir[min(len(ir) - 1, k + 300), 1] += g
    out = np.zeros_like(x)
    for c in range(2):
        out[:, c] = signal.fftconvolve(x[:, c], ir[:, c])[:len(x)]
    return x * (1 - wet) + out * wet


def load(path, gain=1.0):
    w = wave.open(str(path))
    x = np.frombuffer(w.readframes(w.getnframes()), np.int16).astype(float) / 32768
    x = x.reshape(-1, w.getnchannels()).mean(1)
    sr = w.getframerate()
    if sr != SR:
        x = signal.resample_poly(x, SR, sr)
    return x * gain


SAURUS = ["BRACHIO CALL", "TYRANT ROAR", "RAPTOR BARK", "RAPTOR CHAT", "CREST HOOT", "PTERO SHRIEK", "HERD MOAN",
          "FRILL HISS", "SAURO SNORT", "DEEP RUMBLE", "TITAN APE", "NEST CHIRPS", "SWAMP CROAK", "MONSTER ROAR",
          "DISTANT HERD", "LOW GROWL", "BABY REX", "SEA MONSTER", "HADRO HORN", "COMPY PACK"]


def call(eng, k, secs=5.0):
    """SAURUS preset k (its factory order) calling once, dry, by the firmware (tests/stem.c)"""
    out = S / f"call{k}.wav"
    score = f"15 {SAURUS[k]}|96|{secs + 4}\nn 0 50 {secs / 0.625:.3f} 110\n"
    subprocess.run([str(ROOT / "build/host/stem"), str(out)], input=score, text=True, check=True)
    x = load(out)[:int(secs * SR)]
    return x / (np.abs(x).max() + 1e-9)


STEP = {n: load(KIT / f) for n, f in [("near", "00_GiantStep.wav"), ("far", "01_FarStep.wav"), ("mud", "02_MudStomp.wav"),
        ("claw", "03_ClawTap.wav"), ("snap", "04_BranchSnap.wav"), ("tree", "05_TreeFall.wav"), ("splash", "06_Splash.wav"),
        ("rock", "07_RockFall.wav"), ("leaves", "08_Leaves.wav"), ("bone", "09_BoneClack.wav"), ("ripple", "10_Ripple.wav"),
        ("thunder", "11_Thunder.wav"), ("huff", "12_Huff.wav")]}


def times(rate, lo=0.0, hi=DUR):
    """random event times (Poisson, `rate` a second)"""
    t, out = lo + rng.exponential(1 / rate), []
    while t < hi:
        out.append(t)
        t += rng.exponential(1 / rate)
    return out


# ------------------------------------------------------------------ procedural voices (each a few operations a sample)
def wind(n, base=400, gust=0.15, howl=0.0):
    g = slow(n, gust, 0.15, 1.0)
    x = filt(noise(n), "bandpass", [80, 1600]) * g
    if howl:                                           # a resonant whistle riding the gusts
        f = base * (0.8 + 0.5 * g)
        ph = np.cumsum(TAU * f / SR)
        x += howl * np.sin(ph) * g ** 2 * (0.6 + 0.4 * filt(noise(n), "lowpass", 3))
    return x


def bubble(f0, dur=0.05, rise=1.8):
    n = int(dur * SR)
    t = np.arange(n) / SR
    f = f0 * (1 + (rise - 1) * t / dur)
    return np.sin(np.cumsum(TAU * f / SR)) * np.exp(-t / (dur / 3)) * np.minimum(1, t / 0.002)


def stream(n, rate=60, flo=300, fhi=1400, bed=0.25):
    x = bed * filt(noise(n), "bandpass", [200, 3000])
    for t in times(rate, 0, n / SR):
        b = bubble(rng.uniform(flo, fhi), rng.uniform(0.02, 0.07))
        i = int(t * SR)
        m = min(len(b), n - i)
        x[i:i + m] += b[:m] * rng.uniform(0.1, 0.5)
    return x


def waves(n, period=9.0):
    t = np.arange(n) / SR
    sw = (0.5 + 0.5 * np.sin(TAU * t / period + rng.uniform(0, 6))) ** 3
    x = filt(noise(n), "bandpass", [150, 5000]) * sw
    x += 0.5 * filt(noise(n), "highpass", 2500) * np.roll(sw, int(0.7 * SR)) ** 2   # the wash on the sand
    return x


def rain(n, density=300, level=0.5):
    x = level * filt(noise(n), "bandpass", [400, 6000])
    for t in times(density, 0, n / SR):
        m = int(rng.uniform(0.003, 0.01) * SR)
        i = int(t * SR)
        if i + m < n:
            x[i:i + m] += filt(noise(m), "highpass", 1500) * np.exp(-np.arange(m) / (m / 4)) * rng.uniform(0.2, 0.8)
    return x


def cicadas(n, fc=5200, rate=40, swell=0.2):
    b = filt(noise(n), "bandpass", [fc - 600, fc + 600])
    t = np.arange(n) / SR
    return b * (0.5 + 0.5 * np.sin(TAU * rate * t)) ** 3 * slow(n, swell, 0.1, 1.0)


def crickets(n, f0=4300, per=0.5, level=0.12):
    x = np.zeros(n)
    pl = int(0.016 * SR)
    tone = np.sin(TAU * f0 * np.arange(pl) / SR) * np.sin(np.linspace(0, np.pi, pl)) ** 2
    t = rng.uniform(0, per)
    while t < n / SR:
        for p in range(3):
            i = int((t + p * 0.032) * SR)
            if i + pl < n:
                x[i:i + pl] += tone * level
        t += per * rng.uniform(0.9, 1.1)
    return x


def frog(f0=1900, pulses=4):
    m = int(0.025 * SR * pulses)
    t = np.arange(m) / SR
    buzz = signal.sawtooth(TAU * 260 * t) * (np.sin(TAU * pulses / (m / SR) * t) > 0)
    x = filt(buzz, "bandpass", [f0 * 0.8, f0 * 1.25]) + 0.5 * filt(buzz, "bandpass", [f0 * 1.7, f0 * 2.2])
    return x * np.sin(np.linspace(0, np.pi, m))


def bullfrog():
    m = int(0.45 * SR)
    t = np.arange(m) / SR
    src = signal.sawtooth(np.cumsum(TAU * 115 * (1 - 0.1 * t / t[-1]) / SR), 0.3) * (0.7 + 0.3 * np.sin(TAU * 22 * t))
    return filt(filt(src, "bandpass", [180, 700]), "lowpass", 900) * np.sin(np.linspace(0, np.pi, m)) ** 0.7


def bird(kind):
    """procedural birdsong: a few FM whistles"""
    out = []
    if kind == "song":                                 # a forest songbird: a phrase of quick notes
        for _ in range(rng.integers(4, 9)):
            d = rng.uniform(0.05, 0.14)
            m = int(d * SR)
            t = np.arange(m) / SR
            f0 = rng.uniform(2200, 4200)
            f = f0 * (1 + rng.uniform(-0.3, 0.3) * t / d) * (1 + 0.04 * np.sin(TAU * rng.uniform(20, 60) * t))
            out += [np.sin(np.cumsum(TAU * f / SR)) * np.sin(np.pi * t / d) ** 1.5, np.zeros(int(rng.uniform(0.02, 0.08) * SR))]
    elif kind == "whistle":                            # a long falling whistle
        m = int(0.6 * SR)
        t = np.arange(m) / SR
        f = 2300 * (1500 / 2300) ** (t / t[-1]) * (1 + 0.025 * np.sin(TAU * 13 * t))
        out = [np.sin(np.cumsum(TAU * f / SR)) * np.sin(np.pi * t / t[-1]) ** 1.5]
    elif kind == "hoot":
        for _ in range(2):
            m = int(0.22 * SR)
            t = np.arange(m) / SR
            out += [np.sin(TAU * 420 * t * (1 - 0.04 * t / t[-1])) * np.sin(np.pi * t / t[-1]) ** 2, np.zeros(int(0.25 * SR))]
    elif kind == "knock":                              # a woodpecker-ish drum on hollow wood
        for i in range(rng.integers(8, 16)):
            m = int(0.02 * SR)
            k = np.zeros(m)
            k[:20] = noise(20)
            out += [filt(k, "bandpass", [700, 1500]) * 4, np.zeros(int(0.045 * SR))]
    elif kind == "wader":                              # a water bird: a nasal two-note honk
        for f0 in (620, 520):
            m = int(0.18 * SR)
            t = np.arange(m) / SR
            src = signal.sawtooth(TAU * f0 * t, 0.4)
            out += [filt(src, "bandpass", [900, 2600]) * np.sin(np.pi * t / t[-1]), np.zeros(int(0.08 * SR))]
    elif kind == "raptor":                             # a soaring cry, high and thin
        m = int(1.1 * SR)
        t = np.arange(m) / SR
        f = 2800 * (1 - 0.35 * (t / t[-1]) ** 2) * (1 + 0.03 * np.sin(TAU * 9 * t))
        x = np.sin(np.cumsum(TAU * f / SR)) + 0.3 * filt(noise(m), "bandpass", [2500, 4000])
        out = [x * np.sin(np.pi * t / t[-1]) ** 0.8]
    x = np.concatenate(out)
    return x / (np.abs(x).max() + 1e-9)


def drip(f0):
    return bubble(f0, 0.06, 2.2)


def lava(n, rate=6):
    """thick bubbling: low bubbles that pop, a hiss over them"""
    x = 0.15 * filt(noise(n), "lowpass", 300)
    for t in times(rate, 0, n / SR):
        b = bubble(rng.uniform(70, 180), rng.uniform(0.12, 0.3), rise=1.4)
        pop = filt(noise(int(0.02 * SR)), "bandpass", [300, 2000]) * 0.6
        i = int(t * SR)
        if i + len(b) + len(pop) < n:
            x[i:i + len(b)] += b * rng.uniform(0.3, 0.8)
            x[i + len(b):i + len(b) + len(pop)] += pop
    return x


def rumble(n, level=1.0):
    return level * filt(noise(n), "lowpass", 60, 4) * slow(n, 0.1, 0.4, 1.0)


def ice_creak():
    m = int(rng.uniform(0.3, 0.8) * SR)
    t = np.arange(m) / SR
    f = rng.uniform(90, 160) * (1 + 0.4 * t / t[-1])
    src = signal.sawtooth(np.cumsum(TAU * f / SR), 0.2) * (np.abs(noise(m)) > 1.2)
    return filt(src, "bandpass", [400, 2500]) * np.sin(np.pi * t / t[-1])


def drone(n, root=38, kind="wind"):
    """the biome's drone (the soundscape's own page: here one of its types, in D)"""
    t = np.arange(n) / SR
    f = 440 * 2 ** ((root - 69) / 12)
    if kind == "pad":
        x = sum(signal.sawtooth(TAU * f * r * (1 + d) * t) for r, d in ((1, 0), (1, 0.004), (1.5, -0.003), (2, 0.002)))
        x = filt(x, "lowpass", 700) * 0.15
    elif kind == "choir":
        x = sum(np.sin(TAU * f * h * t) / h for h in range(1, 9))
        x = filt(x, "bandpass", [200, 900]) * 0.3
    elif kind == "throat":                              # a low creature hum, ROAR-like
        x = signal.sawtooth(TAU * f / 2 * t) + 0.6 * signal.sawtooth(TAU * f * 0.75 * t)
        x = filt(x, "lowpass", 400) * 0.18 * (1 + 0.1 * np.sin(TAU * 0.13 * t))
    else:                                              # a resonant wind tone
        x = filt(noise(n), "bandpass", [f * 1.9, f * 2.1]) * 2.5 + filt(noise(n), "bandpass", [f * 2.9, f * 3.1]) * 1.5
    return x * slow(n, 0.08, 0.6, 1.0)


# ------------------------------------------------------------------ the encounter: a predator passes
def encounter(buf, start, pred="TYRANT", near=0.35, quiet_layers=None):
    """distant steps, the jungle hushes, steps closer, a growl, a roar, steps going away. Returns the hush curve."""
    hush = np.ones(N)
    span = 12.0
    i0, i1 = int(start * SR), int((start + span) * SR)
    k = np.linspace(0, np.pi, i1 - i0)
    hush[i0:i1] = 1 - 0.85 * np.sin(k) ** 0.6
    step_t = start
    gap = 0.95
    for j in range(11):
        d = 1 - near * np.sin(np.pi * j / 10)          # distance: far, closest at the middle, far again
        pan = -0.8 + 1.6 * j / 10
        st = STEP["near"] if d < 0.75 else STEP["far"]
        place(buf, far(st, d * 0.7), step_t * SR, pan, 0.9 * (1.2 - d))
        if j in (2, 7):
            place(buf, far(STEP["snap"], d * 0.6), (step_t + 0.3) * SR, pan, 0.5)
        if j == 5:
            place(buf, far(call("SAURUS", 15, 3), d * 0.4), (step_t + 0.2) * SR, pan, 0.6)     # LOW GROWL
            roar = call("SAURUS", 1 if pred == "TYRANT" else 13, 5)
            place(buf, far(roar, d * 0.3), (step_t + 1.6) * SR, pan, 0.95)
            step_t += 1.5
        step_t += gap * rng.uniform(0.9, 1.1)
    return hush


# ------------------------------------------------------------------ biomes
def biome(name):
    b = np.zeros((N, 2))
    amb = np.zeros((N, 2))                             # the layers the encounter hushes
    t = np.arange(N) / SR
    if name == "WATER":                                # a lake shore where a river comes in
        place(b, stream(N, 70, 300, 1300, 0.3), 0, -0.5, 0.35)
        place(b, waves(N, 7.5), 0, 0.4, 0.3)
        place(b, drone(N, 38, "choir"), 0, 0, 0.6)
        for tt in times(0.25):
            place(amb, frog(rng.uniform(1600, 2300)), tt * SR, rng.uniform(-1, 1), 0.12)
        for tt in times(0.07):
            place(amb, bullfrog(), tt * SR, rng.uniform(-0.6, 0.6), 0.25)
        for tt in times(0.08):
            place(amb, far(bird("wader"), rng.uniform(0.2, 0.7)), tt * SR, rng.uniform(-1, 1), 0.25)
        for tt in times(0.12):
            place(b, far(STEP["splash"], rng.uniform(0.3, 0.8)), tt * SR, rng.uniform(-1, 1), 0.35)
        place(b, far(call("SAURUS", 17, 7), 0.55), 16 * SR, 0.3, 0.8)    # SEA MONSTER surfacing out on the lake
        place(b, far(STEP["ripple"], 0.3), 15.6 * SR, 0.3, 0.7)
        out = verb(b + amb, 1.6, 0.3, 3500)
    elif name == "JUNGLE":
        place(amb, cicadas(N, 5200, 40), 0, -0.3, 0.09)
        place(amb, cicadas(N, 6200, 47), 0, 0.4, 0.06)
        place(amb, crickets(N, 4300, 0.5), 0, 0.6, 1.0)
        place(amb, crickets(N, 3700, 0.6), 0, -0.6, 0.7)
        for tt in times(0.4):
            place(amb, frog(rng.uniform(1700, 2200)), tt * SR, rng.uniform(-1, 1), 0.1)
        for tt in times(0.1):
            place(amb, far(bird(rng.choice(["whistle", "hoot", "song"])), rng.uniform(0.3, 0.8)), tt * SR, rng.uniform(-1, 1), 0.12)
        for tt in times(0.3):
            place(amb, drip(rng.uniform(800, 1400)), tt * SR, rng.uniform(-1, 1), 0.08)
        place(b, drone(N, 38, "pad"), 0, 0, 0.7)
        hush = encounter(b, 11.0, "TYRANT", 0.45)
        amb *= hush[:, None]
        out = verb(b + amb, 1.2, 0.28, 3500)
    elif name == "TUNDRA":
        place(b, wind(N, 520, 0.2, howl=0.25), 0, -0.2, 0.5)
        place(b, wind(N, 700, 0.12, howl=0.15), 0, 0.5, 0.35)
        place(b, drone(N, 38, "wind"), 0, 0, 0.5)
        for tt in times(0.12):
            place(b, far(ice_creak(), rng.uniform(0.2, 0.6)), tt * SR, rng.uniform(-1, 1), 0.35)
        st = 6.0                                       # a herd plodding past in the snow, a long call
        for j in range(9):
            place(b, far(STEP["far"], 0.5), (st + j * 1.3) * SR, -0.6 + j * 0.15, 0.6)
            place(b, far(filt(STEP["leaves"], "lowpass", 2500), 0.4), (st + j * 1.3 + 0.05) * SR, -0.6 + j * 0.15, 0.25)
        place(b, far(call("SAURUS", 14, 7), 0.5), 9 * SR, 0.2, 0.7)       # DISTANT HERD
        out = verb(b, 2.4, 0.35, 2500)
    elif name == "FOREST":
        place(amb, wind(N, 400, 0.2) * 0.4 + filt(noise(N), "bandpass", [2000, 7000]) * slow(N, 0.3, 0.05, 0.4), 0, 0.2, 0.4)
        for tt in times(0.25):
            place(amb, far(bird("song"), rng.uniform(0.1, 0.6)), tt * SR, rng.uniform(-1, 1), 0.18)
        for tt in times(0.06):
            place(amb, far(bird("knock"), rng.uniform(0.2, 0.6)), tt * SR, rng.uniform(-1, 1), 0.3)
        place(amb, crickets(N, 3900, 0.8, 0.05), 0, -0.5, 1.0)
        for tt in times(0.1):
            place(b, far(STEP["snap"], rng.uniform(0.3, 0.8)), tt * SR, rng.uniform(-1, 1), 0.4)
        place(b, far(call("SAURUS", 4, 7), 0.45), 7 * SR, -0.5, 0.6)      # CREST HOOT
        place(b, far(call("SAURUS", 18, 7), 0.6), 19 * SR, 0.6, 0.6)      # HADRO HORN answering
        place(b, drone(N, 38, "choir"), 0, 0, 0.6)
        out = verb(b + amb, 1.4, 0.3, 4000)
    elif name == "PLAINS":
        place(b, wind(N, 300, 0.15) * 0.6 + filt(noise(N), "bandpass", [1500, 6000]) * slow(N, 0.4, 0.05, 0.5), 0, 0, 0.45)
        place(amb, cicadas(N, 4400, 30, 0.1), 0, 0.3, 0.05)
        for tt in times(0.05):
            place(b, far(call("SAURUS", 6, 6), rng.uniform(0.5, 0.8)), tt * SR, rng.uniform(-1, 1), 0.5)   # HERD MOAN
        place(b, far(call("SAURUS", 0, 7), 0.6), 4 * SR, -0.4, 0.7)       # BRACHIO CALL
        for j in range(40):                            # a stampede far off, rolling across
            place(b, far(STEP["far"], 0.75), (17 + j * 0.18 + rng.uniform(0, 0.08)) * SR, -0.9 + j * 0.045, 0.45)
        place(b, drone(N, 38, "throat"), 0, 0, 0.6)
        out = verb(b + amb, 1.0, 0.2, 3000)
    elif name == "MOUNTAINS":
        place(b, wind(N, 600, 0.25, howl=0.3), 0, -0.4, 0.55)
        place(b, wind(N, 880, 0.18, howl=0.2), 0, 0.5, 0.35)
        for tt in times(0.08):
            place(b, far(STEP["rock"], rng.uniform(0.3, 0.7)), tt * SR, rng.uniform(-1, 1), 0.5)
        for tt in (5.0, 18.0):
            place(b, far(bird("raptor"), 0.3), tt * SR, rng.uniform(-0.8, 0.8), 0.25)
        place(b, far(call("SAURUS", 5, 5), 0.4), 11 * SR, 0.5, 0.45)      # PTERO SHRIEK
        place(b, drone(N, 38, "wind"), 0, 0, 0.5)
        out = verb(b, 3.0, 0.4, 2500)
    elif name == "CANYONS":
        place(b, wind(N, 740, 0.2, howl=0.35), 0, 0, 0.45)
        for tt in times(0.07):
            place(b, far(STEP["rock"], rng.uniform(0.2, 0.6)), tt * SR, rng.uniform(-1, 1), 0.55)
        place(b, far(call("SAURUS", 2, 3), 0.3), 6 * SR, -0.6, 0.6)       # RAPTOR BARK, echoing off the walls
        place(b, far(call("SAURUS", 2, 3), 0.4), 7.1 * SR, 0.5, 0.4)
        place(b, far(call("SAURUS", 13, 6), 0.5), 17 * SR, 0.2, 0.7)      # MONSTER ROAR down the canyon
        place(b, far(bird("raptor"), 0.4), 24 * SR, -0.5, 0.25)
        place(b, drone(N, 38, "throat"), 0, 0, 0.5)
        out = verb(b, 2.2, 0.35, 2500, echoes=[(0.38, 0.35), (0.71, 0.22), (1.15, 0.14), (1.6, 0.08)])
    elif name == "CALDERA":
        place(b, rumble(N, 3.0), 0, 0, 0.8)
        place(b, lava(N, 5), 0, -0.4, 0.5)
        place(b, lava(N, 3), 0, 0.5, 0.4)
        for tt in times(0.1):                          # steam vents
            m = int(rng.uniform(1.5, 3.5) * SR)
            v = filt(noise(m), "bandpass", [1500, 7000]) * np.sin(np.linspace(0, np.pi, m)) ** 0.5
            place(b, v, tt * SR, rng.uniform(-1, 1), 0.2)
        for tt in times(0.3):                          # crackle
            place(b, filt(noise(80), "highpass", 2000) * 2, tt * SR, rng.uniform(-1, 1), 0.3)
        place(b, far(STEP["thunder"], 0.2), 14 * SR, 0, 0.9)              # a quake
        place(b, far(STEP["rock"], 0.2), 14.4 * SR, 0.4, 0.6)
        place(b, drone(N, 38, "throat"), 0, 0, 0.6)
        out = verb(b, 1.8, 0.3, 2000)
    out = filt(out, "highpass", 25)
    fade = np.minimum(1, np.minimum(t / 1.5, (DUR - t) / 2.0))
    out *= fade[:, None]
    return out / (np.abs(out).max() + 1e-9) * 0.85


def main():
    order = ["WATER", "JUNGLE", "TUNDRA", "FOREST", "PLAINS", "MOUNTAINS", "CANYONS", "CALDERA"]
    sel = sys.argv[1:] or order
    parts = []
    for nm in sel:
        x = biome(nm)
        parts += [x, np.zeros((int(1.5 * SR), 2))]
        p = S / f"biome_{nm}.wav"
        with wave.open(str(p), "wb") as w:
            w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
            w.writeframes((np.clip(x, -1, 1) * 32767).astype("<i2").tobytes())
        print(nm, "done", flush=True)
    y = np.concatenate(parts)
    p = S / "biomes_all.wav"
    with wave.open(str(p), "wb") as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((np.clip(y, -1, 1) * 32767).astype("<i2").tobytes())
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(p), "-b:a", "160k", str(S / "dinobox_biomes.mp3")], check=True)


if __name__ == "__main__":
    main()
