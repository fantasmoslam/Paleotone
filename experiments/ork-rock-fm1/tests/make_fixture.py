#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic fixture only. No user recordings are required or uploaded for CI."""
import math,pathlib,struct,sys,zlib
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from pack_bank import encode,ENTRY,RATE
records=[];blob=bytearray()
for note in range(36,60):
 f=440*2**((note-69)/12);period=RATE/f;ls=round(2*period);le=ls+round(5*period);pcm=[round(11000*math.sin(2*math.pi*f*i/RATE)*min(1,i/32,(2047-i)/96)) for i in range(2048)];data,state=encode(pcm,ls)
 records.append(ENTRY.pack(0,note,1,len(blob),len(pcm),ls,le,*state,0,RATE,len(data),zlib.crc32(data)));blob.extend(data)
body=b''.join(records)+blob;bank=struct.pack('<4sHH6I',b'ORKB',1,36,24,RATE,32+24*36,32+len(body),zlib.crc32(body),0)+body
pathlib.Path(sys.argv[1]).write_bytes(bank)
