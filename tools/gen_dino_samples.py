#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DINOBOX: its own sample material, synthesized (no recordings: everything here is made by this script, CC0 like
# the rest of assets/samples-cc0). Writes the four sets that took GHOULBOX's recordings' places (SAMPLE sets 5..8,
# tools/gen_samples.py CC0_SETS / DINO_SETS), each WAV at the rate and level it is kept at:
#   JUNGLE  a night jungle, a seamless 3 s loop: insects, crickets, tree frogs, a bullfrog, a far bird, drips
#   STEPS   the encounter kit, one sound per key from F3: near and far giant footfalls, a mud stomp, a claw tap,
#           a branch snap, a falling tree, a splash, a rock fall, leaves, bone clacks, a ground ripple, thunder,
#           a huff
#   STONE   a lithophone (tuned stone bars), two zones
#   STORM   a tropical downpour with a low rumble, a seamless 2.6 s loop
# Deterministic (fixed seeds): run it again and the files are the same. Needs numpy and scipy (not the build: the
# WAVs are committed).
#   python3 tools/gen_dino_samples.py [OUTDIR]       (default assets/samples-cc0)
import sys
import wave
from pathlib import Path

import numpy as np
from scipy import signal

ROOT = Path(__file__).resolve().parent.parent
TAU = 2 * np.pi


# ---------------------------------------------------------------- tools

def rng(seed):
    return np.random.default_rng(seed)


def bp(x, sr, lo, hi, order=2):
    return signal.sosfilt(signal.butter(order, [lo, hi], "bandpass", fs=sr, output="sos"), x)


def lp(x, sr, f, order=2):
    return signal.sosfilt(signal.butter(order, f, "lowpass", fs=sr, output="sos"), x)


def hp(x, sr, f, order=2):
    return signal.sosfilt(signal.butter(order, f, "highpass", fs=sr, output="sos"), x)


def reso(x, sr, f, q):
    """a two-pole resonator (a ringing mode) at f Hz"""
    b, a = signal.iirpeak(f, q, fs=sr)
    return signal.lfilter(b, a, x)


def circ_noise(n, sr, shape, r):
    """noise of length n that loops seamlessly: white noise shaped in the frequency domain by shape(f)"""
    spec = np.fft.rfft(r.standard_normal(n))
    f = np.fft.rfftfreq(n, 1 / sr)
    spec *= shape(np.maximum(f, 1.0))
    y = np.fft.irfft(spec, n)
    return y / (np.std(y) + 1e-12)


def tilt(f, lo, hi, slope=1.0):
    """a band from lo to hi Hz with gentle skirts, falling slope dB/oct-ish (pink for 1)"""
    g = (f / lo) ** 2 / (1 + (f / lo) ** 2) / (1 + (f / hi) ** 4)
    return g / f ** (slope * 0.5) * np.sqrt(lo)


def env_ad(n, sr, a, d, curve=1.0):
    t = np.arange(n) / sr
    e = np.where(t < a, t / max(a, 1e-6), np.exp(-(t - a) / max(d, 1e-6) * curve))
    return e


def ir(sr, t60, r, damp=4000.0, pre=0.012, dens=1.0):
    """a reverb impulse response: decaying noise, darker as it goes"""
    n = int(sr * t60 * 1.1)
    t = np.arange(n) / sr
    x = r.standard_normal(n) * np.exp(-6.9 * t / t60)
    lo = lp(x, sr, min(damp, sr * 0.45))
    k = np.clip(t / t60, 0, 1)
    y = x * (1 - k) + lo * k
    y = np.concatenate([np.zeros(int(pre * sr)), y])
    return y / np.sqrt(np.sum(y ** 2))


def verb(x, sr, t60, wet, r, damp=4000.0, pre=0.012):
    w = signal.fftconvolve(x, ir(sr, t60, r, damp, pre))
    out = np.zeros(len(w))
    out[:len(x)] += x * (1 - wet)
    out += w * wet
    return out


def fold(x, n):
    """a render longer than the loop wrapped onto the loop: what runs past the end sounds at the start"""
    out = np.zeros(n)
    for k in range(0, len(x), n):
        seg = x[k:k + n]
        out[:len(seg)] += seg
    return out


