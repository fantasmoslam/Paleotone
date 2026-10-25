#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Assemble installer, compatibility tool and browser instrument for a tested release.
Does not overwrite existing hardware companion URLs. Run test_site_release.mjs before publishing.
"""
import argparse,hashlib,json,re,shutil
from pathlib import Path
from make_site import strip_module,product_of
HERE=Path(__file__).resolve().parent
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('package','emulator','out','source','version'):ap.add_argument('--'+name,required=True)
    a=ap.parse_args();pkg=Path(a.package);emu=Path(a.emulator);out=Path(a.out)
    raw=pkg.read_bytes();product=product_of(raw)
    if product!='FM-1_900' or b'FELUCCA-LOADER-1' not in raw:raise SystemExit('Expected the tested FM-1 Felucca-loader package')
    if not re.fullmatch('[0-9a-f]{40}',a.source):raise SystemExit('Source must be a complete commit hash')
    layout=json.loads((emu/'remote_views.json').read_text())
    manifest={'version':a.version,'source':a.source,'product':product,'rendererId':layout['rendererId'],'schema':layout['schema'],
      'firmware':{'file':'firmware/paleotone-current.fwsc','sha256':hashlib.sha256(raw).hexdigest(),'size':len(raw)},
      'previousRenderers':[695244803,2964915361,2036782920],
      'previousPackages':['d9793366201d5c3d7af6e45343099e65f76adf12c6f4fcb22901e280c0ddff4d','000532c1cc17fcac1aba297356a5d78e865bfa22d8df00ede242022e41544462']}
    token=manifest['firmware']['sha256'][:12]
    inst=out/'webapp/installer';setup=out/'webapp/app-setup';destemu=out/'webapp/emu'
    for d in (inst,setup,out/'firmware',destemu):d.mkdir(parents=True,exist_ok=True)
    lib='\n'.join(strip_module((HERE/n).read_text(encoding='utf-8')) for n in ('fm1pkg.js','fm1ota.js','fm1backup.js'))
    meta={'version':a.version,'product':product,'pkg':'../../'+manifest['firmware']['file']+'?v='+token,'sha256':manifest['firmware']['sha256']}
    html=(HERE/'index_pkg.html').read_text(encoding='utf-8').replace('/*LIB*/',lib).replace('/*META*/',json.dumps(meta))
    (inst/'index.html').write_text(html,encoding='utf-8');shutil.copyfile(pkg,out/manifest['firmware']['file'])
    # Copy the complete emulator (including fonts, audio encoders and their licences).
    for required in ('index.html','worklet.js','felucca.wasm','remote_views.json','emu_io.js','audio_session.js','audio_export.js','audio_encode.js','audio_encode_worker.js'):
        if not (emu/required).is_file():raise SystemExit('Missing emulator asset: '+required)
    shutil.copytree(emu,destemu,dirs_exist_ok=True,ignore=shutil.ignore_patterns('native*'))
    for path in destemu.rglob('*'):
        if path.suffix not in ('.html','.js'):continue
        text=path.read_text(encoding='utf-8')
        # Version local module graphs as well as the AudioWorklet and firmware.
        text=re.sub(r'''(from\s+["']\./[^"'?]+\.js)(["'])''',r'\1?v='+token+r'\2',text)
        text=re.sub(r'''(fetch\(["']felucca\.wasm|addModule\(["']worklet\.js)(["'])''',r'\1?v='+token+r'\2',text)
        path.write_text(text,encoding='utf-8')
    for name in ('firmware_support.js','fm1ota.js','fm1pkg.js','remote_screen.js'):
        text=(HERE/name).read_text(encoding='utf-8')
        text=re.sub(r'''(from\s+["']\./[^"'?]+\.js)(["'])''',r'\1?v='+token+r'\2',text)
        (setup/name).write_text(text,encoding='utf-8')
    text=(HERE/'app-setup.html').read_text(encoding='utf-8').replace("'./firmware_support.js'","'./firmware_support.js?v="+token+"'").replace("fetch('../../release.json')","fetch('../../release.json?v="+token+"')")
    (setup/'index.html').write_text(text,encoding='utf-8')
    (out/'index.html').write_text((HERE/'home.html').read_text(encoding='utf-8'),encoding='utf-8')
    (out/'release.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    editor=out/'webapp/editor';editor.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(HERE/'editor.html',editor/'index.html')
    for name in ('fukiai.ttf','FUKIAI-LICENSE.txt','fm1backup.js'):shutil.copyfile(HERE/name,editor/name)
    shutil.copytree(HERE/'manual',out/'manual',dirs_exist_ok=True)
    for doc in ('LICENSE','LICENSING.md'):shutil.copyfile(HERE.parent/doc,out/'firmware'/doc)
    shutil.copytree(HERE.parent/'LICENSES',out/'firmware/LICENSES',dirs_exist_ok=True)
    print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
