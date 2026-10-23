#!/usr/bin/env python3
"""Build a deterministic, four-track DINOBOX sequencer arrangement plan for RIDERS.

This is an intermediate representation, not a firmware-ready project or rendered preview.
The original tools/dino_riders.py remains the definitive studio arrangement.
No external dependencies: python3 tools/riders_native_plan.py [output.json]
"""
# SPDX-License-Identifier: GPL-3.0-only
import json
import sys
from pathlib import Path

BPM = 84
BARS = 48
# MIDI notes from the existing Riders composition (D minor).
DM, BB, CC, AA, FF, GM = (50, 53, 57), (50, 53, 58), (48, 52, 55), (49, 52, 57), (48, 53, 57), (50, 55, 58)
THEME = [DM, BB, CC, AA, DM, FF, GM, AA]
RALLY = [DM, DM, BB, CC, DM, DM, BB, AA]
BRIDGE = [BB, CC, DM, DM, BB, CC, AA, AA]
PROG = [DM] * 8 + RALLY + THEME + BRIDGE + THEME + [BB, CC, AA, AA] + [DM] * 4
ROOT = {DM: 38, BB: 34, CC: 36, AA: 33, FF: 41, GM: 43}
# Source: MEL, FANFARE and FANFARE2 in tools/dino_riders.py.
MEL = [
    (0,0,74,1.5),(0,1.5,69,.5),(0,2,74,1),(0,3,76,1),
    (1,0,77,2),(1,2,76,1),(1,3,74,1),
    (2,0,72,1.5),(2,1.5,69,.5),(2,2,72,1),(2,3,74,1),
    (3,0,76,3),(3,3,69,1),
    (4,0,74,1.5),(4,1.5,69,.5),(4,2,74,1),(4,3,76,1),
    (5,0,77,1),(5,1,79,1),(5,2,81,2),
    (6,0,82,1.5),(6,1.5,81,.5),(6,2,79,1),(6,3,77,1),
    (7,0,76,2),(7,2,73,1),(7,3,74,1)]
FANFARE = [(0,0,69,.5),(0,.5,69,.5),(0,1,74,1.5),(0,2.5,69,.5),(0,3,74,1),
           (1,0,77,2),(1,2,76,1),(1,3,74,1)]
FANFARE2 = [(0,0,69,.5),(0,.5,69,.5),(0,1,74,1.5),(0,2.5,69,.5),(0,3,74,1),
            (1,0,76,1),(1,1,73,1),(1,2,69,2)]
RHYTHM = [(0,8,"STAMPEDE"),(8,8,"WAR PARADE"),(16,8,"TRIBAL WAR"),
          (24,8,"HEARTBEAT"),(32,12,"STAMPEDE"),(44,4,"CHAOS DRUMS")]
CREATURES = [(2,0,"BRACHIO CALL",45,6),(5,1,"DISTANT HERD",43,6),
             (12,3.5,"RAPTOR BARK",64,.5),(13,3,"RAPTOR BARK",62,.5),
             (13,3.5,"RAPTOR BARK",66,.5),(23,3,"PTERO SHRIEK",74,2),
             (24,0,"TYRANT ROAR",45,6),(28,0,"HADRO HORN",50,6),
             (31,0,"BABY REX",52,2),(36,3.5,"RAPTOR BARK",64,.5),
             (37,3.5,"RAPTOR BARK",66,.5),(40,0,"MONSTER ROAR",43,6),
             (46,0,"TYRANT ROAR",43,6),(47,2,"BRACHIO CALL",45,6)]

def event(bar, beat, note, duration, velocity=110):
    assert 0 <= bar < BARS and 0 <= beat < 4 and 0 < duration
    return dict(bar=bar, beat=beat, note=note, duration=duration, velocity=velocity)

def build():
    """Four prioritized lanes. Source has more parts than available lanes.

    T1 ROAR controls built-in rhythmic drone, T2 PRIMEVAL plays melodic hook,
    T3 TAR PIT BASS accompanies, T4 WAR DRUMS provides backbeat.
    Creature cues remain annotations until track-muting/automation is solved.
    """
    sections = []
    for start, length, name in RHYTHM:
        sections.append(dict(start_bar=start, bars=length, roar_preset=name))
    lead = []
    for start, phrase in ((10,FANFARE),(14,FANFARE2),(16,MEL),(32,MEL),(40,MEL[:13])):
        for b, beat, note, duration in phrase:
            lead.append(event(start+b,beat,note-12,duration*.95))
    lead.append(event(44,0,62,7,120))
    bass, drums = [], []
    for bar in range(8,44):
        root = ROOT[PROG[bar]]
        if 24 <= bar < 32:
            bass.append(event(bar,0,root,3.8,100))
            if bar < 31:
                drums.extend([event(bar,0,45,.3),event(bar,2.5,45,.3,90)])
        else:
            for eighth in range(8):
                bass.append(event(bar,eighth/2,root+(12 if eighth in (3,7) else 0),.42,120 if eighth%2==0 else 95))
            if bar < 16:
                drums.extend([event(bar,0,36,.3,120),event(bar,2,36,.3)])
            else:
                for beat,note in ((0,36),(1,38),(1.5,36),(2,36),(3,38)):
                    drums.append(event(bar,beat,note,.3))
            if bar%4==3:
                drums.extend([event(bar,3.5,45,.2),event(bar,3.75,45,.2)])
    bass.append(event(44,0,38,8))
    drums.extend([event(44,0,36,.3,127),event(44,0,56,.3,127)])
    data = dict(title="RIDERS OF THE VALLEY",tempo=BPM,bars=BARS,
                status="sequencer arrangement plan (not installable firmware)",
                sections=sections,
                tracks=[
                    dict(track=1,engine="ROAR",purpose="section rhythm and drone",note=38,segments=sections),
                    dict(track=2,engine="ROAR",preset="PRIMEVAL",purpose="principal melody",events=lead),
                    dict(track=3,engine="FM6",preset="TAR PIT BASS",purpose="bass",events=bass),
                    dict(track=4,engine="DRUM",preset="WAR DRUMS",purpose="percussion",events=drums)],
                deferred=dict(creature_events=[dict(bar=b,beat=bt,preset=p,note=n,duration=d) for b,bt,p,n,d in CREATURES],
                              extra_parts=["ICE AGE PAD","MAMMOTH stabs","DEEP CHANT","SWAMP FIDDLE","ENCOUNTER","jungle ambience"],
                              reason="Need arrangement compromises, alternate track slots or a scene layer; four tracks cannot sound every studio stem simultaneously."))
    assert len(PROG)==BARS
    assert sum(x["bars"] for x in sections)==BARS
    assert all(e["bar"]<BARS for t in data["tracks"][1:] for e in t["events"])
    return data

if __name__=="__main__":
    target=Path(sys.argv[1]) if len(sys.argv)>1 else Path("build/riders_native_plan.json")
    target.parent.mkdir(parents=True,exist_ok=True)
    data=build()
    target.write_text(json.dumps(data,indent=2)+"\n")
    print(f"{target}: {data['bars']} bars at {data['tempo']} BPM, 4 tracks; preview pending")
