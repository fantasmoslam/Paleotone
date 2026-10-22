#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# GHOULBOX: the dungeon presets, one table -> the engines' preset arrays (firmware/src/eng_*.c) and the web
# editor's mirror of them (web/editor.html). Appended after each engine's own presets (a project stores a
# preset's index: never reorder, only append), between GHOULBOX markers; run again after an edit, the block
# is replaced.
#   python3 tools/ghoulbox_presets.py
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# engine -> (its .c, its PRESETS array, the editor's engine name)
ENGINES = {
    "ANALOG": ("eng_analog.c", "ANALOG_PRESETS"),
    "VOICE": ("eng_formant.c", "FORMANT_PRESETS"),
    "WHEEL": ("eng_wheel.c", "WHEEL_PRESETS"),
    "PHYS": ("eng_phys.c", "PHYS_PRESETS"),
    "TRIO": ("eng_trio.c", "TRIO_PRESETS"),
    "LOFI": ("eng_lofi.c", "LOFI_PRESETS"),
    "PHASE": ("eng_phase.c", "PHASE_PRESETS"),
    "NOISE": ("eng_noise.c", "NOISE_PRESETS"),
    "ROAR": ("eng_gurdy.c", "GURDY_PRESETS"),          # DINOBOX: GHOULBOX's GURDY engine
    "SAURUS": ("eng_saurus.c", "SAURUS_PRESETS"),      # DINOBOX: the creature voice
}

