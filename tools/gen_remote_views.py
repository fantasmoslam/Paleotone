#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Describe browser-only view overrides of the field-encoded display snapshot.

Only the second renderer receives these copies. Never send them to the device.
The descriptor is bound to the exact renderer identity, not a native C layout.
"""
import json
import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from remote_identity import ROOT, render_id


def generate():
    def clean(path):
        return re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
    core = clean(ROOT / 'firmware/src/core.h')
    globals_enum = re.search(r'enum\s*\{([^{}]*\bG_COUNT\b[^{}]*)\}', core).group(1)
    names = [s.strip() for s in globals_enum.split(',') if s.strip()]
    if any(not re.fullmatch(r'G_\w+', name) for name in names):
        raise ValueError('global enum needs an explicit layout update')
    count = names.index('G_COUNT')
    source = clean(ROOT / 'firmware/src/remote_state.c')
    prefix = re.search(r'static void rs_walk\(void\).*?RS8\(ui.home\)', source, re.S).group(0)
    encodings = re.findall(r'RS(8|16|32)\(([^)]+)\)', prefix)
    expected = [('32', 'fm1_ms'), ('16', 'song.g[i]')]
    expected += [('8', 'song.' + x) for x in ('playing', 'seq_mode', 'grid', 'rec', 'sel', 'octave')]
    expected += [('32', 'song.' + x) for x in ('tick', 'cpu_q8', 'master_q12', 'batt_raw')]
    expected += [('8', 'ui.home')]
    if encodings != expected:
        raise ValueError('snapshot prefix changed; update the view descriptor generator')
    offset = 4 + 2 * count + 6 + 16
    ui = {}
    fields = source[source.index('RS8(ui.home)'):source.index('RSA8(ui.fam_last)')]
    for bits, name in re.findall(r'RS(8|16|32)\(ui\.(\w+)\)', fields):
        ui[name] = offset
        offset += int(bits) // 8
    pages_source = clean(ROOT / 'firmware/src/params.c').split('static const page_t PAGES[] = {', 1)[1].split('#define NPAGES', 1)[0]
    if '#' in pages_source:
        raise ValueError('conditional pages need explicit build-option handling')
    pages = re.findall(r'\{\s*"([^"]+)"\s*,\s*FAM_', pages_source)
    families = re.findall(r'\{\s*"[^"]+"\s*,\s*(FAM_\w+)', pages_source)
    buttons = dict(zip(('HOME','ENV','LFO','FX','SCL','EDIT','GLO','SAVE','ARP','SEQ','TRK'),(6,2,3,0,1,4,5,7,8,9,6)))
    wanted = [('home', 'Home', None), ('mixer', 'Mixer', 'MIXER'), ('sequencer', 'Sequencer', 'STEP'),
              ('envelope', 'Envelope', 'ENV'), ('lfo', 'LFO', 'LFO'), ('fx', 'FX', 'FX'),
              ('arp', 'Arpeggiator', 'ARP'), ('scale', 'Scale', 'SCL'), ('clock', 'Clock', 'CLOCK'),
              ('scape', 'Soundscape', 'SCAPE')]
    views = [{'id': key, 'label': label, 'home': int(title is None),
              'page': 0 if title is None else pages.index(title)} for key, label, title in wanted]
    return {'rendererId': render_id(), 'schema': 1, 'selectedTrack': 4 + 2 * count + 4, 'grid': 4 + 2 * count + 2,
            'ui': ui, 'views': views,
            'pages': [{'page': i, 'label': title, 'button': buttons[families[i][4:]]} for i,title in enumerate(pages)]}


if __name__ == '__main__':
    Path(sys.argv[1]).write_text(json.dumps(generate(), indent=2) + '\n')
