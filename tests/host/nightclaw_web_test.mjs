// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual shipped WASM and AudioWorklet without a browser or audio device.
import fs from "node:fs";
import vm from "node:vm";
import assert from "node:assert/strict";
const root = process.argv[2] || "build/nightclaw";
const bytes = fs.readFileSync(root + "/nightclaw.wasm");
async function boot(store={}) {
  let mem;
  const stub = new Proxy({clock_time_get: (id,p,o)=>{new DataView(mem.buffer).setBigUint64(o,0n,true);return 0;},
    fd_write:(fd,iov,n,o)=>{new DataView(mem.buffer).setUint32(o,0,true);return 0;}},{get:(t,k)=>t[k]||(()=>0)});
  const {instance}=await WebAssembly.instantiate(bytes,{env:stub,wasi_snapshot_preview1:stub});
  const ex=instance.exports;mem=ex.memory;ex._initialize?.();
  for(const [o,b] of Object.entries(store)){new Uint8Array(mem.buffer,ex.web_store(+o),b.length).set(b);ex.web_store_set_len(+o,b.length);}
  ex.web_master(2800);ex.web_boot();
  const str=p=>{const m=new Uint8Array(mem.buffer);let e=p;while(m[e])e++;return new TextDecoder().decode(m.subarray(p,e));};
  const state=()=>Array.from(new Int32Array(mem.buffer,ex.nc_state(),32));
  let frames=0;
  const render=(n=4410)=>{let peak=0;while(n>0){const len=Math.min(128,n);ex.web_render(len);frames+=len;for(const p of [ex.web_out_l(),ex.web_out_r()])for(const v of new Float32Array(mem.buffer,p,len)){assert(Number.isFinite(v));assert(Math.abs(v)<=1.05,"audio output bound");peak=Math.max(peak,Math.abs(v));}n-=len;}return peak;};
  return{ex,mem,str,state,render};
}
const b=await boot(),{ex,str,state,render}=b;
assert.equal(ex.nc_scene_count(),20);
const names=Array.from({length:20},(_,i)=>str(ex.nc_scene_name(i)));assert(names.includes("RAPTOR NIGHT"));
for(let i=0;i<20;i++){ex.nc_scene(i);assert.equal(state()[0],i);assert(render(44100)>0.001,names[i]+" audible");}
ex.nc_scene(names.indexOf("RAPTOR NIGHT"));ex.nc_track(2);
assert.equal(state()[1],7);assert.equal(str(ex.nc_page_name()),"Home");
const voices=Array.from({length:ex.nc_voice_count()},(_,i)=>str(ex.nc_voice_name(i)));
for(const name of ["Theremin","Cello","Cello Low","Sub Pad","Fat Pad"])assert(voices.includes(name),name+" available");
ex.nc_voice(voices.indexOf("Theremin"));assert.equal(state()[7],voices.indexOf("Theremin"));
let s=state();ex.nc_control(0,s[12]-1);assert.equal(state()[12],s[12]-1);
ex.web_keys(1);render(4410);ex.web_keys(0);render(4410);
ex.nc_track(5);assert.equal(ex.nc_voice_count(),14);ex.nc_voice(13);assert.equal(state()[7],13);
ex.nc_track(6);assert.equal(state()[9],1);assert.equal(state()[6],0);ex.nc_calls_auto(1);render(1024);assert.equal(state()[6],1);ex.nc_calls_auto(0);render(1024);
ex.web_keys(1);render(22050);ex.web_keys(0);
ex.nc_page(17);assert.equal(state()[9],2);ex.nc_page(18);assert.equal(str(ex.nc_control_name(0)),"Tempo");
ex.nc_page(15);assert.equal(state()[1],15);
ex.nc_page(3);const octave=state()[15];ex.nc_track(2);ex.nc_octave(octave>=1?-1:1);ex.nc_page(3);assert.equal(state()[15],octave+(octave>=1?-1:1));
ex.nc_track(2);ex.nc_play(0);render(1024);assert.equal(state()[3],0);ex.nc_play(1);render(1024);assert.equal(state()[3],1);
ex.nc_hold(1);render(1024);assert.equal(state()[4],1);ex.nc_hold(0);render(1024);assert.equal(state()[4],0);
ex.nc_save();assert.equal(state()[5],0);const saved={};
for(let i=0;i<ex.web_nobj();i++){const n=ex.web_store_len(i);if(n>=0)saved[i]=new Uint8Array(b.mem.buffer,ex.web_store(i),n).slice();}
assert(Object.keys(saved).length>0);const rebooted=await boot(saved);rebooted.ex.nc_track(2);assert.equal(rebooted.state()[7],voices.indexOf("Theremin"));rebooted.ex.nc_track(5);assert.equal(rebooted.state()[7],13);assert.equal(rebooted.state()[6],0);
console.log("WASM: 20 audible scenes, voices, controls, beats, calls, pages, transport and save/reboot passed.");

for(const rate of [44100,48000]){
 const messages=[];let Processor;
 const context=vm.createContext({
  AudioWorkletProcessor:class{constructor(){this.port={onmessage:null,postMessage:m=>messages.push(m)};}},
  registerProcessor:(name,ctor)=>{assert.equal(name,"nightclaw");Processor=ctor;},
  WebAssembly,Uint8Array,Uint16Array,Float32Array,Int32Array,ArrayBuffer,DataView,BigInt,Proxy,Math,Number,String,Object,Array,Error,
  currentTime:0,sampleRate:rate,
 });
 vm.runInContext(fs.readFileSync(root+"/worklet.js","utf8"),context);
 const p=new Processor();await p.message({type:"load",wasm:bytes,store:{}});
 assert(messages.some(m=>m.type==="ready"));assert(!messages.some(m=>m.type==="error"));
 await p.message({type:"track",value:5});await p.message({type:"voice",value:12});
 await p.message({type:"control",col:0,value:80});await p.message({type:"save"});
 assert(messages.some(m=>m.store),"saved bytes sent to browser");
 const out=[new Float32Array(128),new Float32Array(128)];let peak=0;
 for(let k=0;k<Math.ceil(rate*2/128);k++){context.currentTime=k*128/rate;assert.equal(p.process([], [out]),true);for(const ch of out)for(const v of ch){assert(Number.isFinite(v));assert(Math.abs(v)<=1.05);peak=Math.max(peak,Math.abs(v));}}
 assert(peak>0.001);assert(messages.some(m=>m.fb?.length===57600));assert(!messages.some(m=>m.type==="error"));
 const t=p.ex.web_now();assert(Math.abs(t-2000)<70,"firmware timing preserved at "+rate+": "+t);
 await p.message({type:"keys",mask:0x7fffffff});await p.message({type:"keys",mask:0});
 await p.message({type:"visible",value:false});const before=messages.length;
 for(let k=0;k<10;k++){context.currentTime+=128/rate;p.process([], [out]);}assert.equal(messages.length,before,"no background screen traffic");
 console.log("AudioWorklet: "+rate+" Hz, commands, finite stereo output, screen, persistence and background suppression passed.");
}
