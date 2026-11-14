import json, re, pathlib, urllib.request, hashlib, concurrent.futures, time
P=pathlib.Path(__file__).parent
def read(p): return json.loads(p.read_text())
def dump(p,x): p.write_text(json.dumps(x,indent=2,ensure_ascii=False))
def get(url):
    req=urllib.request.Request(url,headers={'User-Agent':'FM1-source-survey','Accept':'application/vnd.github+json'})
    for attempt in range(3):
        try: return urllib.request.urlopen(req,timeout=25).read()
        except Exception:
            if attempt==2: raise
            time.sleep(.3)
def blob(b):return hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()
def discover():
    cs={r['repo'].lower():r for r in read(P/'candidates.json')}
    for file in ['resource-hub-current.md','smk-hub-current.html']:
        for repo in re.findall(r'https://github.com/([\w.-]+/[\w.-]+)',(P/'sources'/file).read_text()):
            repo=repo.rstrip('.').removesuffix('.git')
            if repo.startswith(('topics/','sponsors/')):continue
            r=cs.setdefault(repo.lower(),{'repo':repo,'discovered_in':[]})
            if file not in r['discovered_in']:r['discovered_in'].append(file)
    q=P/'sources/collection-current.json'
    if q.exists():
        d=read(q)
        for a in d.get('firmwares',[]):
            repo=a['repo'];r=cs.setdefault(repo.lower(),{'repo':repo,'discovered_in':[]})
            if 'collection-current' not in r['discovered_in']:r['discovered_in'].append('collection-current')
            r.update({k:v for k,v in {'catalog_head':a.get('head'),'catalog_recipe':a.get('recipe'),'catalog_license':a.get('license'),'description':a.get('description'),'github_fork_of':a.get('fork_of'),'branch':a.get('branch')}.items() if v is not None})
    ts={r['repo'].lower():r for f in (P/'audit/trees').glob('*.json') for r in read(f)}
    todo=[r for k,r in cs.items() if k not in ts]
    def tree(r):
        try:
            ref=r.get('catalog_head')
            if not ref:ref=json.loads(get('https://api.github.com/repos/'+r['repo']+'/commits/HEAD'))['sha']
            t=json.loads(get('https://api.github.com/repos/'+r['repo']+'/git/trees/'+ref+'?recursive=1'))
            return {'repo':r['repo'],'sha':ref,'truncated':t['truncated'],'total_files':sum(x['type']=='blob' for x in t['tree']),'paths':[{'path':x['path'],'sha':x['sha'],'size':x.get('size'),'type':x['type']} for x in t['tree'] if x['type'] in ['blob','commit']]}
        except Exception as e:return {'repo':r['repo'],'error':str(e)}
    out=[]
    with concurrent.futures.ThreadPoolExecutor(12) as ex:
        for t in ex.map(tree,todo):
            out.append(t);print('discovery',t['repo'],'error' if t.get('error') else 'ok',flush=True)
    dump(P/'audit/trees/current-additions.json',out)
    dump(P/'candidates.json',sorted(cs.values(),key=lambda r:r['repo'].lower()))
    print('discovered',len(cs),'new',len(todo),flush=True)
