#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DINOBOX demo track "RIDERS OF THE VALLEY": every sound is a DINOBOX preset rendered by the firmware's own audio code
# (tests/stem.c, dry), arranged and mixed here: an 80s cartoon-hero march (horns, brass stabs, a chip arpeggio, war
# drums with gated reverb, strings, a choir) over ROAR's rhythmic drones, STAMPEDE at the heart of it, switching
# between rhythms that chain (STAMPEDE, WAR PARADE, TRIBAL WAR, HEARTBEAT, CHAOS DRUMS), and the dinosaurs calling.
#   python3 tools/dino_riders.py OUT.wav      (needs build/host/stem: cc ... tests/stem.c; numpy, scipy)
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np
from scipy import signal

ROOT = Path(__file__).resolve().parent.parent
STEM = ROOT / "build/host/stem"
SR = 44100
BPM = 96.0
BEAT = 60.0 / BPM
BAR = 4 * BEAT
NBARS = 48
TAIL = 7.0
N = int((NBARS * BAR + TAIL) * SR)
rng = np.random.default_rng(1984)
TMP = Path(tempfile.mkdtemp(prefix="riders"))

E = {"ANALOG": 0, "PHASE": 2, "LOFI": 3, "SAMPLE": 4, "VOICE": 5, "TRIO": 6, "DRUM": 10, "NOISE": 11, "FM6": 12,
     "ROAR": 14, "SAURUS": 15}


# ------------------------------------------------------------------ rendering a part through the firmware
_n = 0


def render(eng, preset, notes, secs=None, edits=(), params=()):
    """notes: [(beat, note, len_beats, vel)] -> mono float array, the dry part"""
    global _n
    _n += 1
    secs = secs or max(b + l for b, _, l, _ in notes) * BEAT + 6.0
    score = [f"{E[eng]} {preset}|{BPM}|{secs}"]
    score += [f"e {k} {v}" for k, v in edits] + [f"p {k} {v}" for k, v in params]
    score += [f"n {b:.4f} {nt} {l:.4f} {v}" for b, nt, l, v in notes]
    out = TMP / f"s{_n}.wav"
    subprocess.run([str(STEM), str(out)], input="\n".join(score) + "\n", text=True, check=True)
    w = wave.open(str(out))
    x = np.frombuffer(w.readframes(w.getnframes()), np.int16).reshape(-1, 2).astype(float).mean(1) / 32768
    return x


def at_bar(bar, beat=0.0):
    return int(((bar * 4 + beat) * BEAT) * SR)


# ------------------------------------------------------------------ effects
def sos(kind, f, order=2):
    return signal.butter(order, f, kind, fs=SR, output="sos")


def filt(x, kind, f, order=2):
    return signal.sosfilt(sos(kind, f, order), x, axis=0)


def stereo(x, pan=0.0, width=0.0):
    """mono -> stereo, constant-power pan, a Haas widening (ms)"""
    l, r = np.cos((pan + 1) * np.pi / 4), np.sin((pan + 1) * np.pi / 4)
    y = np.stack([x * l, x * r], 1) * np.sqrt(2)
    if width:
        d = int(width / 1000 * SR)
        y[:, 1] = np.concatenate([np.zeros(d), y[:-d, 1]]) if d else y[:, 1]
    return y


def ir(t60, damp=5000, pre=0.02, seed=0):
    r = np.random.default_rng(seed)
    n = int(SR * t60 * 1.2)
    t = np.arange(n) / SR
    out = np.zeros((n + int(pre * SR), 2))
    for c in range(2):
        e = r.standard_normal(n) * np.exp(-6.9 * t / t60)
        lo = filt(e, "lowpass", damp)
        k = np.clip(t / t60, 0, 1)
        e = e * (1 - k) + lo * k                       # darker as it fades
        e[: int(0.004 * SR)] *= np.linspace(0, 1, int(0.004 * SR))
        out[int(pre * SR):, c] = e / np.sqrt((e ** 2).sum())
    return out


IRS = {"hall": ir(3.6, 5000, 0.035, 1), "valley": ir(6.0, 3500, 0.06, 2), "plate": ir(1.8, 8000, 0.01, 3),
       "room": ir(0.9, 6000, 0.008, 4)}


