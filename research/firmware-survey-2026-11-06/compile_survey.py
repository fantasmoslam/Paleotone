import json,pathlib,re,hashlib,collections
P=pathlib.Path(__file__).parent
def load(p):return json.loads(p.read_text())
def dump(p,x):p.write_text(json.dumps(x,indent=2,ensure_ascii=False))
trees={r['repo'].lower():r for f in (P/'audit/trees').glob('*.json') for r in load(f)}
refs=load(P/'audit/final-source-references.json')
byrepo=collections.defaultdict(list)
for r in refs:byrepo[r.get('variant',r['repo']).lower()].append(r)
oldread={r['repo'].lower():r for f in (P/'audit/files').glob('readmes*.json') for r in load(f)}
def content(r):
 f=P/'corpus/blobs'/r['sha']
 return f.read_text(errors='replace') if f.exists() else ''
foundation={'hugelton/felucca-bsp','hugelton/felucca-engines','2bbb/felucca-engines','charlesvestal/fm1-base','konsumer/foma','genkigenki/fm1-to-smk37','w0ts/optimist'}
hosts={'tonalpak/fm1-deck','holidayman/fm1-android','thingsapart/fm1-studio','joakimeriksson/fm1-emu','quixotic7/remotefm1','quixotic7/remotefm1fw','quixotic7/remotefm1android','quixotic7/remotefm1ios','charlesvestal/fm1-ctl'}
manual_games={'adamcharleswilsonmusic-code/trumpagotchi','adamcharleswilsonmusic-code/rambeaver'}
deep={'hugelton/felucca','hugelton/felucca-engines','hugelton/felucca-bsp','charlesvestal/fm1-amb','rastapopolix/tessera','fartypants/fm1-nachtfahrt','aceyprime/zen-1','reytian/fm1-min1','delirioux/tuna','davidpryan/haunted-fm1','jasonpersinger/ghoulbox-fm1-dungeon-synth','ruppert/fmini-1','ruppert/organvave-1','keremimo/melodee','charlesvestal/fm1-x0x','charlesvestal/fm1-fomni','charlesvestal/fm1-loom','zednaked/jangada','tonalpak/fm1-deck','thingsapart/fm1-studio','quixotic7/remotefm1','genkigenki/fm1-to-smk37','fantasmoslam/paleotone','fantasmoslam/fmini-1','isod89/sloop-fm1'}
patterns={
 'generative':r'\bgenerativ\w*|self[- ]playing|composes as it plays',
 'seeded':r'\bseed\w*|deterministic',
 'euclidean':r'\beuclid\w*|\bbjorklund\b',
 'markov':r'\bmarkov\b',
 'shift_register':r'\bturing\b|shift register',
 'long_form':r'long[- ]form|section machine|arrangement|arranger|conductor|song form|sections form|musical sections|section changes',
 'physical_model':r'physical\w* modell?\w*|karplus|\bplaits\b|\bbraids\b|\brings\b',
 'granular':r'\bgranul\w*',
 'sampling':r'\bsampler\b|\bsampling\b|sample (?:playback|player|bank|slots|library)|\bbreakbeat\b|\bslic(?:e|er|ing)\b',
 'fm_synthesis':r'\bFM[46]\b|\bdexed\b|\bDX[- ]?7\b|frequency modulation|(?:four|six|[2468])[- ](?:op|operator)|\bFM (?:synth|engine|voice|pair)',
 'analog_model':r'virtual analog|analogue|analog[- ]style|\b303\b|\bjuno\b|\bminimoog\b|\bprophet\b',
 'expression':r'\bMPE\b|aftertouch|\bpressure\b|\bvelocity\b',
 'usb_audio':r'USB[- ]audio',
 'smk_mention':r'\bSMK[- ]?37\b|MKE[- ]?P37',
 'android_mention':r'\bandroid\b',
 'host_preview':r'\bWebAssembly\b|\bWASM\b|browser.*(?:emulat|simulat|play)|host simulator',
 'multicore_mention':r'dual[- ]core|second core|core[- ]?1|multicore',
}
base=trees.get('hugelton/felucca',{})
baseblobs={p['sha'] for p in base.get('paths',[]) if re.search(r'/(voice|eng_\w+|engines|core)\.(c|h)$',p['path'])}
catalog=load(P/'candidates.json');rows=[]
for c in catalog:
 key=c['repo'].lower();t=trees.get(key,{});rr=byrepo[key];docs=[r for r in rr if re.search(r'(^|/)README[^/]*$',r['path'],re.I) and content(r)]
 docs.sort(key=lambda r:(r['path'].count('/'),len(r['path'])))
 rd=content(docs[0]) if docs else oldread.get(key,{}).get('content','')
 desc=c.get('description','') or '';alltext=desc+'\n'+rd;paths=[p['path'] for p in t.get('paths',[])]
 source=[r for r in rr if r['path'].endswith(('.c','.cpp','.cc','.rs','.py','.kt','.java','.js','.ts')) and content(r)]
 production=[r for r in source if not re.search(r'(^|/)(tests?|tools|scripts|web)/',r['path'])]
 errs=str(t.get('error',''))
 music_layout=any(re.search(r'(^|/)(firmware|app|device|src|deepfield)/.*\.(c|cpp|rs)$',s) for s in paths)
 related=bool(re.search(r'\bFM[- ]?1\b|\bM[- ]?VAVE\b|\bSMK[- ]?37\b|\bJieLi\b|\bsloop\b',alltext,re.I)) or music_layout or 'smk-hub-current.html' in c['discovered_in']
 name=key.split('/')[-1]
 if key=='topics/m-vave':role='excluded: GitHub topic URL, not a repository'
 elif errs:role='unavailable or empty repository'
 elif not related:role='adjacent or unresolved relevance'
 elif key in foundation or name in ['felucca-bsp','felucca-engines','fm1-base','fm1-to-smk37']:role='platform, engine library or porting foundation'
 elif key in hosts or re.search(r'emu(?:lator)?|companion|android|backup|recovery|unbrick|flasher|loader|flipper|firmware[-_](?:hub|collection|database)|dev-wiki|guide|atlas|awesome|editor|preset|toolchain|protocol|soundscape|custom-fw$|^fm-1-re$|^fm1vst$|mirror|reader|firmwarehost|switch',name):role='host, tool, documentation or data'
 elif key in manual_games or re.search(r'\bdoom\b|game boy emulator|virtual pet|gameboy|\bPICO-8\b|spelling game|je[u]? d.orthographe',alltext[:5000],re.I):role='game or non-music application'
 elif music_layout:role='firmware or musical application candidate'
 else:role='resource or source-light candidate'
 shared=sorted({p['path'] for p in t.get('paths',[]) if p['sha'] in baseblobs})
 if len(shared)>=2:lineage='shared Felucca synthesis-core blobs'
 elif c.get('github_fork_of'):lineage='catalog-reported fork: '+c['github_fork_of']
 elif c.get('catalog_recipe') in ['felucca','felucca-sub']:lineage='catalog Felucca-family build recipe'
 elif re.search(r'fm1-base|felucca-bsp',rd,re.I):lineage='platform ancestry declared; synthesis ancestry not established'
 elif re.search(r'Felucca',rd,re.I):lineage='Felucca mentioned; derivation requires contextual check'
 else:lineage='not established in inspected evidence'
 evidence={}
 for tag,pat in patterns.items():
  hits=[{'line':i+1,'text':l.strip()[:280]} for i,l in enumerate(rd.splitlines()) if re.search(pat,l,re.I)]
  if hits:evidence[tag]=hits[:3]
  elif re.search(pat,desc,re.I):evidence[tag]=[{'line':None,'text':desc[:280],'source':'catalog description'}]
 # Equality here is equality of indexed native source path/blob pairs, not musical equivalence.
 natives=sorted((x['path'],x['sha']) for x in t.get('paths',[]) if x['path'].endswith(('.c','.h','.cpp','.hpp','.cc','.rs')) and not re.search(r'(^|/)(tests?|tools|scripts|web|docs|build)/',x['path']))
 fp=hashlib.sha256(json.dumps(natives).encode()).hexdigest() if natives else None
 spdx=[]
 for r in rr:
  for license in re.findall(r'SPDX-License-Identifier:[ \t]*([A-Za-z0-9.+()-]+(?:[ \t]+(?:AND|OR|WITH)[ \t]+[A-Za-z0-9.+()-]+)*)',content(r)[:1200]):
   if license.strip() not in spdx:spdx.append(license.strip())
 licensefiles=[{'path':r['path'],'sha':r['sha']} for r in rr if re.search(r'(^|/)(LICENSE|LICENSING|COPYING)(\.|$)',r['path'],re.I) and content(r)]
 row={'repo':c['repo'],'url':'https://github.com/'+c['repo'],'snapshot_commit':t.get('sha'),'discovered_in':c['discovered_in'],'role':role,'lineage_evidence':lineage,'shared_felucca_core_paths':shared,'description_from_catalog':desc,'readme_source':docs[0]['path'] if docs else oldread.get(key,{}).get('path'),'readme_evidence':evidence,'feature_tags_are':'documentation mentions, not independently verified behavior','source_screening':'collected and hash verified' if production else ('support code collected' if source else 'no representative implementation collected'),'collected_source_files':len(source),'production_source_files':len(production),'source_paths':[r['path'] for r in production],'source_fingerprint':fp,'catalog_license':c.get('catalog_license'),'observed_spdx':spdx,'license_files':licensefiles,'fetch_error':t.get('error'),'report_focus':key in deep,'hardware_tested_in_this_survey':False}
 rows.append(row)
