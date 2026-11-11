#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Encode the generated NightClaw artwork for the FM-1's monochrome boot screen."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
N=88

def pack(data):
    out=bytearray();p=0
    while p<len(data):
        run=1
        while p+run<len(data) and data[p+run]==data[p] and run<128:run+=1
        if run>=3:
            out.extend((127+run,data[p]));p+=run
        else:
            start=p;p+=run
            while p<len(data) and p-start<128:
                r=1
                while p+r<len(data) and data[p+r]==data[p] and r<128:r+=1
                if r>=3:break
                p+=min(r,128-(p-start))
            out.append(p-start-1);out.extend(data[start:p])
    return out

def unpack(data):
    out=bytearray();p=0
    while p<len(data):
        c=data[p];p+=1;n=(c&127)+1
        if c&128:out.extend([data[p]]*n);p+=1
        else:out.extend(data[p:p+n]);p+=n
    return bytes(out)

im=Image.open(ROOT/'docs/assets/nightclaw-jungle.png').convert('L')
im=im.resize((N,N),Image.Resampling.LANCZOS).point(lambda x:255 if x>=128 else 0).convert('1')
raw=im.tobytes();encoded=pack(raw)
assert unpack(encoded)==raw and len(raw)==N*N//8
text='/* NightClaw original generated artwork; monochrome LCD encoding. */\n'
text+=f'#define NC_SPLASH_N {N}\n#define NC_SPLASH_BYTES {len(raw)}\n'
text+='static const uint8_t NC_SPLASH_RLE[] = {\n'
text+=',\n'.join(','.join(str(v) for v in encoded[j:j+24]) for j in range(0,len(encoded),24))+'\n};\n'
(ROOT/'firmware/src/app/nightclaw_splash.inc').write_text(text,encoding='utf-8',newline='\n')
(ROOT/'build').mkdir(exist_ok=True)
im.resize((240,240),Image.Resampling.NEAREST).save(ROOT/'build/nightclaw-lcd-art.png')
print(f'NightClaw splash: {N} x {N}, {len(encoded)} bytes')
