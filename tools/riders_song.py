#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# RIDERS OF THE VALLEY as a native FM-1 song: four project slots and a song chain, written as a score for
# tests/song_score.c (the firmware's own sequencer and engines). The notes come from tools/riders_native_plan.py;
# the balance is the owner-approved preview (tools/riders_preview.py: the lead 12 dB under the others, all dry).
#   python3 tools/riders_song.py [chaos|stampede] > score.txt
#   build/host/song_score OUT.wav < score.txt
#
# Tracks: T1 PRIMEVAL lead, T2 ROAR rhythm (STAMPEDE as its sound; the other rhythms by automation), T3 TAR PIT BASS,
# T4 WAR DRUMS. The lead is T1 so that its empty bridge pattern can time the intro (a section lasts the loops of its
# first sounding track).
# Slots: A rally (bars 8-15), B theme (16-23, again at 32-39), C bridge (24-31), D finale (40-47).
# ROAR's rhythm: every section restarts each track, and the sounds stay one set, so each slot's ROAR pattern starts
# from one rhythm. Five rhythms, four slots: the finale's switch to CHAOS DRUMS has to happen inside slot D, so
#   chaos:    the original ending (STAMPEDE 40-43, CHAOS DRUMS 44-47); the climax's first 8 bars ride TRIBAL WAR
#   stampede: STAMPEDE through the whole climax (32-47) as the original had it, no CHAOS DRUMS ending
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import riders_native_plan as plan  # noqa: E402

LEAD, ROAR, BASS, DRUMS = 0, 1, 2, 3
GURDY, FM6, DRUM = 14, 12, 10
DIV_8, DIV_BAR = 1, 7
SIL = 4
LEVEL = {LEAD: 104 - 24, ROAR: 104, BASS: 104, DRUMS: 104}     # 0.5 dB a step: the lead -12 dB (option B)
GATE = {LEAD: 121, BASS: 107, DRUMS: 127}                       # (the plan's lengths: .95 of a note, .42 of an 1/8)
SLOT_BARS = {0: 8, 1: 16, 2: 24, 3: 40}                         # the first bar each slot's patterns hold
ROAR_RHYTHM = {0: "WAR PARADE", 1: "TRIBAL WAR", 2: "HEARTBEAT"}  # slot D: STAMPEDE, the sound's own


def score(option):
    d = plan.build()
    tracks = {t["track"]: t for t in d["tracks"]}
    events = {LEAD: tracks[2]["events"], BASS: tracks[3]["events"], DRUMS: tracks[4]["events"]}
    out = [f"# RIDERS OF THE VALLEY, native, option {option}", f"bpm {plan.BPM}"]
    out += [f"sound {LEAD} {GURDY} PRIMEVAL", f"sound {ROAR} {GURDY} STAMPEDE",
            f"sound {BASS} {FM6} TAR PIT BASS", f"sound {DRUMS} {DRUM} WAR DRUMS", f"param {DRUMS} E1 14",
            f"param {LEAD} RING 0"]                                    # (as approved: no drone through the rests)
    out += [f"level {k} {v}" for k, v in LEVEL.items()]
    for s, b0 in SLOT_BARS.items():
        out.append(f"slot {s}")
        # the lead's bridge pattern is empty: 4 bars in the chaos version, to time the intro (two 4-bar sections)
        lead_len = 32 if (s == 2 and option == "chaos") else 64
        out.append(f"pat {LEAD} {DIV_8} {lead_len} {GATE[LEAD]}")
        for k in (BASS, DRUMS):
            out.append(f"pat {k} {DIV_8} 64 {GATE[k]}")
        for k, evs in events.items():
            sixteenths = {(e["bar"], e["beat"] - 0.25) for e in evs if (e["beat"] * 2) % 1}
            for e in evs:
                if not b0 <= e["bar"] < b0 + 8 or (e["beat"] * 2) % 1:
                    continue                                          # (a 16th: the ratchet of the step before)
                step = (e["bar"] - b0) * 8 + int(e["beat"] * 2)
                n = max(1, round(e["duration"] * 2))
                r = " 2" if (e["bar"], e["beat"]) in sixteenths else ""
                out.append(f"n {k} {step} {e['note']} {min(n, 64 - step)} {e['velocity']}{r}")
        out.append(f"pat {ROAR} {DIV_BAR} 8 127")
        out.append(f"n {ROAR} 0 38 8 110")
        if s in ROAR_RHYTHM:
            out.append(f"mpreset {ROAR} 0 {ROAR_RHYTHM[s]}")
        elif option == "chaos":
            out.append(f"mpreset {ROAR} 4 CHAOS DRUMS")                 # bar 44
        out.append("save")
    A, B, C, D = 0, 1, 2, 3
    if option == "chaos":
        rows = [(C, D, SIL, SIL, 1), (C, D, SIL, SIL, 1),             # intro: STAMPEDE, 4 + 4 bars
                (A, A, A, A, 1), (B, B, B, B, 1), (C, C, C, C, 2),
                (B, B, B, B, 1),                                      # climax: the theme again, on TRIBAL WAR
                (D, D, D, D, 1)]                                      # STAMPEDE, then CHAOS DRUMS at bar 44
    else:
        rows = [(C, D, SIL, SIL, 1), (A, A, A, A, 1), (B, B, B, B, 1), (C, C, C, C, 1),
                (B, D, B, B, 1),                                      # climax: the theme on STAMPEDE
                (D, D, D, D, 1)]
    out += [f"row {a} {b} {c} {d_} {r}" for a, b, c, d_, r in rows]
    out.append(f"secs {plan.BARS * 4 * 60 / plan.BPM + 4:.2f}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    opt = sys.argv[1] if len(sys.argv) > 1 else "chaos"
    assert opt in ("chaos", "stampede"), opt
    sys.stdout.write(score(opt))
