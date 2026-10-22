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
    """DINOBOX: the TERMINAL palette's colours (the name kept: make_site.py, the pages' art)"""
    m = re.search(r'\{"TERMINAL",\s*(0x\w+),\s*(0x\w+),\s*(0x\w+),\s*(0x\w+),\s*(0x\w+)\}',
                  (gen / "ui_palettes.h").read_text())
    bg, surf, text, theme, accent = (rgb565(int(v, 16)) for v in m.groups())
    return {"bg": bg, "surf": surf, "text": text, "theme": theme, "accent": accent,
            "stone": mix(surf, bg, 45), "edge": mix(bg, theme, 45), "sel": mix(bg, theme, 80)}


def torch(c):
    """DINOBOX: the electric fence post (src/ui_graph.c torch_draw; torch.png kept as the name), f = 0..3 side by side"""
    ins = [11, 18, 25]
    dim = mix(c["bg"], c["text"], 42)
    im = Image.new("RGBA", (48, 30), (0, 0, 0, 0))

    def rect(x, y, w, h, col):
        for j in range(h):
            for i in range(w):
                im.putpixel((x + i, y + j), col + (255,))
    for f in range(4):
        x0, lit = 12 * f, f < 2
        rect(x0 + 4, 0, 4, 4, c["accent"] if lit else dim)
        if lit:
            rect(x0 + 5, 1, 2, 2, c["text"])
        rect(x0 + 3, 4, 6, 2, c["theme"])
        for j in range(6, 30):
            for i in range(4, 8):
                rect(x0 + i, j, 1, 1, c["accent"] if ((i + j) // 3) & 1 else c["edge"])
        for y in ins:
            rect(x0 + 1, y, 3, 2, c["theme"]); rect(x0 + 8, y, 3, 2, c["theme"])
            rect(x0, y + 2, 1, 1, dim); rect(x0 + 11, y + 2, 1, 1, dim)
        y = ins[f % 3]
        rect(x0 + (9 if f & 1 else 1), y - 1, 1, 1, c["text"])
        rect(x0 + (10 if f & 1 else 2), y - 2, 1, 1, c["text"])
    return im


def tinted(alpha, col):
    im = Image.new("RGBA", alpha.size, col + (0,))
    im.putalpha(alpha)
    return im


def wall():
    """DINOBOX: a terminal's glass, 64 x 32 and tiling: near-black with a faint scanline every third row and a faint
    grid every 32 px, as an old green screen"""
    bg, line, grid = (5, 11, 7), (9, 20, 12), (11, 30, 16)
    im = Image.new("RGB", (64, 32), bg)
    for y in range(32):
        for x in range(64):
            col = grid if (x % 32 == 0 or y == 0) else line if y % 3 == 2 else bg
            im.putpixel((x, y), col)
    return im


def main():
    gen, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    c = crypt(gen)
    torch(c).save(out / "torch.png")
    tinted(logo(896, 208, 4), c["theme"]).save(out / "logo.png")      # (GB_LOGO is logo(224, 52))
    tinted(skull(440, 240), c["text"]).save(out / "skull.png")      # (GB_SKULL is skull(110, 60))
    wall().save(out / "wall.png")


if __name__ == "__main__":
    main()
