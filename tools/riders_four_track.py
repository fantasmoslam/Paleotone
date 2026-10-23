#!/usr/bin/env python3
"""Emit a four-track audition score; no firmware or project files are changed.

Read the melody/harmony literals from dino_riders.py without running its desktop
production pipeline. Pipe stdout into build/host/riders_preview OUT.wav.
"""
import ast
from pathlib import Path

source = Path(__file__).with_name('dino_riders.py').read_text()
names = {}
for node in ast.parse(source).body:
    if isinstance(node, ast.Assign):
        if any(t.id in {'DM', 'BB', 'CC', 'AA', 'FF', 'GM', 'ROOT_OF', 'THEME',
                        'RALLY', 'BRIDGE', 'CLIMAX', 'PROG', 'MEL', 'FANFARE', 'FANFARE2'}
               for target in node.targets for t in ast.walk(target) if isinstance(t, ast.Name)):
            exec(compile(ast.Module(body=[node], type_ignores=[]), source, 'exec'),
                 {'__builtins__': {}}, names)

def note(track, beat, pitch, length, velocity):
    print(f'{track} {beat:.4f} {pitch} {length:.4f} {velocity}')

# Four fixed presets: STAMPEDE, TAR PIT BASS, PRIMEVAL, ICE AGE PAD.
# The drone is rearticulated every four bars, matching a feasible 64-step phrase.
for bar in range(0, 48, 4):
    note(0, bar * 4, 38, 15.9, 100)
for bar in range(8, 44):
    root = names['ROOT_OF'][names['PROG'][bar]]
    if 24 <= bar < 32:
        note(1, bar * 4, root, 3.8, 95)
    else:
        for eighth in range(8):
            note(1, bar * 4 + eighth * .5, root + (12 if eighth in (3, 7) else 0),
                 .42, 110 if eighth % 2 == 0 else 90)
note(1, 176, 38, 7.8, 100)
for melody, start in [('FANFARE', 10), ('FANFARE2', 14), ('MEL', 16), ('MEL', 32)]:
    for bar, beat, pitch, length in names[melody]:
        note(2, (start + bar) * 4 + beat, pitch - 12, length * .95, 108)
for bar, beat, pitch, length in names['MEL'][:13]:
    note(2, (40 + bar) * 4 + beat, pitch - 12, length * .95, 108)
note(2, 176, 62, 7, 110)
for bar in range(4, 44):
    for pitch in names['PROG'][bar]:
        note(3, bar * 4, pitch - 12, 3.8, 75)
for pitch in (50, 57, 62):
    note(3, 176, pitch - 12, 7.8, 80)
