# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import subprocess,wave,base64
import numpy as np
import imageio_ffmpeg
root=Path(__file__).resolve().parents[1]
out=root/'build/paleotone-audio';out.mkdir(parents=True,exist_ok=True)
sections=[('Music alone: four tracks',10,1,1,0,0),
 ('Four tracks with integrated JUNGLE',32,1,1,1,110),
 ('Integrated JUNGLE encounter alone',32,1,0,1,110),
 ('Integrated WATER alone',24,0,0,1,0)]
parts=[];cues=[];at=0
for i,(title,secs,bio,music,on,danger) in enumerate(sections):
 p=out/f'section{i}.wav'
 r=subprocess.run([str(root/'build/host/paleotone_preview'),str(p),str(secs),str(bio),str(music),str(on),str(danger)],check=True,capture_output=True,text=True)
 with wave.open(str(p)) as f:x=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2).astype(float)
 n=len(x);fade=np.minimum(1,np.minimum(np.arange(n)/44100,(n-np.arange(n))/44100))
 parts.extend([x*fade[:,None],np.zeros((66150,2))])
 cues.append(f'{int(at)//60}:{int(at)%60:02d} {title}: {r.stdout.strip()}')
 at+=n/44100+1.5
y=np.concatenate(parts)
p=out/'paleotone-integrated.wav'
with wave.open(str(p),'wb') as f:
 f.setnchannels(2);f.setsampwidth(2);f.setframerate(44100);f.writeframes(y.astype('<i2').tobytes())
subprocess.run([imageio_ffmpeg.get_ffmpeg_exe(),'-y','-i',str(p),'-b:a','128k',str(out/'paleotone-integrated.mp3')],check=True)
(out/'cues.txt').write_text('\n'.join(cues)+'\n')
data=(out/'paleotone-integrated.mp3').read_bytes()
for i,at in enumerate(range(0,len(data),500000)):
 (out/f'audio-{i:02d}.b64').write_text(base64.b64encode(data[at:at+500000]).decode()+'\n')
print('\n'.join(cues))