# DINOBOX: epic prehistoric sounds in GHOULBOX's slots (same order, same engines: saved projects keep
# pointing at a sound of the same kind). name, EDIT 1..8, {A D S R}, filter env, mono, FX(dist, chorus, delay, reverb), suggested pattern
# (the reverb sends are high: GHOULBOX's HALL; TAPE / CRSH on the master do the rest of the age)
PRESETS = {
    "ANALOG": [  # WAVE (SAW SQR TRI SIN PWM SYNC SUB) DTN MIX NOIS | CUT RES DRV KTR
        ("JUNGLE PAD", [0, 20, 64, 8, 52, 12, 4, 44], [80, 90, 120, 100], 10, 0, (0, 85, 15, 100), 5),     # lush, wide
        ("EPIC STRINGS", [4, 12, 50, 0, 64, 6, 0, 52], [60, 90, 120, 95], 6, 0, (0, 100, 10, 95), 5),    # a big section
        ("CANYON HORN", [0, 8, 64, 0, 34, 20, 28, 64], [35, 80, 110, 70], 55, 0, (12, 25, 15, 90), 4),  # brassy swell
        ("STOMP BASS", [6, 0, 0, 0, 30, 14, 45, 32], [4, 85, 105, 55], 30, 1, (18, 0, 0, 35), 8),       # a sub with bite
    ],
    "VOICE": [  # VOWL (A 0, E 32, I 64, O 95, U 127) VOWL2 TALK SHIFT | BUZZ BRTH Q RAND
        ("TRIBE CHANT", [95, 127, 40, -7, 85, 22, 75, 10], [60, 90, 120, 100], 0, 0, (0, 50, 0, 105), 5),
        ("EPIC CHOIR", [0, 32, 90, 0, 60, 20, 60, 12], [90, 90, 120, 110], 0, 0, (0, 90, 15, 115), 5),   # open "ah"
        ("RAPTOR CALL", [40, 80, 25, 12, 110, 90, 95, 50], [2, 70, 90, 45], 0, 1, (15, 20, 30, 95), 4),  # a hissing screech
        ("REX ROAR", [127, 0, 45, -12, 127, 75, 70, 25], [18, 90, 115, 85], 0, 1, (45, 10, 0, 105), 17),  # "oo" opening to "ah", low
    ],
    "WHEEL": [  # REG (FLUTE MELLO HOLLW SMOOT 3BAR BLUES GOSPL ROCK TOPS CLARI REED STRNG CHAPL BRITE BASS FULL)
                # SUB BODY TOP | PERC CLICK DRV ROTR (OFF SLOW FAST)
        ("AMBER ORGAN", [15, 3, 1, 0, 0, 5, 20, 1], [20, 64, 127, 70], 0, 0, (0, 10, 0, 100), 5),
        ("CANOPY", [11, 2, 0, 2, 0, 0, 10, 2], [30, 64, 127, 90], 0, 0, (0, 20, 0, 110), 5),       # shimmering strings
        ("SWAMP REED", [10, 0, 2, -4, 0, 10, 40, 1], [15, 64, 127, 50], 0, 0, (5, 20, 0, 85), 6),
        ("MOSS FLUTE", [0, 0, -2, -6, 3, 20, 25, 1], [25, 64, 127, 60], 0, 1, (0, 30, 20, 95), 4),
    ],
    "PHYS": [  # MODAL: MODEL STRC BRIT DAMP POS ACC -; STRNG: MODEL STRC BRIT DAMP POS ACC BOW EXC; SYMP: .. SYMP ACC BUZZ EXC
        ("LOG MARIMBA", [0, 116, 30, 52, 30, 100, 0, 40], [0, 95, 127, 60], 0, 0, (0, 15, 20, 80), 3),   # hollow wood
        ("GEODE BELLS", [0, 70, 105, 108, 14, 100, 0, 8], [0, 100, 127, 90], 0, 0, (0, 20, 15, 100), 3),
        ("VALLEY HARP", [3, 4, 70, 85, 70, 127, 0, 0], [0, 100, 127, 95], 0, 0, (0, 25, 20, 105), 3),
        ("TARPIT DRONE", [3, 1, 45, 100, 80, 127, 120, 0], [30, 100, 127, 90], 0, 0, (8, 10, 0, 95), 5),
    ],
    "TRIO": [  # WAVE INT2 INT3 DTN | MODE CUT RES PW
        ("PANGAEA PAD", [0, 7, -12, 18, 0, 62, 15, 64], [100, 90, 120, 110], 12, 0, (0, 100, 25, 115), 5),
        ("BRONTO HORN", [1, -12, -24, 6, 0, 34, 25, 50], [45, 85, 110, 75], 40, 0, (10, 20, 10, 90), 4),   # vast and low
        ("SKY STRINGS", [0, 12, 0, 22, 0, 80, 5, 64], [60, 90, 120, 95], 0, 0, (0, 110, 0, 100), 5),
        ("PTERO CRY", [1, 12, 19, 8, 1, 80, 70, 30], [3, 70, 110, 40], 20, 1, (25, 0, 25, 85), 19),     # a band-pass shriek
    ],
    "LOFI": [  # CHIP (4BIT 4B/2 8BIT 1BIT STEP) WAVE (PLS TRI SAW NOIS WRAM) DUTY CRSH | SWP VIB ARP TONE
        ("FOSSIL LEAD", [0, 1, 50, 10, 0, 40, 0, 80], [5, 70, 110, 60], 0, 1, (0, 20, 35, 90), 4),
        ("DINO CHOIR", [0, 4, 92, 0, 0, 25, 0, 90], [60, 90, 115, 90], 0, 0, (0, 70, 20, 100), 5),
        ("BONE WHISTLE", [2, 1, 64, 0, 10, 30, 0, 50], [12, 70, 115, 50], 0, 1, (0, 20, 25, 90), 4),
    ],
    "PHASE": [  # WAVE WAVE2 DCW ENV | DTN LINE SUB -
        ("TITAN BRASS", [0, 0, 55, 85, 10, 0, 45, 0], [40, 80, 105, 70], 0, 0, (10, 25, 15, 95), 4),
    ],
    "NOISE": [  # MODE (ANLG DUST LFSR META) COLR FREQ RES | TRK DENS DRFT CRSH
        ("VOLCANO", [0, 110, 30, 25, 0, 30, 100, 0], [90, 90, 127, 120], 0, 0, (20, 0, 10, 110), 5),     # a deep rumble, falling rocks
        ("LAVA CRACKLE", [1, 45, 80, 55, 0, 80, 40, 0], [20, 90, 127, 80], 0, 0, (0, 0, 0, 80), 5),
    ],
    "ROAR": [  # DRN (C .. B) DLVL 5TH BRTH | RASP PULS (HOLD 1/4 1/8 1/8T 1/16) BODY WOBL
        ("PRIMEVAL", [2, 110, 70, 70, 60, 2, 95, 50], [15, 64, 127, 50], 0, 1, (12, 15, 0, 100), 15),
        ("EARTHQUAKE", [2, 127, 127, 40, 30, 0, 127, 90], [60, 64, 127, 110], 0, 1, (25, 15, 0, 115), 17),   # a wall of drone
        ("STAMPEDE", [2, 60, 30, 110, 127, 4, 60, 15], [2, 64, 127, 25], 0, 1, (25, 0, 0, 75), 21),          # 1/16 thunder
        ("REX GROWL", [2, 80, 20, 90, 127, 0, 90, 60], [6, 64, 127, 45], 0, 1, (35, 0, 0, 90), 18),          # a constant snarl
        ("SWAMP FIDDLE", [2, 20, 0, 70, 10, 0, 110, 60], [40, 64, 120, 75], 0, 1, (5, 25, 10, 105), 19),
        ("TRIBE PIPES", [2, 127, 100, 120, 20, 0, 40, 10], [6, 64, 127, 35], 0, 1, (22, 0, 0, 85), 15),
    ],
    "SAURUS": [  # CALL (HOLD BELLOW ROAR BARK HOOT TRILL CHIRP MOAN HISS SNORT) SIZE SWOOP LEN | GROWL BRTH MOUTH WILD
        # LEN: 1 ms (0) .. 10 s (127), exponential: 82 ~0.4 s, 96 ~1 s, 104 ~1.9 s, 110 ~3 s
        ("BRACHIO CALL", [1, 120, 42, 108, 20, 35, 90, 25], [40, 90, 127, 90], 0, 0, (0, 20, 15, 110), 17),   # a long-neck's
        ("TYRANT ROAR", [2, 105, 58, 102, 100, 45, 127, 30], [4, 90, 127, 80], 0, 1, (30, 0, 10, 100), 17),   # bellow
        ("RAPTOR BARK", [3, 35, 55, 82, 80, 60, 120, 50], [0, 70, 127, 40], 0, 0, (10, 0, 25, 85), 4),
        ("RAPTOR CHAT", [5, 30, 20, 96, 50, 50, 90, 70], [2, 80, 110, 40], 0, 0, (5, 0, 20, 85), 4),
        ("CREST HOOT", [4, 85, 10, 104, 10, 25, 40, 15], [50, 90, 127, 100], 0, 0, (0, 30, 20, 115), 17),     # a crested
        ("PTERO SHRIEK", [6, 15, 70, 92, 70, 70, 127, 60], [0, 80, 110, 50], 0, 0, (15, 0, 30, 95), 4),     # duckbill
        ("HERD MOAN", [7, 95, 45, 110, 35, 40, 80, 40], [40, 90, 120, 100], 0, 0, (0, 40, 10, 110), 5),
        ("FRILL HISS", [8, 40, 0, 96, 30, 90, 110, 30], [6, 80, 110, 60], 0, 0, (0, 0, 10, 80), 17),
        ("SAURO SNORT", [9, 120, 0, 88, 40, 110, 70, 30], [0, 80, 127, 50], 0, 0, (0, 0, 0, 90), 4),
        ("DEEP RUMBLE", [0, 127, 0, 0, 60, 30, 40, 20], [80, 90, 127, 110], 0, 0, (10, 30, 0, 110), 17),      # a drone
        ("TITAN APE", [2, 70, 30, 98, 100, 60, 110, 30], [6, 90, 127, 80], 0, 1, (25, 0, 10, 95), 17),
        ("NEST CHIRPS", [6, 5, 50, 84, 20, 40, 110, 90], [0, 70, 110, 40], 0, 0, (0, 20, 30, 90), 4),
        ("SWAMP CROAK", [5, 75, 8, 100, 60, 30, 20, 40], [10, 80, 120, 60], 0, 0, (0, 0, 10, 90), 8),
        ("MONSTER ROAR", [2, 90, 66, 100, 127, 70, 127, 45], [2, 90, 127, 70], 0, 1, (60, 0, 20, 90), 17),   # B-movie
        ("DISTANT HERD", [1, 110, 36, 112, 15, 30, 70, 50], [90, 90, 120, 120], 0, 0, (0, 40, 25, 127), 5),
        ("LOW GROWL", [0, 95, 0, 0, 127, 80, 50, 40], [30, 90, 127, 80], 0, 1, (20, 0, 0, 90), 17),
    ],
}

