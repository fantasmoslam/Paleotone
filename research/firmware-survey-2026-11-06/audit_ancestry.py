"""Add conservative, reproducible source-sharing evidence to every candidate."""
import collections, json, pathlib, re
P = pathlib.Path(__file__).parent
def read(path): return json.loads(path.read_text())
trees = {r['repo'].lower(): r for f in (P/'audit/trees').glob('*.json') for r in read(f)}
rows = read(P/'deliverables/repository-inventory.json')
refs = collections.defaultdict(list)
for r in read(P/'audit/final-source-references.json'):
    refs[r.get('variant', r['repo']).lower()].append(r)
upstreams = ['hugelton/felucca', 'hugelton/felucca-engines', 'hugelton/felucca-bsp', 'charlesvestal/fm1-base']
indices = {}
for name in upstreams:
    index = collections.defaultdict(list)
    for r in trees[name]['paths']:
        path = r['path']
        if path.endswith(('.c','.h','.cpp','.hpp','.cc')) and not re.search(r'(^|/)(tests?|docs|tools|scripts|web|examples)/', path):
            index[r['sha']].append(path)
    indices[name] = index
out = []
for row in rows:
    key = row['repo'].lower(); tree = trees.get(key, {})
    sharing = []
    for upstream, index in indices.items():
        if key == upstream: continue
        matches = [{'path': f['path'], 'sha': f['sha'], 'upstream_paths': index[f['sha']]} for f in tree.get('paths',[]) if f['sha'] in index and f['path'].endswith(('.c','.h','.cpp','.hpp','.cc'))]
        if matches: sharing.append({'upstream': upstream, 'upstream_commit': trees[upstream]['sha'], 'matches': matches})
    declarations = []
    for r in refs[key]:
        if r['path'] != row['readme_source']: continue
        path = P/'corpus/blobs'/r['sha']
        if not path.exists(): continue
        for line_no, line in enumerate(path.read_text(errors='replace').splitlines(), 1):
            if re.search(r'Felucca|fm1-base|felucca-bsp', line, re.I) and re.search(r'based|fork|deriv|engine|platform|built|port|credit|origin', line, re.I):
                declarations.append({'path':r['path'], 'sha':r['sha'], 'line':line_no, 'text':line.strip()[:450]})
            if len(declarations) == 8: break
    evidence = {'repo':row['repo'], 'snapshot_commit':row['snapshot_commit'], 'exact_source_sharing':sharing, 'contextual_readme_lines':declarations,
        'interpretation':'Exact file equality demonstrates source sharing at these snapshots. It does not prove direction of copying, direct parentage, whole-engine reuse, or licence compatibility. Missing matches do not exclude older or modified ancestry.'}
    out.append(evidence)
(P/'deliverables/ancestry-evidence.json').write_text(json.dumps(out,indent=2))
print('Ancestry records', len(out), 'with source sharing',sum(bool(x['exact_source_sharing']) for x in out),'with contextual lines',sum(bool(x['contextual_readme_lines']) for x in out))
