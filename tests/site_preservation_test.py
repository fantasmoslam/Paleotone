#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run the actual site preservation step, validate exact routes, never publish."""
import hashlib
import json
import subprocess
import textwrap
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'web'))
from screen_registry import screen_pages

workflow = (ROOT / '.github/workflows/site.yml').read_text()
marker = workflow.index('      - name: Preserve the tested releases published beside the site')
start = workflow.index('        run: |\n', marker) + len('        run: |\n')
end = workflow.index('\n      - name:', start)
script = textwrap.dedent(workflow[start:end])
destination = ROOT / 'site'
if destination.exists():
    raise SystemExit('Use a clean staging directory: site already exists')
subprocess.run(['bash', '-e', '-o', 'pipefail', '-c', script], cwd=ROOT, check=True)
published = json.loads(subprocess.check_output(
    ['git', 'show', 'origin/gh-pages:webapp/screens.json'], cwd=ROOT, text=True))['pages']
actual = screen_pages(destination / 'webapp')
routes = {p['path']: p['rendererId'] for p in actual}
assert 'audio-priority-pr25/' not in routes, 'explicit trial must not replace the default renderer route'
for page in published:
    # The legacy root companion is built separately, not preserved here.
    if page['path'] != 'screen/':
        assert routes.get(page['path']) == page['rendererId'], page
trial = destination / 'webapp/audio-priority-pr25'
assert trial.is_dir(), 'published trial URL was lost'
page = (trial/'index.html').read_text()
assert '<base ' not in page, 'trial must not depend on mutable sibling assets'
assert 'lastSurface' in page
assert 'updates/s (recent)' in (trial/'display_performance.js').read_text()
meta = json.loads((trial/'build.json').read_text())
assert hashlib.sha256((trial/'felucca.wasm').read_bytes()).hexdigest() == meta['rendererSha256']
assert (trial/'frame_presenter.js').read_bytes() == (ROOT/'web/frame_presenter.js').read_bytes()
print(f'preserved {len(published)-1} installed renderer routes; full updated trial staged with pinned WASM')