def place(buf, ev, at):
    """mix ev into buf (a circle: the end wraps to the start)"""
    n = len(buf)
    idx = (np.arange(len(ev)) + at) % n
    np.add.at(buf, idx, ev)


def fade_out(x, sr, s):
    k = min(len(x), int(s * sr))
    x[-k:] *= 0.5 + 0.5 * np.cos(np.linspace(0, np.pi, k))
    return x


def fade_in(x, sr, s):
    k = int(s * sr)
    x[:k] *= np.linspace(0, 1, k)
    return x


def chirp_sine(n, sr, f0, f1, curve="exp"):
    t = np.linspace(0, 1, n)
    f = f0 * (f1 / f0) ** t if curve == "exp" else f0 + (f1 - f0) * t
    return np.sin(np.cumsum(TAU * f / sr))


def write(path, x, sr, level):
    """x scaled so its peak is `level` (of full scale), 16-bit mono"""
    x = np.asarray(x, float)
    x = x - np.mean(x) if len(x) > 2 * sr else x
    x = x / (np.max(np.abs(x)) + 1e-12) * level
    pcm = np.round(np.clip(x, -1, 1) * 32767).astype("<i2")
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(pcm.tobytes())
    print(f"dino samples: {path.relative_to(path.parents[1])}  {len(x) / sr:.2f} s at {sr} Hz, peak {level:.2f}")


# ---------------------------------------------------------------- JUNGLE: a night jungle loop

def jungle(sr=16000, secs=3.0):
    r = rng(1993)
    n = int(sr * secs)
    t = np.arange(n) / sr
    out = np.zeros(n)
    # the bed: warm air and far leaves, pink below 1.4 kHz
    out += 0.10 * circ_noise(n, sr, lambda f: tilt(f, 60, 1400, 1.0), r)
    # cicadas: a buzzing band, pulsed at ~40 Hz, swelling (whole cycles per loop: seamless)
    for fc, bw, rate, cyc, ph, g in ((5100, 900, 40.0, 1, 0.0, 0.09), (6200, 700, 47.0, 2, 1.7, 0.05)):
        b = circ_noise(n, sr, lambda f, fc=fc, bw=bw: np.exp(-0.5 * ((f - fc) / bw) ** 2), r)
        rate = round(rate * secs) / secs
        pulse = (0.5 + 0.5 * np.sin(TAU * rate * t)) ** 3
        swell = 0.55 + 0.45 * np.sin(TAU * cyc * t / secs + ph)
        out += g * b * pulse * swell
    # crickets: three-pulse chirps of a pure tone, two of them, each on its own beat (whole beats per loop)
    for f0, per, off, g in ((4300.0, secs / 6, 0.07, 0.11), (3700.0, secs / 5, 0.29, 0.07)):
        pl = int(0.016 * sr)
        pe = np.sin(np.linspace(0, np.pi, pl)) ** 2
        tone = np.sin(TAU * f0 * np.arange(pl) / sr) * pe
        k = 0
        while k * per < secs:
            for p in range(3):
                place(out, g * tone * (1 - 0.15 * p), int((off + k * per + p * 0.032) * sr))
            k += 1
    # tree frogs: a rasping "kreck": a few fast pulses of a buzzy tone through a nasal band
    for at, f0 in ((0.40, 1900), (0.47, 1900), (1.62, 2150), (2.38, 1800), (2.45, 1800)):
        m = int(0.07 * sr)
        tt = np.arange(m) / sr
        buzz = signal.sawtooth(TAU * 260 * tt) * (0.5 + 0.5 * np.sign(np.sin(TAU * 55 * tt)))
        ev = reso(buzz, sr, f0, 6) + 0.5 * reso(buzz, sr, f0 * 1.9, 8)
        ev *= np.sin(np.linspace(0, np.pi, m)) * 0.10
        place(out, ev, int(at * sr))
    # a bullfrog, low and far: "brr-ummm"
    m = int(0.42 * sr)
    tt = np.arange(m) / sr
    f = 118 * (1 - 0.12 * tt / tt[-1])
    src = signal.sawtooth(np.cumsum(TAU * f / sr), 0.3) * (0.7 + 0.3 * np.sin(TAU * 22 * tt))
    ev = reso(src, sr, 260, 4) + 0.6 * reso(src, sr, 520, 5)
    ev = lp(ev, sr, 900) * np.sin(np.linspace(0, np.pi, m)) ** 0.7 * 0.18
    place(out, ev, int(1.05 * sr))
    # a far bird: a long falling whistle with a warble, answered by two soft hoots
    m = int(0.62 * sr)
    tt = np.arange(m) / sr
    fq = 2300 * (1500 / 2300) ** (tt / tt[-1]) * (1 + 0.025 * np.sin(TAU * 13 * tt))
    ev = np.sin(np.cumsum(TAU * fq / sr)) * np.sin(np.linspace(0, np.pi, m)) ** 1.5 * 0.07
    place(out, verb(ev, sr, 1.6, 0.6, r), int(0.15 * sr))
    for at in (1.95, 2.22):
        m = int(0.2 * sr)
        tt = np.arange(m) / sr
        ev = np.sin(TAU * 430 * tt * (1 - 0.04 * tt / tt[-1])) * np.sin(np.linspace(0, np.pi, m)) ** 2 * 0.07
        place(out, verb(ev, sr, 1.8, 0.65, r, 2500), int(at * sr))
    # drips: a drop's rising "bloop"
    for at, f0 in ((0.85, 900), (2.0, 1250), (2.72, 1050)):
        m = int(0.06 * sr)
        ev = chirp_sine(m, sr, f0, f0 * 2.2) * np.exp(-np.arange(m) / (0.012 * sr)) * 0.10
        place(out, verb(ev, sr, 0.9, 0.35, r), int(at * sr))
    # the whole night through a small damp space, the tails wrapped round
    out = fold(verb(out, sr, 1.1, 0.25, r, 3500), n)
    return out


