#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Wait for the exact candidate's target and host checks before publishing."""
import json, os, sys, time, urllib.request
repo=os.environ['GITHUB_REPOSITORY']; source=os.environ['GITHUB_SHA']
token=os.environ['GH_TOKEN']
def api(path):
    req=urllib.request.Request('https://api.github.com/repos/'+repo+'/'+path,
        headers={'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json'})
    with urllib.request.urlopen(req,timeout=30) as response:return json.load(response)
deadline=time.monotonic()+50*60
wanted={'build firmware','host tests'}
while time.monotonic()<deadline:
    head=api('git/ref/heads/feature/scape-garden')['object']['sha']
    if head!=source:sys.exit('A newer candidate exists; this preview will not be published.')
    runs=api('actions/runs?head_sha='+source+'&per_page=100')['workflow_runs']
    latest={name:max((r for r in runs if r['name']==name),key=lambda r:r['id'],default=None) for name in wanted}
    for name,run in latest.items():
        if run and run['status']=='completed' and run['conclusion']!='success':
            sys.exit(name+' did not pass; the current preview is preserved.')
    if all(r and r['status']=='completed' and r['conclusion']=='success' for r in latest.values()):
        with open(os.environ['GITHUB_OUTPUT'],'a') as output:
            output.write('firmware_run='+str(latest['build firmware']['id'])+'\n')
        print('Exact source passed firmware and full host checks: '+source)
        break
    print('Waiting for exact-source target and host checks.',flush=True)
    time.sleep(30)
else:sys.exit('Timed out waiting for validation; current preview is preserved.')
