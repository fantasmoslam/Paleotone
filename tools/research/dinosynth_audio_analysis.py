import os, sys
from pathlib import Path
ROOT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'packages'))
os.environ['NUMBA_NUM_THREADS']='2'
os.environ['OMP_NUM_THREADS']='2'
import json, subprocess, hashlib, time
import numpy as np
from scipy import signal, ndimage
import imageio_ffmpeg
import librosa
SR=22050; HOP=512; NFFT=4096
FF=imageio_ffmpeg.get_ffmpeg_exe()
def decode(p):
    b=subprocess.check_output([FF,'-v','error','-i',str(p),'-f','f32le','-ac','1','-ar',str(SR),'-'])
    return np.frombuffer(b,dtype='<f4').copy()
def peaks_tempo(env):
    z=env-np.mean(env); ac=signal.fftconvolve(z,z[::-1],mode='full')[len(z)-1:]; ac/=max(float(ac[0]),1e-20)
    lo=int(60*SR/HOP/180); hi=min(len(ac)-1,int(60*SR/HOP/30))
    pk,_=signal.find_peaks(ac[lo:hi]); pk=pk+lo
    pk=sorted(pk,key=lambda k:ac[k],reverse=True)[:4]
    return [{'bpm':round(60*SR/HOP/int(k),1),'correlation':round(float(ac[k]),3)} for k in pk]
def analyze(meta,p):
    y=decode(p); expected=meta.get('duration',120); assert abs(len(y)/SR-expected)<.2, (meta['id'],len(y)/SR,expected); D=librosa.stft(y,n_fft=NFFT,hop_length=HOP); P=np.abs(D)**2
    fr=librosa.fft_frequencies(sr=SR,n_fft=NFFT); ps=P.mean(axis=1); valid=(fr>=20)&(fr<10000); total=float(ps[valid].sum())
    bands={name:round(100*float(ps[(fr>=lo)&(fr<hi)].sum())/max(total,1e-30),2) for name,lo,hi in [('20_60',20,60),('60_120',60,120),('120_250',120,250),('250_1000',250,1000),('1000_10000',1000,10000)]}
    centroid=float((ps[valid]*fr[valid]).sum()/max(total,1e-30))
    all_energy=P[valid].sum(axis=0); low=P[(fr>=20)&(fr<120)].sum(axis=0)
    rms=np.sqrt(np.mean(librosa.util.frame(y,frame_length=2048,hop_length=HOP)**2,axis=0))
    active=rms[rms>max(float(rms.max())*.01,1e-9)]
    dyn=20*np.log10(max(float(np.percentile(active,95)),1e-12)/max(float(np.percentile(active,10)),1e-12))
    flux=np.maximum(np.diff(np.log1p(1000*P/max(float(P.max()),1e-20)),axis=1),0)
    env=flux[valid].mean(axis=0); lowenv=flux[(fr>=20)&(fr<250)].mean(axis=0)
    times=librosa.frames_to_time(np.arange(P.shape[1]),sr=SR,hop_length=HOP)
    chroma=librosa.feature.chroma_stft(S=P,sr=SR,n_fft=NFFT,hop_length=HOP,tuning=0)
    sec=np.arange(int(len(y)/SR))
    C=np.stack([chroma[:,(times>=s)&(times<s+1)].mean(axis=1) for s in sec],axis=1)
    C/=np.maximum(np.linalg.norm(C,axis=0),1e-9)
    novelty=np.zeros(len(sec))
    for t in range(6,len(sec)-6):
        a=C[:,t-6:t].mean(axis=1); b=C[:,t:t+6].mean(axis=1); novelty[t]=1-float(a@b/max(np.linalg.norm(a)*np.linalg.norm(b),1e-9))
    peaks,_=signal.find_peaks(novelty,distance=15,prominence=.015)
    boundaries=sorted(sorted(peaks,key=lambda k:novelty[k],reverse=True)[:7])
    # Recurrence after removing the average pitch-class profile, to reduce false drone matches.
    X=C-C.mean(axis=1,keepdims=True); X/=np.maximum(np.linalg.norm(X,axis=0),1e-8)
    rec=[(lag,float(np.mean(np.sum(X[:,lag:]*X[:,:-lag],axis=0)))) for lag in range(4,min(41,len(sec)//2))]
    rec=sorted(rec,key=lambda q:q[1],reverse=True)[:3]
    segments=[]
    for start in range(0,int(len(y)/SR),30):
        pick=(times>=start)&(times<start+30); pp=P[:,pick].mean(axis=1); tt=float(pp[valid].sum())
        segments.append({'start_s':start,'low_20_120_pct':round(100*float(pp[(fr>=20)&(fr<120)].sum())/max(tt,1e-30),2),'rms_dbfs':round(10*np.log10(max(float(np.mean(y[start*SR:min(len(y),(start+30)*SR)]**2)),1e-20)),1)})
    out={**meta,'analyzed_seconds':round(len(y)/SR,2),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bands_pct':bands,'power_centroid_hz':round(centroid),'active_rms_p95_p10_db':round(float(dyn),1),'low_energy_occupancy_pct':round(100*float(np.mean(low>.15*np.percentile(low,95))),1),'pulse_candidates':peaks_tempo(env),'low_pulse_candidates':peaks_tempo(lowenv),'chroma_change_candidates_s':[int(k) for k in boundaries],'recurrence_candidates_s':[{'lag_s':lag,'correlation':round(v,3)} for lag,v in rec],'segments':segments}
    return out
refs=json.loads((ROOT/'references.json').read_text())
inputs=[(r,ROOT/(r['id']+'.mp3')) for r in refs]
for key in ['stampede','bass','extinction','drums','drone']:
    p=ROOT/('twrecks-'+key+'.wav')
    if p.exists(): inputs.append(({'id':'twrecks-'+key,'title':'T-WRECKS '+key,'source':'installed recipe 91dcc88','diagnostic':'Bass-only dry render' if key=='bass' else 'Full mix, manual calls off'},p))
results=[]
for meta,p in inputs:
    t=time.time(); out=analyze(meta,p);results.append(out)
    (ROOT/'analysis.json').write_text(json.dumps(results,indent=2))
    print(out['id'],out['title'],'bands',out['bands_pct'],'pulse',out['pulse_candidates'][:2],'seconds',round(time.time()-t,1),flush=True)
print('ANALYSIS_COMPLETE',len(results),flush=True)
