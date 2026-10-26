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
    "GRAIN": ("eng_grain.c", "GRAIN_PRESETS"),         # DINOBOX: grains of its own sample sets
    "DRUM": ("eng_drum.c", "DRUM_PRESETS"),            # DINOBOX: the TRIBE kit's sounds (GHOULBOX's two, redone)
    "FM6": ("eng_fm6.c", "FM6_PRESETS"),               # DINOBOX: its patches F9..F15 (tools/gen_fm6_patches.py)
}

# DINOBOX: its own prehistoric sounds, in GHOULBOX's slots first (same order, same engines: saved projects keep
# pointing at a sound of the same kind; every value set anew), then its new ones appended. name, EDIT 1..8, {A D S R}, filter env, mono, FX(dist, chorus, delay, reverb), suggested pattern
# (the reverb sends are high: GHOULBOX's HALL; TAPE / CRSH on the master do the rest of the age)
PRESETS = {
    "ANALOG": [  # WAVE (SAW SQR TRI SIN PWM SYNC SUB) DTN MIX NOIS | CUT RES DRV KTR
        ("JUNGLE PAD", [4, 28, 80, 14, 46, 18, 0, 50], [85, 95, 118, 105], 14, 0, (0, 90, 18, 105), 5),    # PWM, misty
        ("EPIC STRINGS", [0, 34, 96, 0, 58, 10, 6, 70], [70, 90, 122, 100], 4, 0, (0, 95, 12, 100), 5),   # a wide section
        ("CANYON HORN", [5, 14, 40, 0, 40, 34, 20, 80], [40, 75, 112, 72], 48, 0, (10, 20, 18, 92), 4),   # sync brass swell
        ("STOMP BASS", [6, 4, 30, 6, 26, 28, 60, 40], [2, 80, 100, 40], 40, 1, (25, 0, 0, 30), 8),        # sub and bite
        ("AMBER LEAD", [5, 6, 50, 0, 70, 40, 30, 90], [3, 75, 110, 50], 30, 1, (10, 20, 40, 80), 4),
        ("SWAMP BASS", [1, 3, 10, 0, 22, 70, 40, 30], [0, 60, 60, 30], 55, 1, (10, 0, 0, 20), 1),          # a squelch
        ("FERN SWELL", [3, 18, 64, 22, 34, 10, 0, 60], [110, 90, 120, 115], 0, 0, (0, 70, 30, 118), 5),   # breathy sines
        ("STAR SKY", [2, 40, 90, 0, 70, 15, 0, 70], [95, 100, 120, 120], 0, 0, (0, 110, 40, 127), 5),
        ("ICE AGE PAD", [4, 50, 70, 30, 38, 25, 8, 50], [120, 100, 124, 120], -10, 0, (0, 100, 20, 127), 17),
    ],
    "VOICE": [  # VOWL (A 0, E 32, I 64, O 95, U 127) VOWL2 TALK SHIFT | BUZZ BRTH Q RAND
        ("TRIBE CHANT", [110, 60, 30, -12, 95, 30, 85, 15], [50, 90, 120, 95], 0, 0, (0, 40, 0, 100), 16),
        ("EPIC CHOIR", [10, 40, 110, 0, 50, 28, 55, 18], [95, 90, 122, 115], 0, 0, (0, 95, 20, 118), 5),   # "ah" to "e"
        ("RAPTOR CALL", [50, 100, 18, 12, 120, 105, 110, 70], [0, 64, 80, 40], 0, 1, (20, 10, 35, 90), 4),  # a screech
        ("REX ROAR", [120, 10, 60, -12, 127, 90, 60, 40], [20, 95, 110, 80], 0, 1, (55, 0, 0, 100), 17),   # "oo" to "ah"
        ("DEEP CHANT", [127, 95, 70, -12, 70, 15, 95, 8], [60, 90, 122, 100], 0, 0, (0, 30, 0, 110), 16),  # low "oo"
        ("SPIRIT VOX", [64, 0, 90, 0, 40, 60, 50, 20], [80, 90, 118, 110], 0, 0, (0, 80, 40, 120), 5),     # "ee" to "ah"
        ("SHAMAN", [30, 110, 15, -5, 100, 20, 100, 40], [10, 80, 115, 60], 0, 1, (0, 20, 25, 95), 3),     # talking
        ("NIGHT WAIL", [80, 20, 100, 5, 30, 70, 70, 30], [90, 90, 115, 120], 0, 0, (0, 60, 50, 127), 19),
    ],
    "WHEEL": [  # REG (FLUTE MELLO HOLLW SMOOT 3BAR BLUES GOSPL ROCK TOPS CLARI REED STRNG CHAPL BRITE BASS FULL)
                # SUB BODY TOP | PERC (OFF 2ND 3RD 2SOFT 3SOFT 2SLOW 3SLOW) CLICK DRV ROTR (OFF SLOW FAST)
        ("AMBER ORGAN", [6, 4, 2, -2, 0, 12, 35, 1], [18, 64, 127, 72], 0, 0, (5, 15, 0, 95), 14),
        ("CANOPY", [8, -2, 0, 4, 3, 5, 0, 2], [40, 64, 127, 95], 0, 0, (0, 30, 25, 112), 5),           # tops, fast rotor
        ("SWAMP REED", [9, -4, 3, -6, 0, 25, 55, 0], [10, 64, 127, 45], 0, 0, (15, 0, 10, 80), 6),
        ("MOSS FLUTE", [3, -6, -3, -8, 4, 35, 0, 1], [20, 64, 127, 65], 0, 1, (0, 20, 30, 100), 4),
        ("TEMPLE ORGAN", [12, 6, 4, 2, 0, 8, 10, 1], [30, 64, 127, 90], 0, 0, (0, 10, 0, 120), 14),
        ("BONE KEYS", [7, -8, 0, 0, 2, 90, 20, 2], [0, 64, 127, 30], 0, 0, (10, 10, 20, 70), 6),
        ("LAGOON", [1, 2, 0, -4, 5, 20, 0, 1], [10, 64, 127, 80], 0, 0, (0, 40, 40, 105), 6),
    ],
    "PHYS": [  # MODAL: MODEL STRC BRIT DAMP POS ACC -; STRNG: .. BOW EXC; MEMB; SYMP: .. SYMP ACC BUZZ EXC
        ("LOG MARIMBA", [0, 100, 22, 60, 40, 110, 0, 55], [0, 95, 127, 55], 0, 0, (0, 10, 20, 85), 3),    # hollow wood
        ("GEODE BELLS", [0, 50, 120, 115, 8, 100, 0, 20], [0, 100, 127, 95], 0, 0, (0, 25, 20, 105), 7),
        ("VALLEY HARP", [1, 10, 85, 70, 55, 115, 0, 10], [0, 100, 127, 90], 0, 0, (0, 30, 25, 110), 20),
        ("TARPIT DRONE", [3, 6, 30, 110, 95, 120, 110, 30], [30, 100, 127, 95], 0, 0, (8, 10, 0, 100), 17),
        ("HIDE DRUM", [2, 80, 40, 50, 30, 120, 0, 50], [0, 90, 127, 50], 0, 0, (0, 0, 10, 80), 18),
        ("WATER DROPS", [0, 20, 110, 90, 90, 90, 0, 100], [0, 90, 127, 70], 0, 0, (0, 20, 60, 110), 7),
        ("BOWED BONE", [1, 0, 60, 100, 30, 110, 100, 0], [60, 95, 127, 90], 0, 0, (0, 20, 10, 110), 17),
        ("SITAR VINE", [3, 30, 100, 90, 20, 120, 0, 60], [0, 100, 127, 80], 0, 0, (0, 10, 30, 95), 20),
        ("TUSK TINES", [0, 40, 70, 100, 60, 100, 0, 0], [0, 100, 127, 80], 0, 0, (0, 30, 30, 95), 13),
    ],
    "TRIO": [  # WAVE (SAW3 PLS3 PPT SST TRI3 S&T P&S P+N NOIS SYNC SYNCP SYNC3 RING RING3 R+S R+SP) INT2 INT3 DTN
               # | MODE (LP BP HP NOT) CUT RES PW
        ("PANGAEA PAD", [5, 12, -12, 30, 0, 55, 30, 40], [105, 90, 120, 115], 18, 0, (0, 100, 25, 115), 5),
        ("BRONTO HORN", [0, -12, 7, 10, 1, 40, 40, 64], [50, 85, 110, 80], 45, 0, (15, 10, 10, 95), 4),  # low, a fifth
        ("SKY STRINGS", [4, 7, 12, 35, 0, 90, 10, 64], [70, 90, 120, 100], 0, 0, (0, 110, 15, 110), 5),
        ("PTERO CRY", [12, 19, 0, 15, 1, 85, 80, 30], [0, 64, 100, 30], 30, 1, (25, 0, 25, 85), 4),    # ring, a shriek
        ("LAVA BRASS", [9, 0, -12, 8, 0, 60, 50, 64], [20, 80, 110, 60], 50, 1, (20, 10, 15, 90), 4),
        ("SPORE CLOUD", [8, 0, 12, 50, 1, 70, 60, 64], [110, 90, 120, 120], 0, 0, (0, 80, 40, 120), 17),
        ("PRIMAL STAB", [1, 7, 12, 12, 0, 70, 30, 40], [0, 60, 30, 30], 60, 0, (10, 20, 30, 85), 2),
        ("AURORA PAD", [13, 7, -12, 20, 0, 75, 20, 64], [100, 90, 120, 120], 0, 0, (0, 100, 40, 127), 5),
    ],
    "LOFI": [  # CHIP (4BIT 4B/2 8BIT 1BIT STEP) WAVE (PLS TRI SAW NOIS WRAM) DUTY CRSH | SWP VIB ARP TONE
        ("FOSSIL LEAD", [2, 2, 80, 30, 6, 50, 0, 100], [0, 70, 110, 50], 0, 1, (0, 15, 35, 85), 4),
        ("DINO CHOIR", [1, 4, 40, 10, 0, 35, 0, 75], [70, 90, 115, 100], 0, 0, (0, 70, 25, 105), 5),
        ("BONE WHISTLE", [3, 1, 30, 12, 4, 60, 0, 40], [15, 70, 110, 40], 0, 1, (0, 25, 30, 95), 4),
        ("PIXEL RAPTOR", [2, 0, 20, 40, 90, 20, 0, 110], [0, 50, 50, 20], 0, 1, (0, 0, 30, 70), 4),
        ("EGG ARP", [0, 0, 32, 0, 0, 0, 2, 100], [0, 60, 90, 40], 0, 0, (0, 10, 40, 80), 13),
        ("CAVE ECHO", [1, 1, 48, 20, 0, 18, 0, 66], [20, 80, 110, 90], 0, 0, (0, 20, 90, 110), 3),
    ],
    "PHASE": [  # WAVE (SAW SQR PLS DSIN SPLS RSAW RTRI RTRP) WAVE2 (- ..) DCW ENV | DTN LINE SUB -
        ("TITAN BRASS", [5, 0, 80, 100, 20, 1, 70, 0], [45, 80, 108, 70], 0, 0, (12, 20, 15, 95), 4),
        ("ICE CRYSTAL", [3, 7, 90, 30, 8, 0, 0, 0], [0, 90, 127, 80], 0, 0, (0, 30, 40, 110), 7),
        ("RESIN PAD", [6, 1, 50, 60, 30, 1, 40, 0], [90, 90, 120, 110], 0, 0, (0, 80, 20, 115), 5),
        ("DEEP PULSE", [2, 0, 70, 40, 0, 0, 100, 0], [0, 60, 90, 30], 0, 1, (10, 0, 10, 40), 8),
    ],
    "NOISE": [  # MODE (ANLG DUST LFSR META) COLR FREQ RES | TRK DENS DRFT CRSH
        ("VOLCANO", [0, 120, 22, 40, 0, 40, 110, 0], [100, 90, 127, 120], 0, 0, (20, 0, 10, 110), 17),   # a deep rumble
        ("LAVA CRACKLE", [1, 40, 90, 50, 0, 70, 30, 0], [30, 90, 127, 90], 0, 0, (0, 0, 0, 80), 17),
        ("MONSOON", [1, 20, 108, 10, 0, 127, 50, 0], [80, 90, 127, 110], 0, 0, (0, 0, 0, 100), 17),     # rain
        ("GEYSER", [0, 60, 60, 100, 20, 0, 80, 0], [40, 90, 127, 100], 0, 0, (0, 0, 20, 100), 17),      # steam's whistle
        ("SWARM", [3, 40, 110, 90, 0, 60, 100, 30], [40, 90, 127, 80], 0, 0, (0, 30, 20, 90), 17),       # insects
        ("WIND RIDGE", [0, 90, 45, 110, 10, 0, 127, 0], [110, 90, 127, 120], 0, 0, (0, 20, 0, 115), 17),  # a howl
    ],
    "ROAR": [  # DRN (C .. B) DLVL 5TH BRTH | RASP PULS (HOLD 1/4 1/8 1/8T 1/16 TURN DRM4 DRM8 GALOP TRES WAR RAIN QUAKE WAR2 TALK SEVEN PENTA SPIRL
               # CHAOS) BODY WOBL
        ("PRIMEVAL", [2, 100, 90, 60, 75, 0, 110, 70], [25, 64, 127, 60], 0, 1, (12, 15, 0, 100), 15),
        ("EARTHQUAKE", [2, 127, 110, 30, 50, 0, 127, 110], [70, 64, 127, 110], 0, 1, (25, 15, 0, 115), 17),
        ("STAMPEDE", [2, 40, 30, 80, 40, 8, 60, 45], [2, 64, 127, 25], 0, 1, (25, 0, 0, 75), 21),           # GALOP drums
        ("REX GROWL", [2, 60, 0, 100, 127, 3, 100, 90], [8, 64, 127, 45], 0, 1, (40, 0, 0, 90), 18),       # 1/8T snarl
        ("SWAMP FIDDLE", [2, 10, 20, 85, 20, 0, 90, 80], [45, 64, 120, 80], 0, 1, (5, 30, 15, 105), 19),
        ("TRIBE PIPES", [2, 110, 127, 127, 5, 0, 30, 30], [6, 64, 127, 35], 0, 1, (20, 0, 0, 85), 15),
        ("WAR CHANT", [2, 100, 60, 90, 90, 2, 100, 55], [5, 64, 127, 40], 0, 1, (20, 0, 10, 95), 18),
        ("TUNDRA", [2, 120, 100, 40, 30, 0, 127, 127], [90, 64, 127, 120], 0, 1, (0, 40, 10, 120), 17),
        # the jungle rhythms (PULS DRM4 .. QUAKE): a deep drum under the drones; hold a note and it plays on
        ("WAR PARADE", [2, 70, 60, 70, 30, 10, 80, 45], [5, 64, 127, 40], 0, 1, (15, 0, 10, 100), 18),
        ("JUNGLE CAMP", [2, 45, 50, 60, 20, 9, 50, 60], [10, 64, 127, 50], 0, 1, (0, 10, 15, 95), 17),
        ("RITUAL FIRE", [2, 55, 70, 50, 10, 11, 70, 70], [20, 64, 127, 60], 0, 1, (5, 10, 20, 105), 16),
        ("TITAN MARCH", [2, 90, 40, 70, 60, 6, 100, 30], [5, 64, 127, 45], 0, 1, (20, 0, 0, 100), 17),
        ("HEARTBEAT", [2, 30, 20, 40, 0, 12, 40, 60], [40, 64, 127, 90], 0, 1, (0, 20, 0, 115), 17),
        ("HERD RUN", [2, 35, 40, 90, 30, 7, 50, 50], [2, 64, 127, 30], 0, 1, (10, 0, 10, 85), 17),
        # (WOBL: ghost notes and each hit's pitch moving; every fourth bar ends in a fill)
        ("TRIBAL WAR", [2, 75, 60, 60, 20, 13, 90, 40], [5, 64, 127, 45], 0, 1, (20, 0, 10, 105), 17),
        ("TALK DRUMS", [2, 40, 40, 50, 10, 14, 60, 50], [10, 64, 127, 50], 0, 1, (0, 10, 15, 95), 17),
        ("ODD RITUAL", [2, 50, 60, 60, 20, 15, 70, 40], [15, 64, 127, 60], 0, 1, (5, 10, 20, 105), 17),
        ("FIVE MOONS", [2, 40, 70, 50, 10, 16, 60, 60], [30, 64, 127, 80], 0, 1, (0, 30, 25, 115), 17),
        ("SPIRAL", [2, 45, 50, 60, 30, 17, 60, 50], [10, 64, 127, 50], 0, 1, (10, 0, 20, 100), 17),
        ("CHAOS DRUMS", [2, 40, 40, 70, 30, 18, 60, 90], [5, 64, 127, 40], 0, 1, (15, 0, 15, 95), 17),
    ],
    "GRAIN": [  # SRC (PIANO 0, FLUTE 2, SAX 3, JUNGLE 8, STEPS 9, STONE 10, STORM 11) POS SIZE DENS | PTCH SPRD RAND TONE
        ("NIGHT SWARM", [8, 30, 60, 110, 0, 60, 70, 110], [60, 90, 127, 100], 0, 0, (0, 40, 30, 110), 17),
        ("STONE MIST", [10, 10, 100, 90, 12, 40, 30, 90], [80, 90, 127, 110], 0, 0, (0, 60, 30, 120), 5),
        ("STORM CLOUD", [11, 40, 90, 100, -12, 80, 50, 70], [90, 90, 127, 120], 0, 0, (0, 40, 20, 120), 17),
        ("AMBER DUST", [10, 50, 20, 120, 24, 100, 100, 127], [30, 90, 110, 80], 0, 0, (0, 30, 50, 110), 7),
    ],
    "DRUM": [  # KIT (9 TRIBE) TUNE TONE DECY | SNAP ACC KICK (0 PUNCH, 1 ROUND) DRV
        ("TRIBAL KIT", [9, 66, 72, 58, 80, 95, 0, 10], [0, 100, 127, 100], 0, 0, (0, 0, 0, 70), 22),
        ("WAR DRUMS", [9, 36, 40, 90, 60, 120, 1, 35], [0, 100, 127, 100], 0, 0, (20, 0, 0, 100), 22),
        ("BONE RATTLE", [9, 92, 100, 30, 100, 90, 0, 0], [0, 100, 127, 100], 0, 0, (0, 0, 15, 60), 12),
        ("RITUAL", [9, 50, 30, 110, 40, 100, 1, 0], [0, 100, 127, 100], 0, 0, (0, 0, 10, 110), 22),
    ],
    "FM6": [  # ALG FB MLVL MRAT | MEG VMOD DTUN SLOT (F9 8 .. F15 14: tools/gen_fm6_patches.py); the patch's own envelope
        ("TAR PIT BASS", [0, 0, 0, 0, 0, 0, 0, 8], [0, 0, 127, 0], 0, 1, (10, 0, 0, 40), 8),
        ("SLIT DRUM", [0, 0, 0, 0, 0, 0, 0, 9], [0, 0, 127, 0], 0, 0, (0, 0, 20, 70), 20),
        ("MAMMOTH", [0, 0, 0, 0, 0, 0, 0, 10], [0, 0, 127, 0], 0, 1, (10, 10, 10, 95), 17),
        ("GLACIER PAD", [0, 0, 0, 0, 0, 0, 30, 11], [0, 0, 127, 0], 0, 0, (0, 60, 20, 120), 5),
        ("METEOR", [0, 0, 0, 0, 0, 0, 0, 12], [0, 0, 127, 0], 0, 0, (0, 20, 50, 115), 4),
        ("CICADA BUZZ", [0, 0, 0, 0, 0, 0, 0, 13], [0, 0, 127, 0], 0, 0, (0, 0, 20, 90), 17),
        ("AMBER BELLS", [0, 0, 0, 0, 0, 0, 0, 14], [0, 0, 127, 0], 0, 0, (0, 20, 40, 110), 7),
        ("TAR GROWL", [0, 3, 20, 0, 0, 0, 0, 8], [0, 0, 127, 0], 0, 1, (25, 0, 0, 40), 22),
        ("MAMMOTH CALL", [0, 0, 15, 0, 10, 3, 0, 10], [0, 0, 127, 0], 0, 1, (10, 0, 20, 110), 17),
        ("GLACIER WIDE", [0, 0, -10, 0, 0, 0, 70, 11], [0, 0, 127, 0], 0, 0, (0, 90, 30, 127), 5),
    ],
    "SAURUS": [  # CALL (HOLD BELLOW ROAR BARK HOOT TRILL CHIRP MOAN HISS SNORT SHRIEK HONK BOOM RATTLE) SIZE SWOOP LEN |
        # GROWL BRTH MOUTH WILD (PALEOTONE 2: the layered throats, tools/saurus_design.py; 64 = the call as made)
        # LEN: 1 ms (0) .. 10 s (127), exponential: 82 ~0.4 s, 96 ~1 s, 104 ~1.9 s, 110 ~3 s, 114 ~4 s
        ("BRACHIO CALL", [1, 104, 60, 110, 50, 60, 70, 25], [30, 90, 127, 90], 0, 0, (0, 20, 15, 110), 17),   # a long-neck's
        ("TYRANT ROAR", [2, 64, 64, 112, 64, 64, 64, 30], [2, 90, 127, 80], 0, 1, (15, 0, 10, 100), 17),     # bellow
        ("RAPTOR BARK", [3, 64, 64, 92, 64, 64, 64, 40], [0, 70, 127, 40], 0, 0, (5, 0, 25, 85), 4),
        ("RAPTOR CHAT", [5, 64, 64, 98, 64, 64, 64, 60], [1, 80, 110, 40], 0, 0, (0, 0, 20, 85), 4),
        ("CREST HOOT", [4, 70, 50, 104, 40, 50, 60, 15], [40, 90, 127, 100], 0, 0, (0, 30, 20, 115), 17),     # a crested
        ("PTERO SHRIEK", [10, 30, 80, 98, 50, 70, 80, 50], [0, 80, 110, 50], 0, 0, (10, 0, 30, 95), 4),    # duckbill
        ("HERD MOAN", [7, 76, 64, 110, 64, 64, 64, 40], [30, 90, 120, 100], 0, 0, (0, 40, 10, 110), 5),
        ("FRILL HISS", [8, 64, 0, 96, 64, 64, 64, 30], [4, 80, 110, 60], 0, 0, (0, 0, 10, 80), 17),
        ("SAURO SNORT", [9, 90, 0, 88, 64, 64, 64, 30], [0, 80, 127, 50], 0, 0, (0, 0, 0, 90), 4),
        ("DEEP RUMBLE", [0, 110, 0, 0, 80, 40, 40, 20], [80, 90, 127, 110], 0, 0, (10, 30, 0, 110), 17),      # a drone
        ("TITAN APE", [2, 80, 50, 104, 90, 70, 80, 30], [4, 90, 127, 80], 0, 1, (20, 0, 10, 95), 17),
        ("NEST CHIRPS", [6, 30, 64, 84, 40, 50, 64, 90], [0, 70, 110, 40], 0, 0, (0, 20, 30, 90), 4),
        ("SWAMP CROAK", [13, 50, 40, 100, 70, 40, 50, 40], [6, 80, 120, 60], 0, 0, (0, 0, 10, 90), 8),
        ("MONSTER ROAR", [2, 90, 70, 112, 100, 90, 80, 45], [2, 90, 127, 70], 0, 1, (40, 0, 20, 90), 17),    # B-movie
        ("DISTANT HERD", [7, 90, 50, 112, 50, 80, 50, 50], [80, 90, 120, 120], 0, 0, (0, 40, 25, 127), 5),
        ("LOW GROWL", [13, 80, 30, 116, 90, 60, 64, 40], [20, 90, 127, 80], 0, 1, (10, 0, 0, 90), 17),
        ("BABY REX", [2, 30, 64, 100, 50, 50, 70, 40], [2, 90, 127, 60], 0, 1, (5, 0, 20, 90), 4),
        ("SEA MONSTER", [1, 120, 50, 116, 100, 60, 50, 30], [50, 90, 127, 120], 0, 0, (10, 30, 20, 127), 17),
        ("HADRO HORN", [11, 64, 64, 110, 64, 64, 64, 20], [30, 90, 127, 110], 0, 0, (0, 20, 30, 120), 17),   # a crest's
        ("COMPY PACK", [3, 20, 64, 82, 50, 60, 64, 110], [0, 64, 100, 30], 0, 0, (0, 20, 30, 85), 13),       # resonant horn
        ("RAPTOR CRY", [10, 64, 64, 100, 64, 64, 64, 40], [0, 80, 120, 50], 0, 0, (5, 0, 25, 90), 4),     # PALEOTONE 2
        ("TITAN BOOM", [12, 64, 64, 114, 64, 64, 64, 20], [60, 90, 127, 110], 0, 0, (0, 20, 10, 120), 17),
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
