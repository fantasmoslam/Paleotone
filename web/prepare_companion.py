#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Publish the complete graphics renderer from the pinned companion build."""
import json
import shutil
import sys
from pathlib import Path


def prepare(source, destination, revision):
    source, destination = Path(source), Path(destination)
    page = (source / "screen.html").read_text(encoding="utf-8")
    physical_focus = "PhysicalFocus" in page
    required = ("screen.html", "remote_screen.js", "second_screen.js", "control_target.js",
                "remote_views.json", "felucca.wasm")
    if physical_focus:
        required += ("physical_focus.js", "display_performance.js", "frame_presenter.js")
    for name in required:
        if not (source / name).is_file():
            raise SystemExit(f"graphics companion missing {source / name}")
    layout = json.loads((source / "remote_views.json").read_text())
    destination.mkdir(parents=True, exist_ok=True)
    for name in required:
        shutil.copyfile(source / name, destination / ("index.html" if name == "screen.html" else name))
    if not physical_focus:
        page = Path(__file__).with_name("companion_page.html").read_text(encoding="utf-8")
        shutil.copyfile(Path(__file__).with_name("frame_presenter.js"), destination / "frame_presenter.js")
    (destination / "index.html").write_text(page, encoding="utf-8")
    metadata = {"source": revision, "rendererId": layout["rendererId"],
        "kind": "firmware-graphics-companion", "presentation": "android-compatible-v1"}
    if physical_focus:
        metadata["physicalFocusProtocol"] = 2
    (destination / "build.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    prepare(*sys.argv[1:])
