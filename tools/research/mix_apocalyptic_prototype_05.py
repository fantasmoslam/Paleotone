import sys,json,math,csv,subprocess
from pathlib import Path
ROOT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent/'genre-analysis'/'packages'))
import numpy as np,soundfile as sf
from scipy import signal
import imageio_ffmpeg
SR=44100;BEAT=60/54
bank=Path(sys.argv[2]).resolve() if len(sys.argv)>2 else ROOT.parent/'taiko-redesign'/'compact'
backing=Path(sys.argv[3]).resolve() if len(sys.argv)>3 else ROOT.parent/'apocalyptic-prototype-03'/'stems.wav'
ff=imageio_ffmpeg.get_ffmpeg_exe()
samples=[np.frombuffer(subprocess.check_output([ff,'-v','error','-i',str(bank/('taiko-'+str(i)+'.wav')),'-f','f32le','-ar','44100','-ac','1','-']),dtype='<f4') for i in range(4)]
x,sr=sf.read(backing,dtype='float32',always_2d=True)
assert sr==SR and len(x)==3400600
# Preserve the native bass and organ sound; remove every former toy/log/hide drum.
bass=signal.sosfilt(signal.butter(2,[20,150],btype='bandpass',fs=SR,output='sos'),x[:,0])*.5114512072643861
body=signal.sosfilt(signal.butter(2,[140,2200],btype='bandpass',fs=SR,output='sos'),x[:,1])*.34855171007986074
t=np.arange(len(x))/SR;beat=t/BEAT;body[(beat>=32)&(beat<40)]*=.5
# Expose the drum sound first, then let the sustained backing swell under it.
backing_in=np.clip((t-3.2)/1.3,0,1);bass*=backing_in;body*=backing_in
patterns=[[(0,'low',112),(.75,'mid',78),(1.5,'mid',91),(2,'low',104),(2.75,'mid',70),(3.5,'mid',91)],
 [(0,'low',114),(.5,'mid',78),(1,'mid',94),(1.5,'mid',76),(2.5,'low',109),(3,'mid',86),(3.5,'mid',96)],
 [(0,'low',112),(.75,'mid',81),(1.5,'low',102),(2,'mid',90),(2.5,'mid',79),(3.25,'mid',88),(3.75,'low',100)],
 [(0,'low',119),(.5,'mid',82),(1,'low',105),(1.5,'mid',82),(2,'low',115),(2.5,'mid',84),(3,'mid',95),(3.25,'mid',75),(3.5,'mid',91),(3.75,'mid',85)]]
rng=np.random.default_rng(51054);events=[]
for bar in range(16):
    pat=patterns[bar%4]
    if bar in [8,9]:pat=[(0,'low',91),(.75,'mid',69),(1.5,'mid',76),(2,'low',87),(3.25,'mid',73)]
    for off,family,vel in pat:
        gain=1.05 if bar>=10 else 1
        sec=(bar*4+off)*BEAT
        if off:sec+=float(rng.uniform(-.006,.006))
        events.append({'time_s':sec,'family':family,'velocity':min(127,round(vel*gain)),'pan':-.26 if family=='low' else .25,'ensemble':False})
        if off==0 or (family=='low' and vel>=110):
            events.append({'time_s':sec+.015,'family':'mid','velocity':round(vel*.72),'pan':.36,'ensemble':True})
events.sort(key=lambda e:e['time_s'])
drums=np.zeros((len(x),2));active=[];rr={'low':0,'mid':0};steals=0;peakpoly=0
for ev in events:
    st=round(ev['time_s']*SR);fam=ev['family'];i=(rr[fam]%2)+(0 if fam=='low' else 2);rr[fam]+=1
    y=samples[i].astype(np.float64).copy()
    # Two ensembles: a large low drum and a slightly tighter responding drum.
    rate=.94 if fam=='low' else 1.14
    y=np.interp(np.arange(0,len(y),rate),np.arange(len(y)),y)
    # Keep the natural strike attack; no blanket low-pass that would bury it under a lead.
    y=signal.sosfilt(signal.butter(2,6000,fs=SR,output='sos'),y)
    y*= (ev['velocity']/127)**1.35
    n=min(len(y),len(x)-st);y=y[:n];pan=ev['pan'];a=(pan+1)*math.pi/4;yy=np.stack([y*math.cos(a),y*math.sin(a)],axis=1)
    active=[v for v in active if v['end']>st]
    if len(active)>=6:
        v=min(active,key=lambda v:float(np.sum(v['y'][max(0,st-v['start']):]**2)))
        start=max(0,st-v['start']);old=v['y'][start:];nold=min(len(old),len(x)-st);fade=np.clip(1-np.arange(nold)/(.025*SR),0,1)
        drums[st:st+nold]-=old[:nold]*(1-fade[:,None]);active.remove(v);steals+=1
    drums[st:st+n]+=yy;active.append({'start':st,'end':st+n,'y':yy});peakpoly=max(peakpoly,len(active))