BEGIN, END = "GHOULBOX presets (tools/ghoulbox_presets.py)", "GHOULBOX end"


def c_line(p):
    name, e, env, fenv, mono, fx, pat = p
    return (f'    {{"{name}", {{{", ".join(map(str, e))}}}, {{{", ".join(map(str, env))}}}, {fenv}, {mono}, '
            f'FX({", ".join(map(str, fx))}), PAT({pat})}},')


def js_item(p):
    name, e, env, _, mono, _, pat = p
    return f'P("{name}", [{", ".join(map(str, e))}], [{", ".join(map(str, env))}], {mono}, {pat})'


# engines.c PATTERNS[]: how many (a preset's suggested pattern is 1..NPATTERNS), counted so this never goes stale
NPATTERNS = len(re.findall(r'^    \{"[A-Z0-9]+", \{', (ROOT / "firmware/src/engines.c").read_text().split("} PATTERNS[] = {", 1)[1]
                           .split("};", 1)[0], re.M))


def check(eng, p):
    name, e, env, fenv, mono, fx, pat = p
    assert len(name) <= 12 and name == name.upper(), (eng, name)
    assert len(e) == 8 and len(env) == 4 and len(fx) == 4, (eng, name)
    assert all(0 <= v <= 127 for v in env + list(fx)) and mono in (0, 1) and 1 <= pat <= NPATTERNS, (eng, name)


