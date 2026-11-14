"""Verify the local survey's references, source bytes and coverage accounting."""
import hashlib, json, pathlib
P = pathlib.Path(__file__).parent
def load(name): return json.loads((P/name).read_text())
refs = load('audit/final-source-references.json')
rows = load('deliverables/repository-inventory.json')
summary = load('deliverables/survey-summary.json')
statuses = load('audit/blob-status.json')
requested = {r['sha'] for r in refs}
assert len(rows) == len({r['repo'].lower() for r in rows}) == summary['repository_candidates']
assert len(refs) == summary['source_references']
assert requested == {r['sha'] for r in statuses}
assert len(requested) == summary['unique_requested_blobs']
for sha in requested:
    data = (P/'corpus/blobs'/sha).read_bytes()
    assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest() == sha, sha
assert summary['verified_requested_blobs'] == len(requested)
assert summary['unavailable_blobs'] == 0
assert sum(summary['roles'].values()) == len(rows)
assert len(load('deliverables/ancestry-evidence.json')) == len(rows)
assert len(load('deliverables/identical-source-groups.json')) == summary['identical_source_groups']
result = {'result':'passed', 'repository_records':len(rows), 'source_references':len(refs),
    'unique_source_blobs_verified':len(requested), 'missing_or_corrupt':0,
    'scope':'Collection integrity and coverage accounting; no firmware execution or hardware tests.'}
(P/'deliverables/verification.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
