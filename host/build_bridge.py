#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build this project's USB SMK bridge from pinned, attributed board/protocol sources."""
import argparse, hashlib, json, os, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PIN='ed06aa502c7b75a3414c9c1976d0a5abd4350f57'
def run(*a,cwd=None,env=None): subprocess.run(list(map(str,a)),cwd=cwd,env=env,check=True)
def prepare(r):
    if not r.exists():
        run('git','clone','--filter=blob:none','--no-checkout','https://github.com/Quixotic7/RemoteFM1.git',r)
        run('git','checkout','--detach',PIN,cwd=r)
    assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()==PIN
    run('git','submodule','update','--init','--depth','1','base',cwd=r)
    run('git','submodule','update','--init','--recursive','--depth','1',cwd=r/'base')
    shutil.copy2(ROOT/'host/bridge/instrument-host.patch',r/'firmware/patches/smk37/app/0002-instrument-host.patch')
    shutil.copy2(ROOT/'host/bridge/fader-state.patch',r/'firmware/patches/smk37/fm1-base/0003-fader-state.patch')

def main():
    p=argparse.ArgumentParser();p.add_argument('--remote',type=Path,required=True);p.add_argument('--host',action='store_true');p.add_argument('--models',nargs='+',default=['pro','elite','mke'],choices=['pro','elite','mke']);a=p.parse_args();r=a.remote.resolve();prepare(r)
    out=ROOT/'build/instrument-host/controller';out.mkdir(parents=True,exist_ok=True)
    records=[]
    for model in a.models:
        env={**os.environ,'BOARD':'smk37','MODEL':model}
        run('bash','build.sh',*(['--host'] if a.host else ['--no-ble','--release','0.2.0']),cwd=r,env=env)
        suffix='' if model=='pro' else '-'+model
        if a.host:
            binary=r/f'firmware/build/host/remotefm1-smk37{suffix}_host'
            scenarios=[]
            # Retain the upstream standalone key, pad, knob and ownership checks.
            for name in ['smk_panel','smk_extras']:
                original=(r/f'firmware/tests/{name}.txt').read_text()
                dest=out/f'{name}-standalone.txt'
                standalone=original.split('rf hello usb',1)[0]
                # No Yamaha ROM assets are distributed: this build has only INIT VOICE.
                standalone=standalone.replace('expect sa_voice 2','expect sa_voice 0')
                dest.write_text(standalone)
                scenarios.append(dest)
            scenarios += [ROOT/'host/bridge/no-data-writes.txt',ROOT/'host/bridge/session-controls.txt',ROOT/'host/bridge/keys-lights.txt']
            for scenario in scenarios:
                result=subprocess.run([str(binary),str(scenario),str(out)],cwd=r,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
                (out/f'{model}-{scenario.stem}.log').write_text(result.stdout)
                if result.returncode or 'FAIL' in result.stdout:
                    print(result.stdout);raise SystemExit(f'SMK simulator failed: {model} {scenario.name}')
                print(f'Passed {model}: {scenario.name}',flush=True)
            continue
        src=r/f'firmware/build/remotefm1-smk37{suffix}-usb-0.2.0.fwsc'
        dest=out/f'Instrument-Host-SMK37-{model}-0.2.0.fwsc';shutil.copy2(src,dest)
        records.append({'model':model,'file':dest.name,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'bytes':dest.stat().st_size})
    if records:
        (out/'BUILD.json').write_text(json.dumps({'source':PIN,'version':'0.2.0','transport':'USB MIDI','capability':11,'data_writes':False,'hardware_qualified':False,'packages':records},indent=2)+'\n')
        shutil.copy2(ROOT/'docs/android-host-smk-setup.md',out/'SETUP.md')
        shutil.copy2(r/'LICENSE',out/'BRIDGE-LICENSE.txt')
        shutil.copy2(r/'LICENSING.md',out/'UPSTREAM-LICENSING.md')
        shutil.copytree(r/'LICENSES',out/'LICENSES',dirs_exist_ok=True)
        shutil.copy2(r/'tools/fm1_install.py',out/'fm1_install.py')
        # That model-aware installer is kept with all its Python dependencies.
        shutil.copytree(r/'tools',out/'tools',ignore=shutil.ignore_patterns('__pycache__'),dirs_exist_ok=True)
        print(out)
if __name__=='__main__':main()
