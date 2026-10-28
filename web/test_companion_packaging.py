#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""No-hardware smoke test for companion file packaging and script references."""
import json
import tempfile
from pathlib import Path
from prepare_companion import prepare

with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    source, dest = root / "source", root / "dest"
    source.mkdir()
    for name in ("screen.html", "remote_screen.js", "second_screen.js",
                 "control_target.js", "remote_views.json", "felucca.wasm"):
        (source / name).write_bytes(b"stub renderer data")
    (source / "remote_views.json").write_text(json.dumps({"rendererId": 123456, "views": []}))
    prepare(source, dest, "test-pinned")
    expected = ("index.html", "remote_screen.js", "second_screen.js", "control_target.js",
                "remote_views.json", "felucca.wasm", "frame_presenter.js",
                "screen_router.js", "build.json")
    for name in expected:
        assert (dest / name).is_file(), f"missing packaged file: {name}"
    html = (dest / "index.html").read_text()
    for module in ("remote_screen.js", "second_screen.js", "control_target.js",
                   "frame_presenter.js", "screen_router.js"):
        assert module in html, f"expected local module not referenced: {module}"
        assert (dest / module).exists(), f"unpackaged dependency: {module}"
    assert "fast-refresh" in html and "FramePresenter" in html
    meta = json.loads((dest / "build.json").read_text())
    assert meta["rendererId"] == 123456
    assert meta["source"] == "test-pinned"
    assert "frames" in html and (dest / "frame_presenter.js").stat().st_size > 0
print("companion packaging smoke test passed (synthetic renderer files)")