# One static ensemble gain; preserve the recording dynamics and accents.
f=np.sqrt(np.mean(np.mean(drums[:len(drums)//2205*2205].reshape(-1,2205,2)**2,axis=2),axis=1));a=f[f>f.max()*.12]
drum_gain=10**(-16.0/20)/float(np.sqrt(np.mean(a*a)));drums*=drum_gain
# Clear a small amount of competing organ energy on major strikes.
for e in events:
    if e['ensemble'] or e['velocity']<100:continue
    s=round(e['time_s']*SR);n=min(round(.19*SR),len(x)-s);body[s:s+n]*=1-.22*np.exp(-np.arange(n)/(.065*SR))
dry=(bass+body)[:,None]+drums
# Early reflections and a moderate percussion room; all sub-bass remains dry.
room_len=round(1.65*SR);rt=np.arange(room_len)/SR;wet=[]
for channel,seed in enumerate([205,509]):
    ir=np.random.default_rng(seed).normal(size=room_len)*np.exp(-rt/.43)
    ir=signal.sosfilt(signal.butter(2,[220,4800],btype='bandpass',fs=SR,output='sos'),ir);ir[:round(.035*SR)]=0;ir*=.38/max(np.sqrt(np.sum(ir*ir)),1e-9)
    for sec,amp in [(.041,.12),(.079,.085),(.119,.06)]:ir[round(sec*SR)]+=amp
    send=drums[:,channel]*.18+body*.10
    wet.append(signal.fftconvolve(send,ir,mode='full')[:len(x)])
y=dry+np.stack(wet,axis=1)
y*=np.minimum(np.clip(t/.012,0,1),np.clip((t[-1]-t)/2,0,1))[:,None]
master=10**(-1.5/20)/float(np.max(np.abs(y)));y*=master
assert np.isfinite(y).all() and np.max(np.abs(y))<1
name='T-Wrecks-Apocalyptic-Prototype-05'
sf.write(ROOT/'pre-master.wav',y,SR,subtype='FLOAT')
ff=imageio_ffmpeg.get_ffmpeg_exe()
# Linked stereo lookahead gain control preserves hits without a distortion/clipping stage.
subprocess.run([ff,'-y','-v','error','-i',str(ROOT/'pre-master.wav'),'-af','volume=2.5dB,alimiter=limit=0.841395:attack=5:release=100:level=false:latency=true','-c:a','pcm_s24le',str(ROOT/(name+'.wav'))],check=True)
y,check_sr=sf.read(ROOT/(name+'.wav'),always_2d=True);assert check_sr==SR and len(y)==3400600 and np.isfinite(y).all()

ff=imageio_ffmpeg.get_ffmpeg_exe();credit='Taiko: Subaqueous, SCC Taiko Drums by S. Christian Collins, Floe port. Adapted: mono, trim, resample, IMA ADPCM, sequencing and mix. Preview CC BY-SA 4.0. Sources and credits: https://github.com/floe-audio/Taiko-Drums'
subprocess.run([ff,'-y','-v','error','-i',str(ROOT/(name+'.wav')),'-codec:a','libmp3lame','-b:a','192k','-metadata','title=Apocalyptic Prototype 05 - Taiko Ensemble','-metadata','artist=T-WRECKS prototype; taiko samples Subaqueous / SCC / Floe','-metadata','comment='+credit,'-metadata','copyright=CC BY-SA 4.0; https://creativecommons.org/licenses/by-sa/4.0/',str(ROOT/(name+'.mp3'))],check=True)
b=subprocess.check_output([ff,'-v','error','-i',str(ROOT/(name+'.mp3')),'-f','f32le','-ac','2','-ar',str(SR),'-']);mp=np.frombuffer(b,dtype='<f4')
assert np.isfinite(mp).all() and np.max(np.abs(mp))<1
summary={'seconds':len(y)/SR,'bpm':54,'drum_note_events':len(events),'simultaneous_drum_limit':6,'peak_drum_voices':peakpoly,'tail_steals_25ms_fade':steals,'drum_gain':drum_gain,'master_gain':master,'peak_dbfs':20*np.log10(np.max(np.abs(y))),'rms_dbfs':20*np.log10(np.sqrt(np.mean(y*y))),'decoded_mp3_peak':float(np.max(np.abs(mp))),'bank_bytes':40312,'instrument_source':'Recorded taiko, compact 16 kHz mono IMA ADPCM, decoded for this desktop audition','license':'CC BY-SA 4.0','credit':credit,'intro_drums_exposed_s':3.2,'desktop_peak_control':'2.5 dB input, 5 ms lookahead, 100 ms release, -1.5 dBFS ceiling'}
(ROOT/'mix-summary.json').write_text(json.dumps(summary,indent=2));(ROOT/'events.json').write_text(json.dumps(events,indent=2));print(json.dumps(summary,indent=2))
