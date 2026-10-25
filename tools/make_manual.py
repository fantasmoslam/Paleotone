#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# PALEOTONE: the user manual as one web page (web/manual/index.html + its pictures), from the manual's Markdown export.
#   tools/make_manual.py MANUAL.md PICS_DIR DIAGRAM_DIR OUT_DIR
# PICS_DIR: the screen pictures (tests/manual_shots.c, 480 x 480), DIAGRAM_DIR: signal.png scape.png pages.png.
# make_site.py copies web/manual to the site's manual/.
import html
import re
import shutil
import sys
from pathlib import Path

import markdown

# each picture's caption in the manual -> its file
PICS = [
    ("The boot screen", "splash.png"), ("HOME with the HERD scene", "home_herd.png"), ("HOME on track 2", "home_saurus.png"),
    ("The ENV page", "env.png"), ("The LFO page", "lfo.png"), ("The VOICE page", "voice.png"), ("The SCL page", "scl.png"),
    ("The ARP page", "arp.png"), ("The PRESETS browser", "presets.png"), ("SAURUS's CALL page", "edit_saurus.png"),
    ("SAURUS's THROAT page", "edit_saurus2.png"), ("ROAR's DRONE page", "edit_roar.png"), ("ROAR's CALL page", "edit_roar2.png"),
    ("FM6's OPS page", "edit_fm6_mammoth.png"), ("VOICE's first page", "edit_voice_chant.png"), ("The FX page", "fx.png"),
    ("The REVERB page", "reverb.png"), ("The TAPE page", "tape.png"), ("The PHRASES page", "phrases.png"),
    ("The STEP page", "step_mammoth.png"), ("The AUTOMATION page", "automation.png"), ("The SONG page", "song.png"),
    ("MIXER with HERD", "mixer.png"), ("CLOCK:", "clock.png"), ("KEYS:", "keys.png"), ("SCAPE MUSIC:", "scape_music.png"),
    ("The SCAPE page", "scape.png"), ("SCAPE DRONE", "scape_drone.png"), ("SCAPE LIFE", "scape_life.png"),
    ("SCAPE LEVELS 2", "scape_levels2.png"), ("SCAPE LEVELS", "scape_levels.png"),
    ("SAVE › PROJECT", "project.png"), ("Naming", "x_name_typing.png"),
    ("SAVE › USER", "x_user.png"), ("MENU, DISPLAY tab", "menu.png"), ("MENU, SYSTEM tab", "x_menu_system.png"),
]
DIAGRAMS = [("the signal path", "signal.png"), ("SCAPE", "scape_diagram.png"), ("the page map", "pages.png")]

CSS = """
:root { --bg: #070c09; --panel: #0d1611; --ink: #d7e4da; --quiet: #93a89a; --green: #4be37f; --amber: #f2b24b; --rule: #1f3326; }
* { box-sizing: border-box; }
html { background: var(--bg); }
body { margin: 0; color: var(--ink); background: var(--bg); font: 17px/1.6 system-ui, -apple-system, "Segoe UI", sans-serif; }
a { color: var(--green); } a:hover { color: var(--amber); }
header.top { border-bottom: 1px solid var(--rule); background: var(--panel); }
header.top div { max-width: 1180px; margin: 0 auto; padding: 14px 16px; display: flex; gap: 20px; flex-wrap: wrap; align-items: center; }
header.top b { font: 700 22px/1 ui-monospace, "Share Tech Mono", monospace; color: var(--green); letter-spacing: 2px; }
.wrap { max-width: 1180px; margin: 0 auto; padding: 0 16px; display: flex; flex-wrap: wrap; gap: 40px; }
nav.toc { flex: 1 1 220px; max-width: 260px; padding-top: 28px; font-size: 15px; }
nav.toc ol { padding-left: 20px; margin: 0; } nav.toc li { margin: 4px 0; }
nav.toc a { color: var(--quiet); text-decoration: none; } nav.toc a:hover { color: var(--green); }
main { flex: 999 1 560px; min-width: 0; padding-bottom: 80px; }
h1, h2, h3 { font-family: ui-monospace, "Share Tech Mono", Menlo, monospace; color: var(--green); line-height: 1.25; }
h1 { font-size: 40px; letter-spacing: 3px; margin: 28px 0 4px; }
h2 { font-size: 27px; margin: 56px 0 12px; padding-top: 12px; border-top: 1px solid var(--rule); }
h3 { font-size: 19px; color: var(--amber); margin: 32px 0 8px; }
code { font: 15px ui-monospace, Menlo, monospace; background: #13211a; padding: 1px 5px; border-radius: 4px; }
table { border-collapse: collapse; width: 100%; margin: 14px 0; font-size: 15px; display: block; overflow-x: auto; }
th, td { border: 1px solid var(--rule); padding: 7px 10px; text-align: left; vertical-align: top; }
th { background: #12201a; color: var(--green); font-weight: 600; }
figure { margin: 18px 0; }
figure img.screen { width: 360px; max-width: 100%; image-rendering: pixelated; border: 2px solid var(--rule); border-radius: 6px; display: block; }
figure img.diagram { width: 100%; max-width: 900px; border-radius: 8px; background: #fff; display: block; }
figcaption { color: var(--quiet); font-size: 14px; margin-top: 6px; max-width: 640px; }
blockquote { margin: 18px 0; padding: 10px 16px; border-left: 3px solid var(--amber); background: #14170e; }
.meta { color: var(--quiet); margin: 0 0 8px; }
"""