# ---------------------------------------------------------------- STORM: a tropical downpour loop

def storm(sr=11025, secs=2.6):
    r = rng(1990)
    n = int(sr * secs)
    t = np.arange(n) / sr
    rain = circ_noise(n, sr, lambda f: tilt(f, 300, 4200, 0.6), r)
    out = 0.55 * rain
    # drops on leaves: many short bright ticks of noise
    for _ in range(int(220 * secs)):
        m = int(r.uniform(0.004, 0.012) * sr)
        ev = r.standard_normal(m) * np.exp(-np.arange(m) / (m / 4)) * r.uniform(0.15, 0.7)
        place(out, hp(ev, sr, 1500), r.integers(n))
    # heavier drops on broad leaves: a little pitched "plap"
    for _ in range(int(14 * secs)):
        m = int(0.05 * sr)
        f0 = r.uniform(380, 900)
        ev = np.sin(TAU * f0 * np.arange(m) / sr * (1 + 0.6 * np.arange(m) / m)) * np.exp(-np.arange(m) / (0.01 * sr))
        place(out, ev * r.uniform(0.3, 0.6), r.integers(n))
    # the low rumble under it: distant thunder that never quite stops (one slow swell per loop)
    rum = circ_noise(n, sr, lambda f: 1 / (1 + (f / 70) ** 4) * (f > 18), r)
    out += 0.9 * rum * (0.65 + 0.35 * np.cos(TAU * t / secs))
    return fold(verb(out, sr, 0.8, 0.2, r, 2500), n)


# ---------------------------------------------------------------- STONE: a lithophone

def stone_note(sr, f0, secs, r):
    n = int(sr * secs)
    t = np.arange(n) / sr
    x = np.zeros(n)
    # a stone bar's modes: inharmonic, the higher ones short; slightly detuned pairs (an uneven slab) beat
    for ratio, amp, dec in ((1.0, 1.0, 0.55), (2.72, 0.42, 0.22), (5.18, 0.22, 0.09), (8.4, 0.12, 0.05), (12.1, 0.06, 0.03)):
        f = f0 * ratio
        if f > sr * 0.45:
            continue
        for det, w in ((1.0, 1.0), (1.004, 0.35)):
            x += amp * w * np.sin(TAU * f * det * t + r.uniform(0, TAU)) * np.exp(-t / dec)
    # the strike: a hard mallet click, a short gritty burst through the upper modes
    m = int(0.006 * sr)
    click = r.standard_normal(m) * np.exp(-np.arange(m) / (m / 5))
    click = bp(np.concatenate([click, np.zeros(n - m)]), sr, min(f0 * 4, 6000), min(f0 * 12, sr * 0.45))
    x += 0.9 * click
    x *= np.minimum(1, t / 0.0015)
    return fade_out(x, sr, secs * 0.35)


