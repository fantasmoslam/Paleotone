#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Published native-focus pages must retain their tested controller and assets."""
import importlib.util
import json
import shutil
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("prepare_companion", ROOT / "web/prepare_companion.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    source = root / "source"
    source.mkdir()
    for name in ("screen.html", "remote_screen.js", "second_screen.js", "control_target.js",
                 "physical_focus.js", "display_performance.js", "frame_presenter.js"):
        shutil.copyfile(ROOT / "web" / name, source / name)
    (source / "remote_views.json").write_text(json.dumps({"rendererId": 123, "schema": 2}))
    (source / "felucca.wasm").write_bytes(b"packaging fixture")
    destination = root / "native"
    module.prepare(source, destination, "fixture")
    page = (destination / "index.html").read_text(encoding="utf-8")
    assert page == (source / "screen.html").read_text(encoding="utf-8")
    assert "PhysicalFocus" in page and "await focus.poll(enabled)" in page
    assert "ControlTarget" not in page
    assert json.loads((destination / "build.json").read_text())["physicalFocusProtocol"] == 2
    for name in ("physical_focus.js", "display_performance.js", "frame_presenter.js", "felucca.wasm"):
        assert (destination / name).read_bytes() == (source / name).read_bytes()

    (source / "physical_focus.js").unlink()
    try:
        module.prepare(source, root / "incomplete", "fixture")
    except SystemExit as error:
        assert "physical_focus.js" in str(error)
    else:
        raise AssertionError("Native focus page was published without its controller")

    (source / "screen.html").write_text("<html>legacy renderer</html>")
    module.prepare(source, root / "legacy", "fixture")
    assert "physicalFocusProtocol" not in json.loads((root / "legacy/build.json").read_text())
    assert (root / "legacy/frame_presenter.js").is_file()
    assert (root / "legacy/screen_router.js").is_file()
    assert 'screen_router.js' in (root / "legacy/index.html").read_text()
print("companion packaging: native focus page and dependencies preserved; missing controller rejected; legacy fallback retained")
