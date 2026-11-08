#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make PALEOTONE's site (GitHub Pages) for the FM-1 build:

  index.html                  redirect to the browser version (webapp/emu/)
  webapp/emu/                 PALEOTONE in the browser: build/emu as web/emu/build.sh made it
  webapp/installer/index.html web/paleotone_install.html with fm1pkg.js, fm1ota.js and the package's metadata inlined
  webapp/editor/              the editor (web/editor.html, SLOOP's, which speaks to this firmware)
  webapp/art/                 the page art and screens (web/site/art), the demo song
  firmware/paleotone-fm1-VER.fwsc  the package (+ LICENSE, LICENSING.md, LICENSES/)

  web/make_paleotone_site.py build/felucca.fwsc VERSION OUT_DIR

Only these paths are written: OUT_DIR may be a checkout of the gh-pages branch with other pages in it.
"""
import hashlib
import json
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
import make_site  # noqa: E402  (the shared parts: strip_module, product_of, editor_files)

ONLINE = "https://fantasmoslam.github.io/Paleotone/"


def main(pkg, version, out):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = make_site.product_of(raw)
    if not re.fullmatch(r"FM-1_9\d\d", product):
        raise SystemExit(f"{pkg}: identity {product!r} is not an FM-1 Felucca package (FM-1_9xx)")
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: no Felucca update loader in it")
    emu = ROOT / "build" / "emu"
    if not (emu / "paleotone.wasm").exists():
        raise SystemExit("no build/emu/paleotone.wasm: run sh web/emu/build.sh first")
    vtag = re.sub(r"[^A-Za-z0-9.-]", "-", version)
    name = f"paleotone-fm1-{vtag}.fwsc"
    app, fw = out / "webapp", out / "firmware"
    inst, ed, art = app / "installer", app / "editor", app / "art"
    for d in (inst, ed, art, fw):
        d.mkdir(parents=True, exist_ok=True)
    for old in list(fw.glob("felucca-*.fwsc")) + list(fw.glob("paleotone-*.fwsc")):   # one package: the current one
        old.unlink()
    shutil.copy(pkg, fw / name)
    for f in ("LICENSE", "LICENSING.md"):
        shutil.copy(ROOT / f, fw / f)
    if (fw / "LICENSES").exists():
        shutil.rmtree(fw / "LICENSES")
    shutil.copytree(ROOT / "LICENSES", fw / "LICENSES")
    names = sorted(f.name for f in (fw / "LICENSES").glob("*.txt"))
    (fw / "LICENSES" / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>Licences</title><h1>Licence texts</h1><ul>'
        + "".join(f'<li><a href="{n}">{n}</a></li>' for n in names)
        + '</ul><p><a href="../LICENSING.md">LICENSING.md</a> · <a href="../LICENSE">LICENSE (GPL-3.0)</a></p>\n',
        encoding="utf-8")

    html = (HERE / "paleotone_install.html").read_text(encoding="utf-8")
    lib = make_site.strip_module((HERE / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        make_site.strip_module((HERE / "fm1ota.js").read_text(encoding="utf-8"))
    meta = json.dumps({"version": version, "product": product, "pkg": "../../firmware/" + name,
                       "sha256": hashlib.sha256(raw).hexdigest()})
    for mark in ("/*LIB*/", "/*META*/", "<!--BANNER-->"):
        if html.count(mark) != 1:
            raise SystemExit(f"paleotone_install.html must contain {mark} once; update make_paleotone_site.py")
    banner = ""
    if "--preview" in sys.argv:                      # a build from a branch, before it is merged
        banner = ('<aside class="alpha" role="note"><b>Preview.</b> This page shows a branch under review, '
                  'not the published release.</aside>')
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta).replace("<!--BANNER-->", banner)
    dl = make_site.editor_files(ed, version, "PALEOTONE Editor", ONLINE)
    if html.count('download="sloop-editor.html"') != 1:
        raise SystemExit('paleotone_install.html must link download="sloop-editor.html" once')
    (inst / "index.html").write_text(html.replace('download="sloop-editor.html"', dl), encoding="utf-8")

    for f in (ROOT / "web" / "site" / "art").iterdir():
        shutil.copy(f, art / f.name)
    if (app / "emu").exists():
        shutil.rmtree(app / "emu")
    shutil.copytree(emu, app / "emu")
    (out / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>PALEOTONE</title>'
        '<meta http-equiv="refresh" content="0; url=webapp/emu/"><a href="webapp/emu/">PALEOTONE</a>\n',
        encoding="utf-8")
    (out / ".nojekyll").write_text("")
    print(f"site: {out}: webapp/emu, webapp/installer ({len(html)} B), webapp/editor, firmware/{name} "
          f"({len(raw)} B, {product})")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a != "--preview"]
    if len(args) != 3:
        sys.exit(__doc__)
    main(*args)
