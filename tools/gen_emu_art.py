#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# GHOULBOX: the browser emulator page's pixel art (web/emu/build.sh), in the CRYPT palette (build/gen/ui_palettes.h):
#   torch.png  DINOBOX: the fern (GHOULBOX: the wall torch), the firmware's own (src/ui_graph.c torch_draw): its 4 frames side by side, 12 x 30 each
#   logo.png   "GHOULBOX" and skull.png, the boot screen's art drawn large (tools/gen_ghoulbox_art.py logo, skull: the
#              same word and skull as GB_LOGO, GB_SKULL, at 4x their size: smooth, the page shows them at 2x) in its
#              colours (THEME, TEXT)
#   wall.png   a tile of grey dungeon blocks, 64 x 32 (16 x 8 blocks, every row half a block along), its own seed
# The page scales them by whole numbers (image-rendering: pixelated).
#   gen_emu_art.py build/gen OUTDIR
import random
import re
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_ghoulbox_art import logo, skull  # noqa: E402


def rgb565(v):
    return ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31)


def mix(a, b, pct):                                     # gfx.c ux_mix: a towards b, pct %
    return tuple(x + (y - x) * pct // 100 for x, y in zip(a, b))


def crypt(gen):
    """DINOBOX: the JUNGLE palette's colours (the name kept: make_site.py, the pages' art)"""
    m = re.search(r'\{"JUNGLE",\s*(0x\w+),\s*(0x\w+),\s*(0x\w+),\s*(0x\w+),\s*(0x\w+)\}',
                  (gen / "ui_palettes.h").read_text())
    bg, surf, text, theme, accent = (rgb565(int(v, 16)) for v in m.groups())
    return {"bg": bg, "surf": surf, "text": text, "theme": theme, "accent": accent,
            "stone": mix(surf, bg, 45), "edge": mix(bg, theme, 45), "sel": mix(bg, theme, 80)}


def torch(c):
    """DINOBOX: the fern (src/ui_graph.c torch_draw, torch.png kept as the name) for f = 0..3, side by side"""
    pairs = [(4, 2), (7, 3), (10, 4), (13, 5), (16, 5), (19, 5), (22, 4), (25, 3)]
    sway = [-1, 0, 1, 0]
    im = Image.new("RGBA", (48, 30), (0, 0, 0, 0))

    def put(x, y, col):
        if 0 <= x < 48 and 0 <= y < 30:
            im.putpixel((x, y), col + (255,))

    def dx(sw, row):
        d = sw * (16 - row) if row < 16 else 0
        return int((d + (5 if d > 0 else -5 if d < 0 else 0)) / 10)
    for f in range(4):
        x0, sw = 12 * f, sway[f]
        for j in range(3, 30):
            put(x0 + 6 + dx(sw, j), j, c["edge"])
        for k, (ly, n) in enumerate(pairs):
            col = c["sel"] if k & 1 else c["theme"]
            for i in range(1, n + 1):
                put(x0 + 6 + dx(sw, ly) - i, ly - i // 2, col)
                put(x0 + 6 + dx(sw, ly) + i, ly - i // 2, col)
        put(x0 + 6 + dx(sw, 1), 1, c["theme"])
        put(x0 + 6 + dx(sw, 2), 2, c["theme"])
    return im


def tinted(alpha, col):
    im = Image.new("RGBA", alpha.size, col + (0,))
    im.putalpha(alpha)
    return im


def wall():
    """DINOBOX: rock strata, 64 x 32 and tiling: dark earth bands rising and falling gently, a lighter seam between
    them, some grit and now and then a pebble"""
    rnd = random.Random(1137)
    earth = [(26, 34, 26), (30, 38, 28), (22, 30, 23), (34, 40, 30)]
    seam = (48, 56, 40)
    im = Image.new("RGB", (64, 32), earth[0])
    wave = [0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, -1, -1, -1, -1, 0]
    for x in range(64):
        w = wave[(x // 4) % 16]
        for y in range(32):
            v = (y + w) % 32
            band = v // 8
            col = earth[band]
            if v % 8 == 0:
                col = seam
            im.putpixel((x, y), col)
    for _ in range(60):                                 # grit
        x, y = rnd.randrange(64), rnd.randrange(32)
        im.putpixel((x, y), tuple(max(0, v - 8) for v in im.getpixel((x, y))))
    for _ in range(5):                                  # pebbles
        x, y = rnd.randrange(64), rnd.randrange(32)
        for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
            im.putpixel(((x + dx) % 64, (y + dy) % 32), (52, 58, 44))
    return im


def main():
    gen, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    c = crypt(gen)
    torch(c).save(out / "torch.png")
    tinted(logo(896, 208), c["theme"]).save(out / "logo.png")      # (GB_LOGO is logo(224, 52))
    tinted(skull(440, 240), c["text"]).save(out / "skull.png")      # (GB_SKULL is skull(110, 60))
    wall().save(out / "wall.png")


if __name__ == "__main__":
    main()