def collect():
    ts={r['repo'].lower():r for f in (P/'audit/trees').glob('*.json') for r in read(f)}
    req=[]
    def add(t,p,kind):req.append({'repo':t.get('repository',t['repo']),'variant':t['repo'],'ref':t['sha'],'path':p['path'],'sha':p['sha'],'size':p.get('size'),'kind':kind})
    for t in ts.values():
        if t.get('error'):continue
        ps=t.get('paths',[])
        for p in ps:
            if p['path']=='.gitmodules':add(t,p,'submodule_provenance')
        docs=[p for p in ps if re.search(r'(^|/)(README|LICENSING|LICENSE|COPYING)(\.[^/]+)?$',p['path'],re.I)]
        for p in sorted(docs,key=lambda p:(p['path'].count('/'),len(p['path'])))[:4]:add(t,p,'documentation')
        code=[p for p in ps if re.search(r'\.(c|cpp|cc|h|hpp|kt|java|rs|py|js|ts)$',p['path']) and not re.search(r'(^|/)(tests?|vendor|third_party|node_modules|build|dist|assets|fonts?|tools|scripts|samples?|presets?)/',p['path'],re.I)]
        selected={}
        for p in code:
            path=p['path']
            if re.search(r'(^|/)(eng_[\w]+|.*_app|.*_gen|.*_engine|gen|generat\w*|ambient|evolve|conductor|arranger|probability|euclid\w*|turing\w*|markov\w*|scene\w*|drone|synth|engine|sequencer|seq|main_fm1|app|felucca)\.(c|cpp|cc)$',path,re.I):selected[path]=(p,'engine_or_sequence')
            if re.search(r'(^|/)dsp/.*\.(c|cpp|cc)$',path) and not re.search(r'(^|/)(base|bsp|lib|libs|external)/',path):selected[path]=(p,'dsp_implementation')
            if re.search(r'(^|/)engine/.*\.(c|cpp|cc)$',path) and not re.search(r'(^|/)(base|bsp|lib|libs|external|shim)/',path):selected[path]=(p,'engine_implementation')
            if re.search(r'(^|/)(core|gen|seq|synth|.*dsp|.*engine|.*FirmwareApi|.*firmware)\.(h|hpp)$',path,re.I) and (p.get('size') or 0)<1000000:selected[path]=(p,'engine_interface_or_inline_dsp')
            if re.search(r'(^|/)(src|app)/[^/]+\.(c|cpp|cc)$',path) and not re.search(r'(^|/)(ui\w*|editor\w*|gfx|icons|lcd|libc|console|splash\w*|font\w*|up_\w+|.*presets.*|.*vbank.*)\.',path) and not re.search(r'(^|/)(base|bsp|lib|libs|external|hal|loader)/',path):selected.setdefault(path,(p,'application_implementation'))
        # A real application entry rather than a host test; all per-app engines retained.
        for p in sorted(code,key=lambda p:(p['path'].count('/'),len(p['path']))):
            if re.search(r'(^|/)(main|audio|voice|dsp)\.(c|cpp|cc)$',p['path']):
                selected.setdefault(p['path'],(p,'render_or_entry'))
                if sum(k=='render_or_entry' for _,k in selected.values())>=2:break
        if not selected:
            impl=[p for p in code if not p['path'].endswith(('.h','.hpp'))]
            for p in sorted(impl,key=lambda p:(p['path'].count('/'),len(p['path'])))[:2]:selected[p['path']]=(p,'representative_implementation')
        for p,kind in selected.values():add(t,p,kind)
    # Preserve previous source references, but refetch capped or corrupted content.
    for p in read(P/'audit/source-requests.json'):req.append(p)
    req=list({(r['repo'].lower(),r['ref'],r['path']):r for r in req}.values())
    dump(P/'audit/final-source-references.json',req)
    dest=P/'corpus/blobs';dest.mkdir(parents=True,exist_ok=True)
    for f in (P/'audit/files').glob('*.json'):
        d=read(f)
        if not isinstance(d,list):continue
        for r in d:
            if r.get('sha') and isinstance(r.get('content'),str):
                b=r['content'].encode()
                if blob(b)==r['sha']:(dest/r['sha']).write_bytes(b)
    unique={}
    for r in req:unique.setdefault(r['sha'],[]).append(r)
    def fetch(kv):
        sha,refs=kv;file=dest/sha
        if file.exists() and blob(file.read_bytes())==sha:return {'sha':sha,'bytes':file.stat().st_size,'status':'verified_cached'}
        errors=[]
        for r in refs[:3]:
            try:
                url='https://raw.githubusercontent.com/'+r['repo']+'/'+r['ref']+'/'+urllib.parse.quote(r['path'])
                b=get(url)
                if blob(b)!=sha:raise ValueError('git blob SHA mismatch')
                file.write_bytes(b);return {'sha':sha,'bytes':len(b),'status':'verified_download','source':url}
            except Exception as e:errors.append(str(e))
        return {'sha':sha,'status':'unavailable','errors':errors}
    result=[]
    with concurrent.futures.ThreadPoolExecutor(64) as ex:
        for r in ex.map(fetch,unique.items()):
            result.append(r)
            if len(result)%100==0:print('blobs',len(result),'/',len(unique),flush=True);dump(P/'audit/blob-status.json',result)
    dump(P/'audit/blob-status.json',result)
    print('source references',len(req),'unique blobs',len(unique),'verified',sum(r['status']!='unavailable' for r in result),'errors',sum(r['status']=='unavailable' for r in result),flush=True)
if __name__=='__main__':
    import sys
    if 'discover' in sys.argv:discover()
    if 'collect' in sys.argv:collect()