# ---------------------------------------------------------------- STEPS: the encounter kit

def thump(sr, n, f0, f1, dec, r, punch=0.02):
    """a heavy body hitting the ground: a sine falling from f0 to f1, a soft attack, a long decay"""
    t = np.arange(n) / sr
    f = f1 + (f0 - f1) * np.exp(-t / 0.08)
    x = np.sin(np.cumsum(TAU * f / sr)) * np.exp(-t / dec) * np.minimum(1, t / punch)
    return x


def debris(sr, n, start, span, count, r, lo=900, hi=5000, g=0.25):
    """grit and pebbles settling: sparse little clicks, thinning out"""
    x = np.zeros(n)
    for _ in range(count):
        at = int((start + span * r.random() ** 2) * sr)
        m = int(r.uniform(0.003, 0.01) * sr)
        if at + m >= n:
            continue
        x[at:at + m] += r.standard_normal(m) * np.exp(-np.arange(m) / (m / 4)) * r.uniform(0.2, 1.0)
    return bp(x, sr, lo, min(hi, sr * 0.45)) * g


def step_near(sr=11025):
    r = rng(1)
    n = int(0.9 * sr)
    t = np.arange(n) / sr
    x = thump(sr, n, 62, 30, 0.28, r, 0.012) * 1.0
    x += 0.5 * thump(sr, n, 120, 70, 0.07, r, 0.004)                           # the foot's own hit
    crunch = lp(r.standard_normal(n), sr, 1400) * np.exp(-t / 0.035)
    x += 0.35 * crunch
    x += 0.5 * lp(r.standard_normal(n), sr, 90) * np.exp(-t / 0.3)              # the ground shaking
    x += debris(sr, n, 0.06, 0.55, 30, r, 700, 4200, 0.1)
    return fade_out(x, sr, 0.25)


def step_far(sr=11025):
    r = rng(2)
    n = int(0.8 * sr)
    t = np.arange(n) / sr
    x = thump(sr, n, 48, 28, 0.3, r, 0.03)
    x += 0.4 * lp(r.standard_normal(n), sr, 70) * np.exp(-t / 0.25)
    x = lp(x, sr, 260)
    x = verb(x, sr, 1.4, 0.45, r, 600, 0.04)[:n]
    return fade_out(x, sr, 0.3)


def mud_stomp(sr=11025):
    r = rng(3)
    n = int(0.42 * sr)
    t = np.arange(n) / sr
    x = 0.8 * thump(sr, n, 110, 55, 0.09, r, 0.006)
    # the squelch: wet noise through a band that opens as the foot sinks, then a suck as it lifts
    nz = r.standard_normal(n)
    sq = np.zeros(n)
    fc = 350 + 700 * (t / t[-1])
    for k in range(0, n, 64):
        seg = nz[k:k + 64]
        sq[k:k + 64] = reso(seg, sr, fc[k], 5)[:len(seg)]
    sq *= np.exp(-t / 0.12) * (1 + 0.6 * np.sin(TAU * 31 * t))
    x += 0.5 * sq / (np.max(np.abs(sq)) + 1e-9)
    return fade_out(x, sr, 0.12)


def claw_tap(sr=22050):
    r = rng(4)
    n = int(0.12 * sr)
    t = np.arange(n) / sr
    hit = np.zeros(n)
    hit[:int(0.0015 * sr)] = r.standard_normal(int(0.0015 * sr))
    x = 0.8 * reso(hit, sr, 3150, 40) + 0.6 * reso(hit, sr, 5400, 50) + 0.4 * reso(hit, sr, 1750, 18)
    x += 0.5 * hp(hit, sr, 2500)
    x *= np.exp(-t / 0.03)
    return fade_out(x, sr, 0.04)