def reverb(y, kind, wet):
    h = IRS[kind]
    w = np.stack([signal.fftconvolve(y[:, c], h[:, c])[: len(y)] for c in range(2)], 1)
    return w * wet


def delay(y, beats, fb, wet, lp=4500, pingpong=True):
    d = int(beats * BEAT * SR)
    out = np.zeros_like(y)
    tap = y.copy()
    g = 1.0
    for k in range(1, 9):
        g *= fb
        if g < 0.02:
            break
        tap = filt(tap, "lowpass", lp, 1)
        sh = np.zeros_like(y)
        sh[d * k:] = tap[: len(y) - d * k]
        if pingpong and k % 2:
            sh = sh[:, ::-1]
        out += sh * g
    return out * wet


def chorus(y, depth_ms=6, rate=0.35, mix=0.5):
    n = len(y)
    t = np.arange(n) / SR
    out = y.copy()
    for c, ph in ((0, 0), (1, np.pi / 2)):
        dl = (depth_ms + depth_ms * 0.6 * np.sin(2 * np.pi * rate * t + ph)) / 1000 * SR
        idx = np.clip(np.arange(n) - dl, 0, n - 1)
        out[:, c] = y[:, c] * (1 - mix) + np.interp(idx, np.arange(n), y[:, c]) * mix
    return out


def sweep_lp(y, f0, f1, curve=2.0, block=1024):
    """a low-pass opening from f0 to f1 over the part (exponential)"""
    n = len(y)
    out = np.zeros_like(y)
    zi = None
    for i in range(0, n, block):
        fr = (i / n) ** curve
        f = f0 * (f1 / f0) ** fr
        s = sos("lowpass", min(f, SR * 0.45), 2)
        if zi is None:
            zi = np.zeros((s.shape[0], 2, 2))
        seg, zi_new = signal.sosfilt(s, y[i:i + block], axis=0, zi=zi)
        out[i:i + block] = seg
        zi = zi_new
    return out


def env_follow(x, att=0.005, rel=0.15):
    a, r = np.exp(-1 / (att * SR)), np.exp(-1 / (rel * SR))
    e = np.abs(x)
    out = np.zeros_like(e)
    v = 0.0
    for i in range(0, len(e), 64):                     # (blockwise: fast enough, smooth enough)
        m = e[i:i + 64].max()
        c = a if m > v else r
        v = c ** 64 * v + (1 - c ** 64) * m
        out[i:i + 64] = v
    return out


def duck(y, key, depth=0.5, att=0.003, rel=0.18):
    e = env_follow(key, att, rel)
    e = e / (e.max() + 1e-9)
    return y * (1 - depth * e)[:, None]


def gate_verb(y, hits, t_open=0.28, wet=0.9):
    """80s gated reverb: the plate's tail cut off a moment after each hit"""
    w = reverb(y, "plate", 1.0)
    g = np.zeros(len(y))
    for h in hits:
        i = int(h * SR)
        m = int(t_open * SR)
        g[i:i + m] = 1
        g[i + m:i + m + int(0.02 * SR)] = np.linspace(1, 0, len(g[i + m:i + m + int(0.02 * SR)]))
    return w * g[:, None] * wet


def comp(y, thr_db=-18, ratio=3, att=0.01, rel=0.2, makeup=0):
    e = env_follow(y.mean(1), att, rel)
    db = 20 * np.log10(e + 1e-9)
    over = np.maximum(0, db - thr_db)
    gain = 10 ** ((-over * (1 - 1 / ratio) + makeup) / 20)
    return y * gain[:, None]


def limiter(y, ceil=0.95):
    """a look-ahead peak limiter: the gain the peaks ask for, held over 5 ms, released over ~60 ms"""
    from scipy.ndimage import minimum_filter1d
    pk = np.abs(y).max(1)
    g = np.minimum(1, ceil / (pk + 1e-9))
    g = minimum_filter1d(g, int(0.005 * SR))
    g = np.minimum(g, signal.sosfiltfilt(sos("lowpass", 8, 1), g))
    g = minimum_filter1d(g, int(0.005 * SR))
    return y * g[:, None]


