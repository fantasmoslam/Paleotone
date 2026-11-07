#!/usr/bin/env python3
"""Pack pre-tuned mono WAVs into bounded, loop-state-aware IMA ADPCM assets.

The codec convention matches Felucca's GPL-3.0 sampleio.py. No recordings ship
with this script. The caller supplies their own w-WORD-note.wav files.
"""
# SPDX-License-Identifier: GPL-3.0-only
import argparse,hashlib,json,math,pathlib,struct,wave,zlib
STEPS=[7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
ADJ=[-1,-1,-1,-1,2,4,6,8]
RATE=11025
PAYLOAD=81408
ENTRY=struct.Struct('<BBHIIIIhBBIII')
def encode(pcm,loop):
 pred=idx=0;codes=[];state=(0,0)
 for i,x in enumerate(pcm):
  if i==loop:state=(pred,idx)
  step=STEPS[idx];diff=x-pred;code=8 if diff<0 else 0;diff=abs(diff);change=step>>3
  for bit,part in [(4,step),(2,step>>1),(1,step>>2)]:
   if diff>=part:code|=bit;diff-=part;change+=part
  pred=max(-32768,min(32767,pred+(-change if code&8 else change)));idx=max(0,min(88,idx+ADJ[code&7]));codes.append(code)
 if len(codes)%2:codes.append(0)
 return bytes(codes[i]|codes[i+1]<<4 for i in range(0,len(codes),2)),state
def decode(data,n,pred=0,index=0,start=0):
 out=[]
 for i in range(start,start+n):
  code=(data[i//2]>>((i&1)*4))&15;step=STEPS[index];d=(step>>3)+((step>>2) if code&1 else 0)+((step>>1) if code&2 else 0)+(step if code&4 else 0)
  pred=max(-32768,min(32767,pred+(-d if code&8 else d)));index=max(0,min(88,index+ADJ[code&7]));out.append(pred)
 return out
def read_resample(file,rate=RATE):
 with wave.open(str(file),'rb') as w:
  assert w.getsampwidth()==2 and w.getnchannels()==1
  sr=w.getframerate();pcm=list(struct.unpack('<%dh'%w.getnframes(),w.readframes(w.getnframes())))
 if sr==rate:return pcm
 # Windowed-sinc resampling, generated offline. No dependency on numpy or scipy.
 out=[];ratio=sr/rate;cut=min(1,rate/sr)*.94;taps=24;kernels={}
 for i in range(int(len(pcm)/ratio)):
  pos=i*ratio;c=int(pos);frac=round((pos-c)*256)/256
  if frac not in kernels:
   weights=[]
   for k in range(-taps//2+1,taps//2+1):
    x=k-frac;v=cut if abs(x)<1e-12 else math.sin(math.pi*cut*x)/(math.pi*x);v*=.5+.5*math.cos(math.pi*x/(taps/2)) if abs(x)<taps/2 else 0;weights.append(v)
   total=sum(weights);kernels[frac]=[v/total for v in weights]
  out.append(round(sum(pcm[min(len(pcm)-1,max(0,c+k))]*v for k,v in zip(range(-taps//2+1,taps//2+1),kernels[frac]))))
 return [max(-32768,min(32767,x)) for x in out]
def choose_loop(pcm,note,word,rate=RATE):
 period=rate/(440*2**((note-69)/12));cycles=max(2,round(.055*rate/period));length=round(cycles*period)
 lo,hi=(int(.200*rate),int(.240*rate)) if word==0 else (int(.156*rate),int(.195*rate))
 best=None
 for st in range(lo,hi):
  en=st+length
  if en>=len(pcm)-int(.09*rate) or (word and en>int(.258*rate)):continue
  energy=sum(x*x for x in pcm[st:en])/length
  if energy<200**2:continue
  score=sum((pcm[st+k]-pcm[en+k])**2 for k in range(-8,8)) + 2*(pcm[st]-pcm[en])**2
  if best is None or score<best[0]:best=(score,st,en)
 if best is None:raise ValueError(f'No usable vowel loop: word={word}, note={note}')
 return best[1],best[2]
def fsmp_header(name,payload):
 n=len(payload)*2;zone=struct.pack('<5I2h4B',0,n,0,n-1,16384,60*16,0,0,0,127,0)
 return (struct.pack('<IHBB8sII8x',0x504d5346,1,1,0,name.encode().ljust(8,b'\0'),len(payload),zlib.crc32(payload))+zone).ljust(512,b'\0')
def build(folder,out):
 records=[];blob=bytearray();stats=[]
 for word,name in enumerate(['GRAH','BOM','DOOM','RUM']):
  for note in range(36,60):
   file=folder/f'{word}-{name}-{note}.wav';pcm=read_resample(file);ls,le=choose_loop(pcm,note,word);data,state=encode(pcm,ls)
   restored=decode(data,len(pcm));loop=decode(data,le-ls,*state,start=ls);assert loop==restored[ls:le]
   records.append(ENTRY.pack(word,note,1,len(blob),len(pcm),ls,le,state[0],state[1],0,RATE,len(data),zlib.crc32(data)))
   stats.append(dict(word=name,note=note,frames=len(pcm),loop_start=ls,loop_end=le,loop_jump=abs(restored[le-1]-restored[ls]),codec_rmse=round(math.sqrt(sum((a-b)**2 for a,b in zip(pcm,restored))/len(pcm)),2)))
   blob.extend(data)
 body=b''.join(records)+blob;header=struct.pack('<4sHH6I',b'ORKB',1,ENTRY.size,len(records),RATE,32+len(records)*ENTRY.size,32+len(body),zlib.crc32(body),0)
 bank=header+body;assert len(bank)<=3*PAYLOAD
 out.mkdir(parents=True,exist_ok=True);(out/'ork-bank.bin').write_bytes(bank)
 for slot in range(3):
  data=bank[slot*PAYLOAD:(slot+1)*PAYLOAD];(out/f'slot{slot+1}.hdr').write_bytes(fsmp_header(f'ORKB{slot+1}',data));(out/f'slot{slot+1}.bin').write_bytes(data)
 report=dict(bank_bytes=len(bank),capacity=3*PAYLOAD,spare=3*PAYLOAD-len(bank),sample_rate=RATE,assets=len(records),words=4,notes=[36,59],source_pcm_private=True,bank_sha256=hashlib.sha256(bank).hexdigest(),assets_detail=stats)
 (out/'bank-report.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k!='assets_detail'},indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('pcm',type=pathlib.Path);p.add_argument('out',type=pathlib.Path);a=p.parse_args();build(a.pcm,a.out)
