#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage a self-contained SCAPE candidate without replacing the current release.
Usage: prepare_scape_preview.py EMULATOR PACKAGE OUT SOURCE
"""
import hashlib,json,re,shutil,sys
from pathlib import Path
from make_site import strip_module,product_of
from prepare_companion import prepare
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from remote_identity import render_id
HERE=Path(__file__).resolve().parent
emu,pkg,out=map(Path,sys.argv[1:4]);source=sys.argv[4]
layout=json.loads((emu/'remote_views.json').read_text(encoding="utf-8"));raw=pkg.read_bytes()
assert re.fullmatch('[0-9a-f]{40}',source)
assert product_of(raw)=='FM-1_900' and b'FELUCCA-LOADER-1' in raw
assert layout['rendererId']==render_id(),'Renderer must match this source tree'
sha=hashlib.sha256(raw).hexdigest();token=sha[:12]
shutil.copytree(emu,out,dirs_exist_ok=True,ignore=shutil.ignore_patterns('native*'))
shutil.copytree(HERE/'manual',out/'manual',dirs_exist_ok=True)
shutil.copyfile(HERE.parent/'docs/UNIFIED-SCAPE.md',out/'SCAPE-CONTROLS.md')
shutil.copyfile(pkg,out/'paleotone-scape.fwsc')
prepare(emu,out/'screen',source)
metadata={'source':source,'rendererId':layout['rendererId'],'schema':layout['schema'],'kind':'firmware-graphics-companion',
          'candidate':True,'firmware':{'file':'paleotone-scape.fwsc','sha256':sha,'size':len(raw)}}
(out/'build.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding="utf-8")
(out/'screen/build.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding="utf-8")
editor=out/'editor';editor.mkdir(exist_ok=True)
for name in ('editor.html','fukiai.ttf','FUKIAI-LICENSE.txt','fm1backup.js'):
    shutil.copyfile(HERE/name,editor/('index.html' if name=='editor.html' else name))
licenses=out/'firmware';licenses.mkdir(exist_ok=True)
for name in ('LICENSE','LICENSING.md'):shutil.copyfile(HERE.parent/name,licenses/name)
shutil.copytree(HERE.parent/'LICENSES',licenses/'LICENSES',dirs_exist_ok=True)
installer=out/'installer';installer.mkdir(exist_ok=True)
lib='\n'.join(strip_module((HERE/name).read_text(encoding="utf-8")) for name in ('fm1pkg.js','fm1ota.js','fm1backup.js'))
meta={'version':'SCAPE candidate','product':'FM-1_900','pkg':'../paleotone-scape.fwsc?v='+token,'sha256':sha}
html=(HERE/'index_pkg.html').read_text(encoding="utf-8").replace('/*LIB*/',lib).replace('/*META*/',json.dumps(meta))
html=html.replace('href="../../"','href="../"').replace('href="../app-setup/"','href="../screen/"').replace('href="../screen-unified/"','href="../screen/"').replace('href="../emu/"','href="../"').replace('href="../../manual/"','href="../manual/"').replace('href="../../firmware/','href="../firmware/').replace('href="../../firmwares/"','href="https://fantasmoslam.github.io/dinobox-fm1/firmwares/"')
html=html.replace('<body>','<body><p style="text-align:center">SCAPE candidate firmware — use the paired candidate display after installing.</p>',1)
html=html.replace('href="../firmware/LICENSES/"','href="https://github.com/fantasmoslam/dinobox-fm1/tree/'+source+'/LICENSES"')
(installer/'index.html').write_text(html,encoding="utf-8")
path=editor/'index.html';path.write_text(path.read_text(encoding="utf-8").replace('href="../emu/"','href="../"'),encoding="utf-8")
path=out/'manual/index.html';path.write_text(path.read_text(encoding="utf-8").replace('href="../webapp/emu/"','href="../"').replace('href="../webapp/installer/"','href="../installer/"').replace('href="../webapp/editor/"','href="../editor/"'),encoding="utf-8")
html=(out/'index.html').read_text(encoding="utf-8").replace('ghoulbox-emu-','paleotone-scape-preview-')
html=html.replace('href="../app-setup/"','href="screen/"').replace('Connect your FM-1 display','Candidate FM-1 display').replace('href="../installer/"','href="installer/"').replace('Install on your FM-1','Install SCAPE candidate').replace('href="../../manual/"','href="manual/"').replace('href="../editor/"','href="editor/"').replace('href="../../firmwares/"','href="https://fantasmoslam.github.io/dinobox-fm1/firmwares/"')
html=re.sub(r'<body([^>]*)>',r'<body\1><aside style="padding:10px;text-align:center;background:#122417">SCAPE candidate preview · Separate browser saves · <a href="manual/#scape-the-living-soundscape">New controls</a> · Your FM-1 changes only if you install.</aside>',html,count=1)
(out/'index.html').write_text(html,encoding="utf-8")
for path in out.rglob('*'):
    if path.suffix not in ('.html','.js'):continue
    text=path.read_text(encoding="utf-8")
    text=re.sub(r'''(from\s+["']\./[^"'?]+\.js)(["'])''',r'\1?v='+token+r'\2',text)
    text=re.sub(r'''(fetch\(["']felucca\.wasm|addModule\(["']worklet\.js)(["'])''',r'\1?v='+token+r'\2',text)
    path.write_text(text,encoding="utf-8")
print(json.dumps(metadata,indent=2))
