import sys, os, json
from pathlib import Path
ROOT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
PKG=ROOT.parent/'genre-analysis'/'packages';sys.path.insert(0,str(PKG))
import numpy as np, soundfile as sf
from scipy import signal
import imageio_ffmpeg,subprocess
sr=44100
x,actual=sf.read(ROOT/'stems.wav',dtype='float32',always_2d=True)
assert actual==sr and x.shape[1]==4 and len(x)==round((64*60/54+6)*sr)
assert np.isfinite(x).all()
t=np.arange(len(x))/sr; beat=t*54/60
# Match active stem RMS, preserving within-part dynamics rather than peak-normalizing each note.
target_db=np.array([-20.0,-18.5,-22.0,-24.0])
levels=[];scaled=[]
for k in range(4):
    y=x[:,k]; frame=np.sqrt(np.mean(y[:len(y)//2205*2205].reshape(-1,2205)**2,axis=1)); threshold=max(frame.max()*.10,1e-9)
    active=frame[frame>threshold]; measured=float(np.sqrt(np.mean(active**2)))
    g=float(10**(target_db[k]/20)/max(measured,1e-9));scaled.append(y*g);levels.append({'role':['bass','body','horn','drums'][k],'raw_active_rms_dbfs':round(20*np.log10(max(measured,1e-9)),2),'gain':round(g,4),'target_active_dbfs':float(target_db[k])})
x=np.stack(scaled,axis=1)
# Midpoint reduction is composed into score; further tame the remaining body there.
reduction=(beat>=32)&(beat<40);x[reduction,1]*=.35
# Damped desktop room audition, with bass kept dry. It is not an FM-1 CPU demonstration.
length=int(3.7*sr); tt=np.arange(length)/sr; irs=[]
for seed in [103,719]:
    rng=np.random.default_rng(seed); tail=rng.normal(size=length)*np.exp(-tt/1.05)
    tail=signal.sosfilt(signal.butter(2,[180,4200],btype='bandpass',fs=sr,output='sos'),tail)
    tail[:int(.045*sr)]=0;tail*=.45/max(np.sqrt(np.sum(tail**2)),1e-9)
    for sec,amp in [(.041,.13),(.067,.10),(.097,.075),(.133,.06)]:tail[int(sec*sr)]+=amp
    irs.append(tail)
dry=x.sum(axis=1)
send=x[:,1]*.34+x[:,2]*.25+x[:,3]*.11
stereo=np.stack([dry+signal.fftconvolve(send,ir,mode='full')[:len(dry)] for ir in irs],axis=1)
# Smooth the opening and final room tail; no auto-gain pumping.
fade=np.minimum(np.clip(t/1.1,0,1),np.clip((t[-1]-t)/4,0,1));stereo*=fade[:,None]
# Leave explicit headroom; modest static peak trim only.
peak=float(np.max(np.abs(stereo)));gain=min(1.0,10**(-2.0/20)/max(peak,1e-9));stereo*=gain
sf.write(ROOT/'T-Wrecks-Apocalyptic-Prototype-01.wav',stereo,sr,subtype='PCM_24')
# User-facing MP3 has exact same mastered mix.
ff=imageio_ffmpeg.get_ffmpeg_exe()
subprocess.run([ff,'-y','-v','error','-i',str(ROOT/'T-Wrecks-Apocalyptic-Prototype-01.wav'),'-codec:a','libmp3lame','-b:a','192k',str(ROOT/'T-Wrecks-Apocalyptic-Prototype-01.mp3')],check=True)
summary={'seconds':len(x)/sr,'bpm':54,'peak_dbfs':round(20*np.log10(float(np.max(np.abs(stereo)))),2),'rms_dbfs':round(20*np.log10(float(np.sqrt(np.mean(stereo**2)))),2),'static_master_gain':gain,'stems':levels,'sections_s':[0,16*60/54,32*60/54,40*60/54,56*60/54,64*60/54]}
(ROOT/'mix-summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
