#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DINOBOX: the SCAPE previews (the soundscape maker, firmware/src/scape.c) as one MP3 with a cue list.
#   python3 tools/scape_preview.py   -> build/scape/dinobox_scape_preview.mp3, build/scape/cues.txt
# Each section is rendered by build/host/scape_preview (tests/scape_preview.c): the firmware's own scape code through
# its whole mix (reverb, MASTER at power-on, limiter), at the levels the device will play them. Nothing normalised:
# what you hear is how loud it will be.
import subprocess
import wave
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build/scape"
OUT.mkdir(parents=True, exist_ok=True)
BIN = ROOT / "build/host/scape_preview"
SR = 44100
BIOMES = ["WATER", "JUNGLE", "TUNDRA", "FOREST", "PLAINS", "MOUNTAINS", "CANYONS", "CALDERA"]
TIMES = ["DAWN", "DAY", "DUSK", "NIGHT"]
WEATHER = ["CLEAR", "WIND", "RAIN", "STORM"]

# (title, seconds, biome, time, weather, danger, music, seed, first encounter at, env)
SECTIONS = [
    ("MUSIC alone (the JUNGLE power-on scene): set your volume here", 12, 1, 3, 0, 0, 1, 1, 0, {"SCAPE_LEVEL": "0"}),
    ("... the same music with the JUNGLE scape under it (SCAPE at its default)", 30, 1, 3, 0, 40, 1, 1, 14, {}),
    ("WATER at dawn: a stream into a lake, frogs, a bullfrog, waders, something big out on the water", 32, 0, 0, 0, 0, 0, 1, 0, {}),
    ("JUNGLE at night, DANGER high: crickets, frogs, owls ... then something comes through", 40, 1, 3, 0, 110, 0, 1, 6, {}),
    ("TUNDRA by day: wind, ice creaking, a herd far off", 30, 2, 1, 0, 0, 0, 1, 0, {}),
    ("FOREST at dawn: songbirds, a woodpecker, crested hoots", 30, 3, 0, 0, 0, 0, 1, 0, {}),
    ("FOREST at night: the same forest, an owl and crickets", 24, 3, 3, 0, 0, 0, 1, 0, {}),
    ("PLAINS by day: grass wind, cicadas, a herd passing, the herds' calls", 30, 4, 1, 0, 0, 0, 1, 0, {}),
    ("PLAINS in a STORM: rain, thunder, the wind up", 26, 4, 1, 3, 0, 0, 1, 0, {}),
    ("MOUNTAINS by day: wind in the peaks, a rock fall, a hawk, pterosaurs", 30, 5, 1, 0, 0, 0, 1, 0, {}),
    ("CANYONS at dusk: wind down the walls, rocks, raptors", 30, 6, 2, 0, 0, 0, 1, 0, {}),
    ("CALDERA at night: rumble, lava, steam vents", 30, 7, 3, 0, 0, 0, 1, 0, {}),
]


def read(p):
    w = wave.open(str(p))
    return np.frombuffer(w.readframes(w.getnframes()), np.int16).reshape(-1, 2).astype(np.float64)


def main():
    import os
    parts, cues, at = [], [], 0.0
    for i, (title, secs, b, t, w, d, m, seed, enc, env) in enumerate(SECTIONS):
        wav = OUT / f"sec{i:02d}.wav"
        args = [str(BIN), str(wav), str(secs), str(b), str(t), str(w), str(d), str(m), str(seed)]
        if enc:
            args.append(str(enc))
        e = dict(os.environ, **env)
        r = subprocess.run(args, env=e, capture_output=True, text=True, check=True)
        x = read(wav)
        n = len(x)
        fade = np.minimum(1.0, np.minimum(np.arange(n) / (1.0 * SR), (n - np.arange(n)) / (1.5 * SR)))
        x *= fade[:, None]
        mm, ss = divmod(int(at), 60)
        label = f"{BIOMES[b]} {TIMES[t]} {WEATHER[w]} DANGER {d}"
        cues.append(f"{mm}:{ss:02d}  {title}   [{label}]   {r.stdout.strip()}")
        print(cues[-1], flush=True)
        parts += [x, np.zeros((int(1.5 * SR), 2))]
        at += n / SR + 1.5
    y = np.concatenate(parts)
    p = OUT / "scape_all.wav"
    with wave.open(str(p), "wb") as f:
        f.setnchannels(2); f.setsampwidth(2); f.setframerate(SR)
        f.writeframes(np.clip(y, -32768, 32767).astype("<i2").tobytes())
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(p), "-b:a", "192k",
                    str(OUT / "dinobox_scape_preview.mp3")], check=True)
    (OUT / "cues.txt").write_text("\n".join(cues) + "\n")


if __name__ == "__main__":
    main()