def main(md_path, pics, diagrams, out):
    md = Path(md_path).read_text(encoding="utf-8")
    pics, diagrams, out = Path(pics), Path(diagrams), Path(out)
    img_dir = out / "img"
    shutil.rmtree(out, ignore_errors=True)
    img_dir.mkdir(parents=True)
    md = re.sub(r"^Oct \d+, \d{4} · @.*$", "", md, flags=re.M)        # the doc's byline: not for the web
    blocks = {}

    def picture(m):
        alt = html.unescape(m.group(1)).rstrip("\\").strip()
        for key, name in PICS:
            if alt.startswith(key):
                shutil.copy(pics / name, img_dir / name)
                tok = f"PICTURE{len(blocks)}X"
                blocks[tok] = (f'<figure><img class="screen" src="img/{name}" alt="{html.escape(alt)}" loading="lazy">'
                               f'<figcaption>{html.escape(alt)}</figcaption></figure>')
                return tok
        raise SystemExit(f"make_manual: no picture for {alt!r}")

    def diagram(m):
        cap = html.unescape(m.group(1)).rstrip("\\").strip()
        for key, name in DIAGRAMS:
            if cap.startswith(key):
                shutil.copy(diagrams / name, img_dir / name)
                tok = f"PICTURE{len(blocks)}X"
                blocks[tok] = (f'<figure><img class="diagram" src="img/{name}" alt="{html.escape(cap)}" loading="lazy">'
                               f'<figcaption>{html.escape(cap)}</figcaption></figure>')
                return tok
        raise SystemExit(f"make_manual: no diagram for {cap!r}")

    md = re.sub(r"&#91;image: (.*?)\\?\]", picture, md)
    md = re.sub(r"&#91;embedded content: (.*?)\\?\]", diagram, md)
    body = markdown.markdown(md, extensions=["tables", "toc", "sane_lists"])
    for tok, h in blocks.items():
        body = body.replace(f"<p>{tok}</p>", h).replace(tok, h)
    # a link to every https:// address written out in the text
    body = re.sub(r'(?<!["\'>=])(https://[^\s<,)]+[^\s<,).])', r'<a href="\1">\1</a>', body)
    toc = "".join(f'<li><a href="#{i}">{t}</a></li>' for i, t in re.findall(r'<h2 id="([^"]+)">(.*?)</h2>', body))
    page = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>PALEOTONE Manual</title>
<meta name="description" content="The PALEOTONE user manual: how the prehistoric synth firmware for the M-VAVE FM-1 works, page by page.">
<style>{CSS}</style>
</head>
<body>
<header class="top"><div><b>PALEOTONE</b><a href="../webapp/emu/">Play it in your browser</a><a href="../webapp/installer/">Install</a><a href="../webapp/editor/">Editor</a><a href="https://github.com/fantasmoslam/dinobox-fm1">Source</a></div></header>
<div class="wrap">
<nav class="toc" aria-label="Contents"><ol>{toc}</ol></nav>
<main>
{body}
</main>
</div>
</body>
</html>
"""
    (out / "index.html").write_text(page, encoding="utf-8")
    print(f"manual: {out}/index.html, {len(list(img_dir.iterdir()))} pictures")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        raise SystemExit(__doc__)
    main(*sys.argv[1:5])