def branch_snap(sr=22050):
    r = rng(5)
    n = int(0.3 * sr)
    t = np.arange(n) / sr
    imp = np.zeros(n)
    for at, a in ((0.0, 1.0), (0.004, 0.7), (0.011, 0.5), (0.019, 0.35), (0.05, 0.2), (0.075, 0.12)):
        k = int(at * sr)
        m = int(0.0012 * sr)
        imp[k:k + m] += r.standard_normal(m) * a
    x = 0.6 * imp
    for f, q, g in ((620, 9, 0.9), (1380, 12, 0.7), (2900, 15, 0.5), (4700, 20, 0.3)):
        x += g * reso(imp, sr, f, q)
    x += debris(sr, n, 0.02, 0.15, 20, r, 1800, 7000, 0.15)                       # splinters
    x *= np.exp(-t / 0.08)
    return fade_out(x, sr, 0.08)


def tree_fall(sr=11025):
    r = rng(6)
    n = int(1.0 * sr)
    t = np.arange(n) / sr
    x = np.zeros(n)
    # the creak: the trunk tearing, a rough low friction tone rising
    m = int(0.45 * sr)
    tt = np.arange(m) / sr
    fq = 70 + 60 * (tt / tt[-1]) ** 2 + 8 * r.standard_normal(m).cumsum() / np.sqrt(m)
    cr = signal.sawtooth(np.cumsum(TAU * fq / sr), 0.2) * (0.6 + 0.4 * np.abs(r.standard_normal(m)))
    cr = reso(cr, sr, 420, 3) + reso(cr, sr, 1100, 5) * 0.5
    x[:m] += cr / np.max(np.abs(cr)) * np.minimum(1, tt / 0.15) * 0.35
    # the crash: the crown hitting the ground, branches breaking all over, and the thud
    k = int(0.42 * sr)
    rest = n - k
    tr = np.arange(rest) / sr
    x[k:] += 0.6 * lp(r.standard_normal(rest), sr, 2600) * np.exp(-tr / 0.18)
    x[k:] += 0.9 * thump(sr, rest, 75, 35, 0.22, r, 0.01)
    x[k:] += debris(sr, rest, 0.0, 0.45, 70, r, 1200, 5000, 0.35)
    return fade_out(x, sr, 0.3)


def splash(sr=16000):
    r = rng(7)
    n = int(0.5 * sr)
    t = np.arange(n) / sr
    x = bp(r.standard_normal(n), sr, 500, 6500) * np.exp(-t / 0.07) * np.minimum(1, t / 0.004)
    x += 0.5 * thump(sr, n, 140, 80, 0.05, r, 0.003)
    for _ in range(14):                                                          # bubbles and falling drops
        at = int((0.03 + 0.4 * r.random() ** 1.5) * sr)
        m = int(r.uniform(0.02, 0.05) * sr)
        if at + m >= n:
            continue
        f0 = r.uniform(500, 1400)
        x[at:at + m] += chirp_sine(m, sr, f0, f0 * 1.8) * np.exp(-np.arange(m) / (m / 4)) * r.uniform(0.1, 0.35)
    return fade_out(x, sr, 0.15)


def rock_fall(sr=16000):
    r = rng(8)
    n = int(0.6 * sr)
    x = np.zeros(n)
    at, gap, a = 0.0, 0.11, 1.0
    while at < 0.45 and gap > 0.006:                                                           # a stone bouncing, then smaller ones
        k = int(at * sr)
        m = int(0.002 * sr)
        imp = np.zeros(n - k)
        imp[:m] = r.standard_normal(m)
        ring = reso(imp, sr, r.uniform(1100, 1700), 14) + 0.6 * reso(imp, sr, r.uniform(2400, 3600), 18) + 0.3 * imp
        x[k:] += a * ring * np.exp(-np.arange(n - k) / (0.03 * sr))
        at += gap * r.uniform(0.7, 1.1)
        gap *= 0.72
        a *= 0.8
    x += debris(sr, n, 0.02, 0.45, 50, r, 1500, 6500, 0.25)
    return fade_out(x, sr, 0.15)


def leaves(sr=16000):
    r = rng(9)
    n = int(0.4 * sr)
    t = np.arange(n) / sr
    nz = bp(r.standard_normal(n), sr, 1800, 7000)
    grains = np.zeros(n)
    for _ in range(60):
        k = r.integers(0, n - 200)
        m = int(r.uniform(0.004, 0.02) * sr)
        grains[k:k + m] += np.sin(np.linspace(0, np.pi, len(grains[k:k + m]))) * r.uniform(0.3, 1)
    x = nz * (0.25 + grains) * np.sin(np.pi * t / t[-1]) ** 0.6
    return fade_out(x, sr, 0.1)