groups=collections.defaultdict(list)
for r in rows:
 if r['source_fingerprint']:groups[r['source_fingerprint']].append(r['repo'])
dup=[{'fingerprint':k,'repositories':v,'meaning':'identical indexed production native source path/blob pairs'} for k,v in groups.items() if len(v)>1]
for r in rows:
 g=groups.get(r['source_fingerprint'],[]);r['identical_indexed_source_repositories']=g if len(g)>1 else []
statuses=load(P/'audit/blob-status.json')
summary={'date':'2026-11-06','timezone':'America/Phoenix','repository_candidates':len(rows),'branch_variants':len([k for k in trees if '@' in k]),'roles':dict(collections.Counter(r['role'] for r in rows)),'lineage':dict(collections.Counter(r['lineage_evidence'].split(':')[0] for r in rows)),'trees_available':sum(not r['fetch_error'] for r in rows),'representative_source_collected':sum(r['production_source_files']>0 for r in rows),'documentation_with_feature_evidence':sum(bool(r['readme_evidence']) for r in rows),'source_references':len(refs),'unique_requested_blobs':len(statuses),'verified_requested_blobs':sum(x['status']!='unavailable' for x in statuses),'unavailable_blobs':sum(x['status']=='unavailable' for x in statuses),'identical_source_groups':len(dup),'repositories_in_identical_source_groups':sum(len(x['repositories']) for x in dup),'unique_indexed_source_sets':len(groups),'source_tree_equivalence_is_not_distinct_firmware_count':True,'feature_tag_counts':dict(collections.Counter(k for r in rows for k in r['readme_evidence']))}
out=P/'deliverables';out.mkdir(exist_ok=True)
dump(out/'survey-summary.json',summary);dump(out/'repository-inventory.json',rows);dump(out/'identical-source-groups.json',dup)
(out/'repository-inventory.jsonl').write_text(''.join(json.dumps(r,ensure_ascii=False)+'\n' for r in rows))
index=['# Repository screening register','', 'November 6, 2026. Every candidate receives a disposition. Feature tags record documentation mentions; code collection is not a hardware test. Source-tree equality applies only to indexed paths.','', '| Repository | Disposition | Ancestry evidence | Source files | Documented feature mentions |','|---|---|---|---:|---|']
for r in rows:index.append('| ['+r['repo']+']('+r['url']+') | '+r['role']+' | '+r['lineage_evidence']+' | '+str(r['production_source_files'])+' | '+', '.join(r['readme_evidence'])+' |')
(out/'Repository-Register.md').write_text('\n'.join(index)+'\n')
print(json.dumps(summary,indent=2))
