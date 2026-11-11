import sys,json,csv
from pathlib import Path
ROOT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent/'genre-analysis'/'packages'))
import numpy as np,soundfile as sf
from scipy import signal
import imageio_ffmpeg,subprocess
x,sr=sf.read(ROOT/'stems.wav',dtype='float32',always_2d=True)
assert sr==44100 and x.shape[1]==4 and len(x)==3400600 and np.isfinite(x).all()
# Shape separate frequency roles; bass stays centered and dry.
for k,lo,hi in [(0,20,150),(1,140,2200),(3,45,5000)]:
    x[:,k]=signal.sosfilt(signal.butter(2,[lo,hi],btype='bandpass',fs=sr,output='sos'),x[:,k])
target_db=[-21,-25,None,-18.5];levels=[]
for k,db in enumerate(target_db):
    if db is None:x[:,k]=0;levels.append({'role':'lead','muted':True});continue
    y=x[:,k];f=np.sqrt(np.mean(y[:len(y)//2205*2205].reshape(-1,2205)**2,axis=1));act=f[f>max(f.max()*.1,1e-9)]
    rms=float(np.sqrt(np.mean(act**2)));g=float(10**(db/20)/max(rms,1e-9));x[:,k]*=g
    levels.append({'role':['bass','body','lead','drums'][k],'target_dbfs':db,'gain':g})
t=np.arange(len(x))/sr;beat=t*54/60
# The pulse continues in the reduction, leaving time unambiguous for a player.
x[(beat>=32)&(beat<40),1]*=.5
# Small transient clearance in the backing pad, not in the deep bass.
events=list(csv.DictReader((ROOT/'events.csv').open()))
for e in events:
    if int(e['role'])<3:continue
    s=int(round(float(e['time_s'])*sr));n=min(int(.16*sr),len(x)-s)
    x[s:s+n,1]*=1-.2*np.exp(-np.arange(n)/(.055*sr))
# Much shorter and quieter room, no distortion stage.
length=int(1.05*sr);tt=np.arange(length)/sr;irs=[]
for seed in [103,719]:
    ir=np.random.default_rng(seed).normal(size=length)*np.exp(-tt/.28)
    ir=signal.sosfilt(signal.butter(2,[250,3800],btype='bandpass',fs=sr,output='sos'),ir)
    ir[:int(.03*sr)]=0;ir*=.25/max(np.sqrt(np.sum(ir**2)),1e-9)
    ir[int(.038*sr)]+=.07;ir[int(.063*sr)]+=.05;irs.append(ir)
dry=x.sum(axis=1);send=x[:,1]*.12+x[:,3]*.045
stereo=np.stack([dry+signal.fftconvolve(send,ir,mode='full')[:len(dry)] for ir in irs],axis=1)
fade=np.minimum(np.clip(t/.15,0,1),np.clip((t[-1]-t)/2.5,0,1));stereo*=fade[:,None]
pk=float(np.max(np.abs(stereo)));gain=10**(-2/20)/max(pk,1e-9);stereo*=gain
assert np.isfinite(stereo).all() and np.max(np.abs(stereo))<1
name='T-Wrecks-Apocalyptic-Prototype-02'
sf.write(ROOT/(name+'.wav'),stereo,sr,subtype='PCM_24')
ff=imageio_ffmpeg.get_ffmpeg_exe();subprocess.run([ff,'-y','-v','error','-i',str(ROOT/(name+'.wav')),'-codec:a','libmp3lame','-b:a','192k',str(ROOT/(name+'.mp3'))],check=True)
b=subprocess.check_output([ff,'-v','error','-i',str(ROOT/(name+'.mp3')),'-f','f32le','-ac','2','-ar',str(sr),'-']);mp=np.frombuffer(b,dtype='<f4')
assert np.isfinite(mp).all() and np.max(np.abs(mp))<1
summary={'seconds':len(x)/sr,'bpm':54,'melodic_lead_events':sum(int(e['role'])==2 for e in events),'drum_events':sum(int(e['role'])>=3 for e in events),'bass_events':sum(int(e['role'])==0 for e in events),'peak_dbfs':round(float(20*np.log10(np.max(np.abs(stereo)))),2),'rms_dbfs':round(float(20*np.log10(np.sqrt(np.mean(stereo**2)))),2),'decoded_mp3_peak':float(np.max(np.abs(mp))),'stems':levels,'master_static_gain':gain,'room_seconds':1.05,'drive':'Organ drive/click/rotary zero; clean Sub Bass; no mix saturation'}
(ROOT/'mix-summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
