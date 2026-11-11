import sys, json, csv, collections
from pathlib import Path
ROOT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'packages'))
import numpy as np, subprocess, imageio_ffmpeg
FF=imageio_ffmpeg.get_ffmpeg_exe(); out=[]
for p in sorted(ROOT.glob('tone-*.wav')):
    v,n=map(int,p.stem.split('-')[1:]); sr=22050
    b=subprocess.check_output([FF,'-v','error','-i',str(p),'-f','f32le','-ac','1','-ar',str(sr),'-']); y=np.frombuffer(b,dtype='<f4')[int(.25*sr):int(3.25*sr)]
    P=np.abs(np.fft.rfft(y*np.hanning(len(y))))**2; f=np.fft.rfftfreq(len(y),1/sr); total=P[(f>=20)&(f<10000)].sum(); fund=440*2**((n-69)/12)
    candidates=np.flatnonzero((f>=20)&(f<1000)); ix=candidates[np.argmax(P[candidates])]
    o={'voice':v,'midi_note':n,'commanded_fundamental_hz':round(fund,2),'strongest_component_hz':round(float(f[ix]),2),'fundamental_band_pct':round(100*float(P[np.abs(f-fund)<max(1,fund*.03)].sum()/max(total,1e-30)),2),'20_60_pct':round(100*float(P[(f>=20)&(f<60)].sum()/max(total,1e-30)),2)};out.append(o)
(ROOT/'tone-analysis.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
rows=list(csv.DictReader((ROOT/'bass-events.csv').open()))
notes=[int(r['note']) for r in rows if int(r['channel'])==4]
times=[float(r['time_s']) for r in rows if int(r['channel'])==4]
print('Bass events',len(notes),'notes',dict(sorted(collections.Counter(notes).items())),'onset_spacing_median',float(np.median(np.diff(times))) if len(times)>1 else None)
(ROOT/'bass-event-summary.json').write_text(json.dumps({'count':len(notes),'note_histogram':dict(sorted(collections.Counter(notes).items())),'median_onset_spacing_s':float(np.median(np.diff(times)))},indent=2))