def strip_block(s, open_c, close_c):
    return re.sub(rf"\s*{re.escape(open_c)} {re.escape(BEGIN)} {re.escape(close_c)}.*?{re.escape(open_c)} "
                  rf"{re.escape(END)} {re.escape(close_c)}", "", s, flags=re.S)


def write_c(eng, presets):
    fname, arr = ENGINES[eng]
    path = ROOT / "firmware/src" / fname
    s = strip_block(path.read_text(), "/*", "*/")
    i = s.index(f"static const preset_t {arr}[] = {{")
    j = s.index("\n};", i)                              # the array's end
    block = f"\n    /* {BEGIN} */\n" + "\n".join(c_line(p) for p in presets) + f"\n    /* {END} */"
    path.write_text(s[:j] + block + s[j:])


def write_js(s, eng, presets):
    s = s
    i = s.index(f'{{ name: "{eng}",')
    k = s.index("presets: [", i) + len("presets: [")
    depth, j = 1, k                                     # the array's matching ]
    while depth:
        depth += {"[": 1, "]": -1}.get(s[j], 0)
        j += 1
    j -= 1
    inner = strip_block(s[k:j], "/*", "*/").rstrip().rstrip(",")   # (the separator before an old block too)
    sep = "," if inner.strip() else ""
    block = f"{sep}\n        /* {BEGIN} */ " + ", ".join(js_item(p) for p in presets) + f" /* {END} */"
    return s[:k] + inner + block + s[j:]


def main():
    html = ROOT / "web/editor.html"
    s = html.read_text()
    for eng, presets in PRESETS.items():
        for p in presets:
            check(eng, p)
        write_c(eng, presets)
        s = write_js(s, eng, presets)
    html.write_text(s)
    print(f"ghoulbox presets: {sum(len(v) for v in PRESETS.values())} in {len(PRESETS)} engines")


if __name__ == "__main__":
    sys.exit(main())
