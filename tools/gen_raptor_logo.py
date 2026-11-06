#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The RAPTOR logo: a snarling raptor head in profile (traced from the owner's photo), bone with a lava eye, three
claw slashes in lava behind its neck, over a slanted, cut-stroke wordmark. Writes the boot splash in the format splash.c reads (the names of SLOOP's: SLOOP_SPLASH_*):

  tools/gen_raptor_logo.py OUT.h            firmware boot splash (RLE, 16-colour palette, RGB565)
  tools/gen_raptor_logo.py --png OUT.png    a preview (2x)
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

BLACK = (0, 0, 0)
BONE = (236, 224, 196)
LAVA = (255, 92, 20)
MOSS = (120, 168, 64)
SPLASH_W, SPLASH_H = 240, 188
SS = 4                                            # supersampling

# the head, traced from the owner's photo of a snarling raptor (facing left, mouth open): photo pixels, mapped by PH()
def PH(pts):
    return [((x - 150) * 0.31 + 8, (y - 172) * 0.31 + 1) for x, y in pts]

HEAD = PH([(152, 452), (158, 410), (196, 386), (244, 352), (300, 300), (352, 254), (404, 222), (446, 200), (502, 182),
           (566, 172), (630, 186), (700, 210), (736, 228), (756, 300), (744, 400), (706, 496), (640, 488), (560, 476),
           (498, 476), (440, 506), (370, 540), (300, 570), (232, 596), (198, 594), (186, 570), (196, 546), (176, 500),
           (160, 484)])
UPPER = [(166, 482), (232, 470), (302, 450), (380, 420), (450, 396), (502, 384)]    # the upper jaw's edge, front to back
LOWER = [(204, 548), (244, 532), (302, 510), (362, 486), (432, 450), (502, 402)]    # the lower jaw's
MOUTH = PH(UPPER + LOWER[::-1])
EYE_C, EYE_R = PH([(404, 324)])[0], 30 * 0.31
NOSTRIL = PH([(176, 412), (198, 406), (196, 422), (180, 426)])
BROW = PH([(296, 300), (446, 254)])
STRIPES = [PH(q) for q in ([(560, 180), (584, 176), (596, 300), (578, 310)], [(632, 188), (656, 196), (664, 330), (644, 336)],
                           [(694, 208), (716, 216), (720, 360), (700, 364)], [(500, 186), (520, 182), (540, 280), (524, 286)])]
SLASHES = [((196, 30), (226, 116)), ((210, 24), (240, 110)), ((224, 18), (254, 104))]

# the wordmark: strokes on a 4 x 6 grid a letter
LETTERS = {
    "R": [[(0, 6), (0, 0), (3, 0), (4, 1), (4, 2), (3, 3), (0, 3)], [(2, 3), (4, 6)]],
    "A": [[(0, 6), (2, 0), (4, 6)], [(1, 4), (3, 4)]],
    "P": [[(0, 6), (0, 0), (3, 0), (4, 1), (4, 2), (3, 3), (0, 3)]],
    "T": [[(0, 0), (4, 0)], [(2, 0), (2, 6)]],
    "O": [[(1, 0), (3, 0), (4, 1), (4, 5), (3, 6), (1, 6), (0, 5), (0, 1), (1, 0)]],
}
WORD = "RAPTOR"
W_X0, W_Y0, W_CELL, W_GAP, W_STROKE, W_SLANT = 24, 140, 6.0, 9.0, 6.0, 0.30


def render(scale=1):
    s = SS * scale
    im = Image.new("RGB", (SPLASH_W * s, SPLASH_H * s), BLACK)
    d = ImageDraw.Draw(im)
    P = lambda pts: [(x * s, y * s) for x, y in pts]
    for (a, b) in SLASHES:                                       # behind the head
        d.line(P([a, b]), fill=LAVA, width=int(7 * s))
    d.polygon(P(HEAD), fill=BONE)
    for q in STRIPES:                                            # the neck's dark stripes
        d.polygon(P(q), fill=BLACK)
    d.polygon(P(MOUTH), fill=tuple(round(v / 3) for v in LAVA))   # the mouth, dark red
    d.line(P(PH([(502, 398), (556, 444), (612, 474)])), fill=BLACK, width=int(2 * s))   # the jaw's hinge
    for jaw, dirn in ((UPPER, 1), (LOWER, -1)):                  # the teeth: along each jaw, pointing into the mouth
        for (x0, y0), (x1, y1) in zip(jaw, jaw[1:]):
            n = 3 if x1 - x0 > 60 else 2
            for k in range(n):
                t0, t1 = k / n, (k + 0.6) / n
                ax, ay = x0 + (x1 - x0) * t0, y0 + (y1 - y0) * t0
                bx, by = x0 + (x1 - x0) * t1, y0 + (y1 - y0) * t1
                L = 20 if x0 < 420 else 12
                tip = ((ax + bx) / 2, (ay + by) / 2 + dirn * L)
                d.polygon(P(PH([(ax, ay), (bx, by), tip])), fill=BONE)
    ex, ey = EYE_C
    d.ellipse(P([(ex - EYE_R, ey - EYE_R), (ex + EYE_R, ey + EYE_R)]), fill=LAVA)
    d.polygon(P([(ex - 1.2, ey - EYE_R + 1.5), (ex + 1.2, ey - EYE_R + 1.5), (ex + 1, ey + EYE_R - 1.5), (ex - 1, ey + EYE_R - 1.5)]), fill=BLACK)
    d.line(P(BROW), fill=BLACK, width=int(3 * s))                # the brow over the eye
    d.polygon(P(NOSTRIL), fill=BLACK)
    x = W_X0
    for ch in WORD:
        for stroke in LETTERS[ch]:
            pts = [(x + gx * W_CELL + (6 - gy) * W_CELL * W_SLANT, W_Y0 + gy * W_CELL) for gx, gy in stroke]
            d.line(P(pts), fill=BONE, width=int(W_STROKE * s), joint="curve")
        x += 4 * W_CELL + W_GAP
    d.line(P([(W_X0, W_Y0 + 44), (x - W_GAP + 8, W_Y0 + 44)]), fill=LAVA, width=int(3 * s))   # the underline
    return im.resize((SPLASH_W * scale, SPLASH_H * scale), Image.LANCZOS)


def rgb565(c):
    r, g, b = c
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def splash_header(path):
    im = render()
    cols = [BLACK]                                   # black, and each colour at 1/3, 2/3 and full (the edges)
    for c in (BONE, LAVA, MOSS):
        cols += [tuple(round(v * f) for v in c) for f in (1 / 3, 2 / 3, 1.0)]
    cols += [BLACK] * (16 - len(cols))
    src = im.tobytes()
    px, cache = [], {}
    for i in range(0, len(src), 3):
        p = (src[i], src[i + 1], src[i + 2])
        if p not in cache:
            cache[p] = min(range(10), key=lambda k: sum((p[j] - cols[k][j]) ** 2 for j in range(3)))
        px.append(cache[p])
    rle = bytearray()                                 # (run - 1) << 4 | index, runs of 1..16
    i = 0
    while i < len(px):
        j = i
        while j < len(px) and px[j] == px[i] and j - i < 16:
            j += 1
        rle.append(((j - i - 1) << 4) | px[i])
        i = j
    L = ["/* generated by tools/gen_raptor_logo.py: the RAPTOR boot splash (splash.c reads SLOOP's names) */",
         "#pragma once", "#include <stdint.h>",
         f"#define SLOOP_SPLASH_W {SPLASH_W}", f"#define SLOOP_SPLASH_H {SPLASH_H}",
         "static const uint16_t SLOOP_SPLASH_PAL[16] = {" + ", ".join(f"0x{rgb565(c):04X}" for c in cols) + "};",
         f"static const uint8_t SLOOP_SPLASH_RLE[{len(rle)}] = {{"]
    for k in range(0, len(rle), 24):
        L.append("    " + ", ".join(str(b) for b in rle[k:k + 24]) + ",")
    L.append("};")
    Path(path).write_text("\n".join(L) + "\n")
    print(f"logo: RAPTOR splash {SPLASH_W}x{SPLASH_H}, {len(rle)} B RLE -> {path}")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--png":
        render(2).save(sys.argv[2])
    elif len(sys.argv) == 2:
        splash_header(sys.argv[1])
    else:
        sys.exit(__doc__)