def drive(x, amount, tone=2500, bias=0.08):
    """grit: the mids pushed into an asymmetric soft clip (a hot tape / overdriven desk), the level kept"""
    if amount <= 0:
        return x
    rms = np.sqrt(np.mean(x ** 2)) + 1e-9
    v = x / (rms * 6) * amount
    v = v + 0.35 * filt(v, "bandpass", [tone * 0.5, tone * 2])          # (the mids hit it first)
    y = np.tanh(v + bias) - np.tanh(bias)
    y = filt(y, "lowpass", 11000)
    return y / (np.sqrt(np.mean(y ** 2)) + 1e-9) * rms


def put(bus, x, at):
    m = min(len(x), len(bus) - at)
    if m > 0:
        bus[at:at + m] += x[:m]


# ------------------------------------------------------------------ the music: D minor, 96 BPM, 48 bars
DM, BB, CC, AA, FF, GM = (50, 53, 57), (50, 53, 58), (48, 52, 55), (49, 52, 57), (48, 53, 57), (50, 55, 58)
ROOT_OF = {DM: 38, BB: 34, CC: 36, AA: 33, FF: 41, GM: 43}
THEME = [DM, BB, CC, AA, DM, FF, GM, AA]
RALLY = [DM, DM, BB, CC, DM, DM, BB, AA]
BRIDGE = [BB, CC, DM, DM, BB, CC, AA, AA]
CLIMAX = THEME + [BB, CC, AA, AA]
PROG = [DM] * 8 + RALLY + THEME + BRIDGE + CLIMAX + [DM] * 4          # a chord a bar, 48 bars

MEL = [  # the theme: (bar in the 8, beat, note, beats)
    (0, 0, 74, 1.5), (0, 1.5, 69, .5), (0, 2, 74, 1), (0, 3, 76, 1),
    (1, 0, 77, 2), (1, 2, 76, 1), (1, 3, 74, 1),
    (2, 0, 72, 1.5), (2, 1.5, 69, .5), (2, 2, 72, 1), (2, 3, 74, 1),
    (3, 0, 76, 3), (3, 3, 69, 1),
    (4, 0, 74, 1.5), (4, 1.5, 69, .5), (4, 2, 74, 1), (4, 3, 76, 1),
    (5, 0, 77, 1), (5, 1, 79, 1), (5, 2, 81, 2),
    (6, 0, 82, 1.5), (6, 1.5, 81, .5), (6, 2, 79, 1), (6, 3, 77, 1),
    (7, 0, 76, 2), (7, 2, 73, 1), (7, 3, 74, 1)]
FANFARE = [(0, 0, 69, .5), (0, .5, 69, .5), (0, 1, 74, 1.5), (0, 2.5, 69, .5), (0, 3, 74, 1),
           (1, 0, 77, 2), (1, 2, 76, 1), (1, 3, 74, 1)]
FANFARE2 = [(0, 0, 69, .5), (0, .5, 69, .5), (0, 1, 74, 1.5), (0, 2.5, 69, .5), (0, 3, 74, 1),
            (1, 0, 76, 1), (1, 1, 73, 1), (1, 2, 69, 2)]


def mel_notes(mel, bar0, shift=0, vel=110):
    return [((bar0 + b) * 4 + bt, n + shift, l * 0.95, vel) for b, bt, n, l in mel]


LAYER = {}


