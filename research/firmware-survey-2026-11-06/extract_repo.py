"""Reconstruct the collected subset of a pinned repository, without network access."""
import argparse,hashlib,json,pathlib
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('repo',help='owner/repository, optionally owner/repository@branch')
p.add_argument('--ref',help='optional commit SHA when several snapshots are present')
p.add_argument('--out',required=True,type=pathlib.Path)
a=p.parse_args();root=pathlib.Path(__file__).parent
refs=json.loads((root/'audit/final-source-references.json').read_text())
chosen=[r for r in refs if r.get('variant',r['repo']).lower()==a.repo.lower() and (not a.ref or r['ref']==a.ref)]
commits={r['ref'] for r in chosen}
if len(commits)>1 and not a.ref:raise SystemExit('Multiple commits: use --ref '+', '.join(sorted(commits)))
if not chosen:raise SystemExit('No collected source references match that repository/ref.')
dest=a.out.resolve();dest.mkdir(parents=True,exist_ok=True);count=0;missing=[]
for r in chosen:
 source=root/'corpus/blobs'/r['sha']
 if not source.exists():missing.append(r['path']);continue
 data=source.read_bytes();sha=hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
 if sha!=r['sha']:raise SystemExit('Corrupt source blob: '+r['sha'])
 target=(dest/r['path']).resolve()
 if not target.is_relative_to(dest):raise SystemExit('Unsafe relative source path')
 target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data);count+=1
(dest/'SURVEY-PROVENANCE.json').write_text(json.dumps(chosen,indent=2))
print('Reconstructed',count,'collected files from',', '.join(commits))
if missing:print('Unavailable files:',', '.join(missing))
print('This is the surveyed subset, not a full build checkout. Original licences apply.')