def bone_clack(sr=22050):
    r = rng(10)
    n = int(0.2 * sr)
    t = np.arange(n) / sr
    x = np.zeros(n)
    for at, f1, f2, a in ((0.0, 930, 2350, 1.0), (0.085, 1010, 2500, 0.75)):
        k = int(at * sr)
        imp = np.zeros(n - k)
        imp[:int(0.001 * sr)] = 1.0
        ring = reso(imp, sr, f1, 40) + 0.5 * reso(imp, sr, f2, 40) + 0.2 * reso(imp, sr, f1 * 3.9, 25)
        x[k:] += a * ring * np.exp(-np.arange(n - k) / (0.025 * sr))
    return fade_out(x, sr, 0.05)


def ripple(sr=11025):
    """a far, heavy impact felt through the ground: a deep thump and a low ring (a cup of water would tremble)"""
    r = rng(11)
    n = int(0.5 * sr)
    t = np.arange(n) / sr
    x = thump(sr, n, 45, 32, 0.18, r, 0.02)
    x += 0.25 * np.sin(TAU * 196 * t) * np.exp(-t / 0.2) * np.minimum(1, t / 0.03)
    x += 0.12 * np.sin(TAU * 393 * t) * np.exp(-t / 0.12) * np.minimum(1, t / 0.03)
    return fade_out(x, sr, 0.15)


def thunder(sr=11025):
    r = rng(12)
    n = int(1.25 * sr)
    t = np.arange(n) / sr
    crack = hp(r.standard_normal(n), sr, 600) * np.exp(-t / 0.04) * 0.7
    roll = lp(r.standard_normal(n), sr, 180, 3)
    amp = np.exp(-t / 0.5) * np.minimum(1, t / 0.05)
    amp *= 0.7 + 0.3 * lp(np.abs(r.standard_normal(n)), sr, 6) / 0.8                # rolling lumps
    x = crack + 2.5 * roll * amp
    x = verb(x, sr, 1.5, 0.3, r, 900, 0.03)[:n]
    return fade_out(x, sr, 0.4)


def huff(sr=11025):
    """a big animal's snorting exhale through its nostrils"""
    r = rng(13)
    n = int(0.5 * sr)
    t = np.arange(n) / sr
    nz = r.standard_normal(n)
    x = 0.8 * reso(nz, sr, 330, 3) + 0.6 * reso(nz, sr, 780, 4) + 0.35 * reso(nz, sr, 1900, 5)
    x = x / np.max(np.abs(x))
    flutter = 1 + 0.5 * np.sin(TAU * 28 * t) * np.exp(-t / 0.1)                     # the nostrils flapping
    x *= flutter * np.where(t < 0.03, t / 0.03, np.exp(-(t - 0.03) / 0.13))
    return fade_out(x, sr, 0.12)


# the kit, by key (F3 up): (file, render, its level: the near footfall the loudest)
STEPS = [("00_GiantStep", step_near, 0.98), ("01_FarStep", step_far, 0.55), ("02_MudStomp", mud_stomp, 0.75),
         ("03_ClawTap", claw_tap, 0.9), ("04_BranchSnap", branch_snap, 0.95), ("05_TreeFall", tree_fall, 0.95),
         ("06_Splash", splash, 0.7), ("07_RockFall", rock_fall, 0.7), ("08_Leaves", leaves, 0.45),
         ("09_BoneClack", bone_clack, 0.85), ("10_Ripple", ripple, 0.8), ("11_Thunder", thunder, 0.95),
         ("12_Huff", huff, 0.7)]


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "assets/samples-cc0"
    write(out / "JUNGLE/00_JungleNight_C3.wav", jungle(), 16000, 0.75)
    write(out / "STORM/00_TropicStorm_C3.wav", storm(), 11025, 0.8)
    for name, f0, root in (("00_Lithophone_C2", 130.81, 48), ("01_Lithophone_C4", 523.25, 72)):
        write(out / f"STONE/{name}.wav", stone_note(22050, f0, 0.7, rng(root)), 22050, 0.9)
    for name, fn, level in STEPS:
        sr = fn.__defaults__[0]
        write(out / f"STEPS/{name}.wav", fn(), sr, level)


if __name__ == "__main__":
    main()
