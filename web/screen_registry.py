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
            metadata = json.loads(build.read_text(encoding='utf-8'))
            # An explicit browser experiment must not become the automatic
            # destination for every instrument sharing its renderer identity.
            if metadata.get('browserTrial'):
                continue
            ident = metadata.get('rendererId')
        except (OSError, ValueError):
            continue
        if ident:
            relative = build.parent.relative_to(webapp).as_posix()
            pages.append(dict(rendererId=ident, path=relative+'/', label=relative))
    return pages
