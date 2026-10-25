// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';import fs from 'node:fs';
import {unpackCompressedBlock,parseStatus,DisplaySync,MidiLink,HEADER,u14} from './remote_screen.js';
const pack=bytes=>{const p=[];for(let i=0;i<bytes.length;i+=7){const b=bytes.slice(i,i+7);p.push(b.reduce((v,x,k)=>v|(x>>7)<<k,0),...b.map(x=>x&127));}return p;};
const block=(count,encoding,bytes)=>[1,0,0,0,0,...u14(count),encoding,...u14(bytes.length),...pack(bytes)];
assert.deepEqual([...unpackCompressedBlock(block(128,1,[255,200]))],Array(128).fill(200));
for(const bad of [block(128,2,[255,0]),block(1,1,[255,0]),block(128,1,[255]),block(128,1,[127,0]),block(128,1,[128,0]),block(1,0,[0,0]),block(0,0,[0])])assert.throws(()=>unpackCompressedBlock(bad));
for(const file of process.argv.slice(2)){
 const data=fs.readFileSync(file);let pos=0,cases=0;
 while(pos<data.length){const n=data.readUInt32LE(pos);pos+=4;const p=[...data.subarray(pos,pos+n)];pos+=n;const expected=data.subarray(pos,pos+(p[5]|p[6]<<7));pos+=expected.length;assert.deepEqual(Buffer.from(unpackCompressedBlock(p)),expected);cases++;}
 assert.equal(pos,data.length);assert.ok(cases);console.log(`${file}: ${cases} C encoder → JS decoder byte-exact fixtures passed`);
}
// Exercise negotiated compression through the actual request, stream, apply and ACK path.
const input={state:'connected'},sent=[];let drop=true;
const status=op=>[op,0,2,1,0,0,0,0,1,0,op===2?1:0,0,0,1,...Array(32).fill(0),68]; // 128 bytes
const output={state:'connected',send(message){
 sent.push(message);const cmd=message[4],a=message.slice(5,-1),reply=(c,p)=>setTimeout(()=>input.onmidimessage({data:[...HEADER,c,...p,247]}),0);
 if(cmd===92)reply(cmd,status(a[0]));
 if(cmd===95){assert.equal(a[2],65);if(drop){drop=false;return;}reply(93,block(128,1,[255,200]));reply(95,[1,0,65,0]);}
}};
const link=new MidiLink(input,output,10);let actual;
const sync=new DisplaySync(link,1,2,bytes=>{actual=bytes.slice();return true;});
await sync.update();assert.deepEqual([...actual],Array(128).fill(200));assert.equal(sync.rev,1);assert.equal(sent.filter(p=>p[4]===95).length,2);link.close();
assert.equal(parseStatus(status(0)).compact,true);assert.equal(parseStatus(status(0)).batchMax,4);
assert.equal(parseStatus(status(0).slice(0,46)).compact,false);
console.log('compression: strict bounds, malformed payload rejection, capability fallback and lost-burst retry passed');
