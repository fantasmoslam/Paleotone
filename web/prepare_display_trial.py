#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage a browser-only trial using the exact published ROAR-worker WASM."""
import hashlib
import json
import shutil
import sys
from pathlib import Path

SOURCE = '0a1327c816086576b3d741e785c2cfbf7e3d8c42'
RENDERER = 3513154928
ASSETS = ('felucca.wasm', 'remote_views.json', 'remote_screen.js',
          'second_screen.js', 'control_target.js', 'physical_focus.js',
          'frame_presenter.js')


def prepare(source, destination):
    source, destination = Path(source), Path(destination)
    metadata = json.loads((source / 'build.json').read_text())
    layout = json.loads((source / 'remote_views.json').read_text())
    if (metadata.get('source') != SOURCE or metadata.get('rendererId') != RENDERER
            or metadata.get('physicalFocusProtocol') != 2
            or layout.get('rendererId') != RENDERER or layout.get('schema') != 2):
        raise SystemExit('Trial requires the exact published ROAR-worker companion.')
    for name in ASSETS:
        if not (source / name).is_file():
            raise SystemExit(f'Missing companion asset: {name}')
    # A new output directory avoids leaving firmware/installer files in a trial.
    destination.mkdir(parents=True, exist_ok=False)
    for name in ASSETS:
        shutil.copyfile(source / name, destination / name)
    own = Path(__file__).parent
    shutil.copyfile(own / 'screen.html', destination / 'index.html')
    shutil.copyfile(own / 'display_performance.js', destination / 'display_performance.js')
    metadata['browserTrial'] = 'audio-priority-20261028'
    metadata['rendererSha256'] = hashlib.sha256((destination / 'felucca.wasm').read_bytes()).hexdigest()
    (destination / 'build.json').write_text(json.dumps(metadata, indent=2) + '\n')


if __name__ == '__main__':
    prepare(*sys.argv[1:])
