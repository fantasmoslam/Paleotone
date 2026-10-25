#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Publish the complete graphics renderer from the pinned companion build."""
import json
import shutil
import sys
from pathlib import Path


def prepare(source, destination, revision):
    source, destination = Path(source), Path(destination)
    required = ("screen.html", "remote_screen.js", "second_screen.js", "control_target.js",
                "remote_views.json", "felucca.wasm")
    for name in required:
        if not (source / name).is_file():
            raise SystemExit(f"graphics companion missing {source / name}")
    layout = json.loads((source / "remote_views.json").read_text())
    destination.mkdir(parents=True, exist_ok=True)
    for name in required:
        shutil.copyfile(source / name, destination / ("index.html" if name == "screen.html" else name))
    page = (destination / "index.html").read_text()
    # Retire the lightweight app's worker, which previously owned this same URL.
    cleanup = '''<script>
if ('serviceWorker' in navigator) navigator.serviceWorker.getRegistrations().then(registrations => {
  const scope = new URL('./', location.href).href;
  for (const registration of registrations) if (registration.scope === scope) registration.unregister();
}).catch(() => {});
</script>
'''
    page = page.replace('</head>', cleanup + '</head>')
    (destination / "index.html").write_text(page)
    (destination / "build.json").write_text(json.dumps({"source": revision,
        "rendererId": layout["rendererId"], "kind": "firmware-graphics-companion"}, indent=2) + "\n")


if __name__ == "__main__":
    prepare(*sys.argv[1:])
