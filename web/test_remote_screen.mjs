// SPDX-License-Identifier: GPL-3.0-only
import assert from "node:assert/strict";
import {readFile} from "node:fs/promises";
import {DisplaySync,MidiLink,HEADER,u14,unpack7,parseStatus,rgb565,MidiTimeoutError} from "./remote_screen.js";
assert.deepEqual([...unpack7([5,0,127,1],3)],[128,127,129]);
assert.throws(()=>unpack7([4,0],1));
assert.throws(()=>unpack7([0],1));
assert.throws(()=>unpack7([0,1,2],1));
assert.deepEqual([...rgb565(new Uint16Array([0x00f8,0xe007,0x1f00]))],[255,0,0,255,0,255,0,255,0,0,255,255]);
function status(op,rev,base,dirty=255){
  const a=[op,0,1,1,0,0,0,0,...u14(rev),...u14(base),...u14(130)];
  for(let i=0;i<16;i++)a.push(i===0?dirty&127:0,i===0?dirty>>7:0);
  return a;
}
assert.equal(parseStatus(status(1,2,1)).length,130);
assert.throws(()=>parseStatus([]));
let fakeRev=2, fakeBase=0, fakeDirty=255, dropBlock=false, dropAck=false;
const calls=[], input={state:"connected"}, output={state:"connected",send(message){
  calls.push(message);
  const cmd=message[4], args=message.slice(5,-1);
  if(cmd===93&&dropBlock){dropBlock=false;return;} // simulate a lost reply during sequencer MIDI traffic
  if(cmd===92&&args[0]===2&&dropAck){dropAck=false;return;}
  input.onmidimessage?.({data:[0xf8]}); // unrelated MIDI clock and notes must not resolve a read
  input.onmidimessage?.({data:[0x90,60,100]});
  let payload;
  if(cmd===92)payload=status(args[0],fakeRev,args[0]===2?fakeRev:fakeBase,fakeDirty);
  else if(cmd===93){
    const block=args[2]|args[3]<<7,n=block?2:128,data=Array.from({length:n},(_,i)=>(i+block)&255),packed=[];
    for(let i=0;i<n;i+=7){const part=data.slice(i,i+7);packed.push(part.reduce((a,v,j)=>a|v>>7<<j,0),...part.map(v=>v&127));}
    payload=[...args,0,...u14(n),...packed];
  } else payload=[args[0],0];
  setTimeout(()=>input.onmidimessage?.({data:[...HEADER,cmd,...payload,0xf7]}),0);
}};
const link=new MidiLink(input,output,50), applied=[];
const sync=new DisplaySync(link,1,1,(bytes)=>{applied.push(bytes.slice());return true;});
await sync.update();
assert.equal(applied[0].length,130);assert.equal(applied[0][128],1);
assert.equal(sync.rev,2);assert.equal(calls.filter(p=>p[4]===93).length,2);
fakeRev=3;fakeBase=2;fakeDirty=1;
await sync.update();
assert.equal(sync.rev,3);
assert.equal(calls.filter(p=>p[4]===93).length,3); // only the changed block
assert.equal(applied[1][128],applied[0][128]);     // tail retained from acknowledged base
fakeRev=4;fakeBase=0;fakeDirty=0;
await sync.update();
assert.equal(calls.filter(p=>p[4]===93).length,5); // reconnect/base loss forces both blocks
let retried=0;link.onRetry=()=>retried++;
fakeRev=5;fakeBase=0;dropBlock=true;dropAck=true;
await sync.update();
assert.equal(sync.rev,5);assert.equal(retried,2); // lost block and lost acknowledgement recovered
assert.throws(()=>sync.check({...parseStatus(status(0,0,0)),id:2}),/versions do not match/);
await link.inputCommand([1,0,65]);
link.close();await assert.rejects(link.request(92,[0]),/disconnected/i);
let lostReads=0;
const silent=new MidiLink({state:"connected"},{state:"connected",send(){lostReads++;}},5);
await assert.rejects(silent.status(0),(e)=>e instanceof MidiTimeoutError);
assert.equal(lostReads,3);silent.close();
let turns=0;
const lostInput=new MidiLink({state:"connected"},{state:"connected",send(){turns++;}},5);
await assert.rejects(lostInput.inputCommand([1,3,65]),(e)=>e instanceof MidiTimeoutError);
assert.equal(turns,1); // an encoder is NEVER retried after a lost acknowledgement
await assert.rejects(lostInput.readRequest(94,[1,3,65]),/Only display/);
assert.equal(turns,1);lostInput.close();

// Optional firmware bursts: out-of-order/duplicate replies, lost bursts, stale snapshots and legacy fallback.
{
 let drop=1,burstCalls=0,bad=false;
 const input={state:'connected'},output={state:'connected',send(message){
   const cmd=message[4],args=message.slice(5,-1);
   const reply=(c,p)=>setTimeout(()=>input.onmidimessage?.({data:[...HEADER,c,...p,0xF7]}),0);
   if(cmd===92){reply(92,[...status(args[0],9,args[0]===2?9:0),4]);return;}
   if(cmd===95){
     burstCalls++;if(drop-- > 0)return;
     reply(93,[...u14(8),...u14(args[3]),1]); // stale revision must be ignored
     for(const b of args.slice(3).reverse()){
       const count=b?2:128,raw=Array.from({length:count},(_,i)=>(i+b)&255),packed=[];
       for(let i=0;i<count;i+=7){const group=raw.slice(i,i+7);packed.push(group.reduce((v,x,j)=>v|(x>>7)<<j,0),...group.map(x=>x&127));}
       const payload=[...u14(9),...u14(b),bad?1:0,...u14(count),...packed];
       reply(93,payload);reply(93,payload);
     }
     reply(95,[...u14(9),args[2],0]);
   }
 }};
 const link=new MidiLink(input,output,20),seen=[];
 const sync=new DisplaySync(link,1,1,bytes=>{seen.push(bytes.slice());return true;});
 let painted=false;
 await sync.update(()=>{painted=true;assert.equal(sync.rev,0,'present before waiting for ACK');});
 assert.ok(painted);assert.equal(sync.rev,9);assert.equal(burstCalls,2);assert.equal(seen[0][128],1);
 bad=true;await assert.rejects(sync.update(),e=>e.code==='SNAPSHOT_STALE');
 await assert.rejects(link.readBlocks(9,[0,0]),/Invalid/);
 link.close();
}
if(process.argv[2]){
  const {instance}=await WebAssembly.instantiate(await readFile(process.argv[2]),{});
  const ex=instance.exports;ex._initialize();ex.web_nor_erase();ex.web_boot();
  const fixture=await readFile(process.argv[3]),n=fixture.readUInt32LE(0);
  new Uint8Array(ex.memory.buffer,ex.web_remote_buffer(),n).set(fixture.subarray(4,4+n));
  assert.equal(ex.web_remote_apply(n),1);
  assert.deepEqual(Buffer.from(ex.memory.buffer,ex.web_screen(),240*240*2),fixture.subarray(4+n,4+n+240*240*2));
  ex.web_remote_expand();
  assert.deepEqual(Buffer.from(ex.memory.buffer,ex.web_remote_wide(),480*240*2),fixture.subarray(4+n+240*240*2));
}
console.log("remote screen protocol and renderer tests passed");
