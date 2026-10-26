# SPDX-License-Identifier: GPL-3.0-only
"""List graphics pages, preferring a candidate's nested screen over its synth."""
import json

def screen_pages(webapp):
    pages = []
    for build in sorted(webapp.glob('*/build.json')):
        nested = build.parent / 'screen' / 'build.json'
        if nested.is_file():
            build = nested
        try:
            ident = json.loads(build.read_text(encoding='utf-8')).get('rendererId')
        except (OSError, ValueError):
            continue
        if ident:
            relative = build.parent.relative_to(webapp).as_posix()
            pages.append(dict(rendererId=ident, path=relative+'/', label=relative))
    return pages