def main(out):
    mix = np.zeros((N, 2))
    sends = {k: np.zeros((N, 2)) for k in ("hall", "valley", "plate")}
    dly = np.zeros((N, 2))
    drums_key = np.zeros(N)

    def add(x, at, pan=0.0, gain=1.0, width=0.0, hall=0.0, valley=0.0, plate=0.0, delay_send=0.0, name="?"):
        y = stereo(x, pan, width) * gain
        put(LAYER.setdefault(name, np.zeros((N, 2))), y, at)
        for name, s in (("hall", hall), ("valley", valley), ("plate", plate)):
            if s:
                put(sends[name], y * s, at)
        if delay_send:
            put(dly, y * delay_send, at)
        put(mix, y, at)

    # --- ROAR: the rhythmic drones, one held D a section, the rhythm switching; heavy effects
    roar_plan = [(0, 8, "STAMPEDE"), (8, 8, "WAR PARADE"), (16, 8, "TRIBAL WAR"), (24, 8, "HEARTBEAT"),
                 (32, 12, "STAMPEDE"), (44, 4, "CHAOS DRUMS")]
    for b0, nb, name in roar_plan:
        x = drive(render("ROAR", name, [(0, 50, nb * 4 + 0.5, 110)], secs=nb * BAR + 5), 2.2, 900)
        n = len(x)
        fade = np.ones(n)
        fi = int(0.3 * SR)
        fade[:fi] = np.linspace(0, 1, fi)                         # (each section's own note: a short crossfade)
        end = int(nb * BAR * SR)
        fade[end:] = np.exp(-np.arange(n - end) / (0.5 * SR))
        x = x * fade
        y = stereo(x, 0, 12)
        if b0 == 0:
            y = sweep_lp(y, 180, 9000, 1.6)                       # the intro opens up out of the dark
        g = {0: 0.9, 8: 0.8, 16: 0.72, 24: 0.65, 32: 0.95, 44: 0.8}[b0]   # (STAMPEDE up front in the climax)
        y = chorus(y, 4, 0.2, 0.35) * g
        at = at_bar(b0)
        put(mix, y, at)
        put(LAYER.setdefault("ROAR", np.zeros((N, 2))), y, at)
        put(sends["valley"], y * 0.55, at)
        put(dly, y * 0.35, at)

    # --- bass: STOMP BASS, eighths on the root (rally on), long in the bridge
    bass = []
    for bar in range(8, 44):
        ch = PROG[bar]
        r = ROOT_OF[ch]
        if 24 <= bar < 32:
            bass.append((bar * 4, r, 3.8, 100))
        else:
            for e in range(8):
                bass.append((bar * 4 + e * 0.5, r + (12 if e in (3, 7) else 0), 0.42, 120 if e % 2 == 0 else 95))
    bass += [(44 * 4, 38, 8, 110)]
    x = drive(render("ANALOG", "STOMP BASS", bass), 2.5, 700)
    add(x, 0, 0, 0.65, hall=0.06, name="bass")
    bass_bus_key = x

    # --- drums: WAR DRUMS; the boom and toms dry and big, the skin and clap through a gated plate (80s)
    kick, snare, toms = [], [], []
    for bar in range(8, 44):
        if 24 <= bar < 31:                                         # the bridge: toms only, a heartbeat
            toms += [(bar * 4, 45, .3, 110), (bar * 4 + 2.5, 45, .3, 90)]
            if bar % 2 == 1:
                kick.append((bar * 4, 36, .3, 120))
            continue
        if bar == 31:                                              # a snare roll into the climax
            snare += [(bar * 4 + i * 0.25, 38, .2, 60 + i * 4) for i in range(16)]
            continue
        if bar < 16:                                               # the rally: boom on 1 and 3, toms at the ends
            kick += [(bar * 4, 36, .3, 120), (bar * 4 + 2, 36, .3, 110)]
            if bar % 4 == 3:
                toms += [(bar * 4 + 3, 45, .3, 100), (bar * 4 + 3.5, 45, .3, 110)]
            continue
        kick += [(bar * 4, 36, .3, 127), (bar * 4 + 1.5, 36, .3, 100), (bar * 4 + 2, 36, .3, 120)]
        snare += [(bar * 4 + 1, 38, .3, 120), (bar * 4 + 3, 38, .3, 127), (bar * 4 + 3, 39, .3, 100)]
        if bar % 4 == 3:
            toms += [(bar * 4 + 3, 45, .2, 100), (bar * 4 + 3.25, 45, .2, 105), (bar * 4 + 3.5, 45, .2, 115),
                     (bar * 4 + 3.75, 45, .2, 127)]
    kick += [(44 * 4, 36, .3, 127), (44 * 4, 56, .3, 127)]
    xk = drive(render("DRUM", "WAR DRUMS", kick + toms), 2.0, 300)
    xs = drive(render("DRUM", "WAR DRUMS", snare), 2.8, 2000)
    drums_key = np.pad(xk, (0, max(0, N - len(xk))))[:N]
    add(xk, 0, 0, 1.0, hall=0.12, name="kick/toms")
    ys = stereo(xs, 0, 0) * 1.1
    put(mix, ys, 0)
    put(LAYER.setdefault("snare", np.zeros((N, 2))), ys, 0)
    put(mix, gate_verb(ys, [b * BEAT for b, nt, _, _ in snare if nt == 38], 0.3, 0.8), 0)

    # --- brass stabs: MAMMOTH, root and fifth on 1 and the "and" of 2 (theme and climax)
    stabs = []
    for bar in list(range(16, 24)) + list(range(32, 44)):
        r = ROOT_OF[PROG[bar]] + 12
        for bt, l in ((0, 1.2), (1.5, 0.4), (2, 1.2)):
            stabs += [(bar * 4 + bt, r, l, 120), (bar * 4 + bt, r + 7, l, 110)]
    stabs += [(44 * 4, 50, 8, 120), (44 * 4, 57, 8, 110), (44 * 4, 62, 8, 110)]
    x = drive(render("FM6", "MAMMOTH", stabs), 3.0, 900)
    add(x, 0, -0.15, 1.4, width=8, hall=0.25, name="stabs")

    # --- the horns: CANYON HORN, the fanfare in the rally, the theme, an octave up in the climax; TITAN BRASS
    # doubling the climax below
    lead = mel_notes(FANFARE, 10) + mel_notes(FANFARE2, 14) + mel_notes(MEL, 16) + mel_notes(MEL, 32, 0, 120) \
        + mel_notes(MEL[:13], 40, 0, 120) + [(44 * 4, 74, 7, 120)]
    x = drive(render("TRIO", "LAVA BRASS", lead), 3.5, 1800)              # (sync brass, overdriven)
    add(x, 0, 0.12, 0.62, width=6, hall=0.35, delay_send=0.18, name="horn")
    x = drive(render("PHASE", "TITAN BRASS", mel_notes(MEL, 32, -12, 110) + mel_notes(MEL[:13], 40, -12, 110)), 3.0, 1200)
    add(x, 0, -0.2, 0.45, width=10, hall=0.3, name="brass2")

    # --- the chip arpeggio: EGG ARP (its own arp off), sixteenths over the chord, the theme and the climax
    arp = []
    for bar in list(range(16, 24)) + list(range(32, 44)):
        ch = PROG[bar]
        tones = [ch[0] + 24, ch[1] + 24, ch[2] + 24, ch[1] + 24]
        arp += [(bar * 4 + i * 0.25, tones[i % 4] + (12 if (i // 4) % 2 else 0), 0.2, 90) for i in range(16)]
    x = render("LOFI", "EGG ARP", arp, edits=[(6, 0)])
    add(x, 0, 0.35, 0.3, width=4, plate=0.2, delay_send=0.25, name="arp")

    # --- strings: EPIC STRINGS, from the intro's middle on; the choir: EPIC CHOIR in the bridge and the climax
    pad = [(bar * 4, n, 3.95, 85) for bar in range(4, 44) for n in PROG[bar]] + [(44 * 4, n, 8, 90) for n in (50, 57, 62)]
    x = drive(render("ANALOG", "EPIC STRINGS", pad), 1.5, 1500)
    x[: at_bar(4)] *= 0
    add(x, 0, 0, 0.25, width=15, hall=0.4, name="strings")
    ch = [(bar * 4, n + 12, 3.95, 90) for bar in list(range(24, 32)) + list(range(36, 44)) for n in PROG[bar]]
    ch += [(44 * 4, n + 12, 8, 95) for n in (50, 57, 62)]
    x = drive(render("VOICE", "EPIC CHOIR", ch), 1.3, 1500)
    add(chorus(stereo(x), 7, 0.3, 0.4).mean(1), 0, 0, 0.3, width=18, hall=0.5, name="choir")

    # --- the dinosaurs
    calls = [  # (bar, beat, preset, note, beats, pan, gain, distance 0..1)
        (2, 0, "BRACHIO CALL", 45, 6, -0.5, 0.55, 0.7), (5, 1, "DISTANT HERD", 43, 6, 0.6, 0.5, 0.8),
        (12, 3.5, "RAPTOR BARK", 64, 0.5, 0.7, 0.45, 0.2), (13, 3, "RAPTOR BARK", 62, 0.5, -0.7, 0.45, 0.25),
        (13, 3.5, "RAPTOR BARK", 66, 0.5, -0.4, 0.45, 0.25), (23, 3, "PTERO SHRIEK", 74, 2, 0.8, 0.35, 0.3),
        (24, 0, "TYRANT ROAR", 45, 6, 0.0, 0.85, 0.0), (28, 0, "HADRO HORN", 50, 6, -0.6, 0.45, 0.6),
        (31, 0, "BABY REX", 52, 2, 0.5, 0.4, 0.3), (36, 3.5, "RAPTOR BARK", 64, 0.5, 0.7, 0.4, 0.2),
        (37, 3.5, "RAPTOR BARK", 66, 0.5, -0.7, 0.4, 0.2), (40, 0, "MONSTER ROAR", 43, 6, 0.0, 0.8, 0.0),
        (46, 0, "TYRANT ROAR", 43, 6, 0.0, 0.9, 0.1), (47, 2, "BRACHIO CALL", 45, 6, 0.4, 0.4, 0.8)]
    for bar, bt, name, nt, l, pan, g, d in calls:
        x = drive(render("SAURUS", name, [(0, nt, l, 127)], secs=l * BEAT + 5), 1.6, 1200)
        if d:
            x = filt(x, "lowpass", 9000 * (1 - d) + 900)
        add(x, at_bar(bar, bt), pan, g * (1 - 0.4 * d), width=4, valley=0.25 + 0.5 * d, delay_send=0.12 + 0.2 * d, name="dinos")

    # --- the earth: the ENCOUNTER kit's thunder and giant steps (SAMPLE, one sound a key from F3)
    hits = [(0, 0, 64, 0.5), (6, 2, 64, 0.35), (23, 2, 53, 0.6), (23, 3, 53, 0.7), (44, 0, 64, 0.6), (47, 0, 53, 0.5),
            (47, 2, 54, 0.4)]
    for bar, bt, nt, g in hits:
        x = render("SAMPLE", "ENCOUNTER", [(0, nt, 2, 127)], secs=4)
        add(x, at_bar(bar, bt), 0, g, valley=0.4, name="earth")

    # --- the space: the sends' returns, the drums ducking the long sounds a little
    ret = reverb(sends["hall"], "hall", 1.0) + reverb(sends["valley"], "valley", 1.0) + reverb(sends["plate"], "plate", 1.0)
    ret += delay(dly, 0.75, 0.45, 1.0, 3500)                     # dotted-eighth echoes, ping-pong
    ret = filt(ret, "highpass", 120)
    mix = duck(mix, drums_key, 0.25)
    mix = mix + ret * 0.9
    # --- the master: a gentle glue, tape warmth, a limiter
    mix = filt(mix, "highpass", 28)
    mix = comp(mix, -20, 1.6, 0.02, 0.25, 2)
    for k, v in LAYER.items():                                   # (the balance, by section)
        print(f"{k:12s}" + " ".join(f"{20 * np.log10(np.std(v[at_bar(a):at_bar(b)]) + 1e-9):6.1f}"
                                    for a, b in ((0, 8), (8, 16), (16, 24), (24, 32), (32, 44), (44, 48))))
    mix = drive(mix.mean(1), 0, 0) if False else mix
    mix = np.tanh(mix * 2.0) / 2.0                                   # (hot tape on the bus)
    tt = np.arange(N) / SR                                           # a little wow and flutter
    wob = (0.0009 * np.sin(2 * np.pi * 0.6 * tt) + 0.0003 * np.sin(2 * np.pi * 5.3 * tt)) * SR
    idx = np.clip(np.arange(N) - 40 - wob, 0, N - 1)
    mix = np.stack([np.interp(idx, np.arange(N), mix[:, c]) for c in range(2)], 1)
    hiss = filt(rng.standard_normal((N, 2)), "bandpass", [2000, 9000]) * 0.0025
    mix = mix + hiss
    mix = mix / (np.abs(mix).max() + 1e-9) * 0.98
    mix = limiter(mix * 1.25, 0.93)
    t = np.arange(N) / SR
    mix *= np.minimum(1, (N / SR - t) / 4.0)[:, None]             # (the very end)
    with wave.open(out, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((np.clip(mix, -1, 1) * 32767).astype("<i2").tobytes())
    print(out, f"{N / SR:.1f} s")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "riders.wav")
