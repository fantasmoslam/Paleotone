#!/usr/bin/env python3
"""Generate the firmware's raw IMA payload from the credited compact WAV bank."""
from pathlib import Path
import struct
root = Path(__file__).resolve().parents[2]
rows=[]
for i in range(4):
    wav=(root/'tools/research/taiko-bank-05'/f'taiko-{i}.wav').read_bytes()
    assert wav[:4]==b'RIFF' and wav[8:12]==b'WAVE'
    pos=12; payload=None
    while pos+8<=len(wav):
        size=struct.unpack_from('<I',wav,pos+4)[0]
        if wav[pos:pos+4]==b'data': payload=wav[pos+8:pos+8+size]; break
        pos+=8+size+(size&1)
    assert payload is not None and len(payload)==9984
    rows.append('{\n'+',\n'.join(','.join(str(v) for v in payload[j:j+32]) for j in range(0,len(payload),32))+'\n}')
text='/* SPDX-License-Identifier: CC-BY-SA-4.0 */\n/* Derived SCC/Subaqueous/Floe taiko recordings. See tools/research/taiko-bank-05/ATTRIBUTION.md. */\nstatic const uint8_t taiko_data[4][9984] = {\n'+',\n'.join(rows)+'\n};\n'
(root/'firmware/src/dsp/taiko_data.inc').write_text(text,encoding='utf-8',newline='\n')
