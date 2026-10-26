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
    page = Path(__file__).with_name("companion_page.html").read_text()
    shutil.copyfile(Path(__file__).with_name("frame_presenter.js"), destination / "frame_presenter.js")
    # the page's script: forward a visitor whose firmware needs a newer screen page (make_site.py's
    # webapp/screens.json); it must ship with the page or the page 404s it
    router = Path(__file__).with_name("screen_router.js")
    if not router.is_file():
        raise SystemExit(f"graphics companion needs {router}")
    shutil.copyfile(router, destination / "screen_router.js")
    (destination / "index.html").write_text(page)
    (destination / "build.json").write_text(json.dumps({"source": revision,
        "rendererId": layout["rendererId"], "kind": "firmware-graphics-companion", "presentation": "android-compatible-v1"}, indent=2) + "\n")


if __name__ == "__main__":
    prepare(*sys.argv[1:])
