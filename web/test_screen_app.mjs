// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import vm from 'node:vm';
const here=new URL('./screen/',import.meta.url);
const html=await readFile(new URL('index.html',here),'utf8');
const source=await readFile(new URL('screen.js',here),'utf8');
const manifest=JSON.parse(await readFile(new URL('manifest.webmanifest',here),'utf8'));
assert.equal(manifest.start_url,'./');assert.equal(manifest.scope,'./');
for(const file of ['screen.js','sw.js','icon.svg','manifest.webmanifest'])assert.ok((await readFile(new URL(file,here))).length);
assert.match(html,/src="\.\/screen.js"/);assert.doesNotMatch(html,/fonts.googleapis/);
let now=100,intervals=[],sent=[],elements=new Map();
const element=id=>{
  if(!elements.has(id))elements.set(id,{textContent:'',hidden:true,value:'',children:[],append(o){this.children.push(o);},replaceChildren(){this.children=[];},getContext(){return new Proxy({},{get:()=>()=>{}});}});
  return elements.get(id);
};
const input={id:'in',name:'FM-1',manufacturer:'M-VAVE',state:'connected',async open(){}};
const output={id:'out',name:'FM-1',manufacturer:'M-VAVE',state:'connected',async open(){},clear(){},send(bytes){sent.push(bytes);}};
const midi={inputs:new Map([['in',input]]),outputs:new Map([['out',output]])};
const context=vm.createContext({document:{getElementById:element,createElement:()=>({})},navigator:{async requestMIDIAccess(opts){assert.equal(opts.sysex,true);return midi;}},window:{isSecureContext:true},performance:{now:()=>now},setInterval:(fn,ms)=>intervals.push({fn,ms}),addEventListener(){},console});
vm.runInContext(source,context);
await element('go').onclick();await new Promise(resolve=>setImmediate(resolve));
assert.equal(typeof input.onmidimessage,'function');
const tx=()=>intervals.find(i=>i.ms===12).fn();tx();assert.equal(sent[0][4],1);
const str=s=>[...s].map(c=>c.charCodeAt(0)).concat(0);
const receive=(cmd,p)=>input.onmidimessage({data:new Uint8Array([240,125,70,76,cmd,...p,247])});
receive(1,[...str('test'),1,104,32,64,96,...str('ANALOG')]);
// Malformed snapshots do not produce a connected display.
receive(90,[1]);assert.equal(vm.runInContext('scr',context),null);
receive(90,[1,0,0,127,0,0,0,0,0,0,0,0]);
receive(91,[0,1,...str('ENV'),0,0,1,1,2,3,4]);
receive(5,[0,1,8,0,64,1,64,0,64,...str('MODE'),0,...str('OFF'),...str('ON')]);
assert.equal(vm.runInContext('fmtVal(desc["0:1"],1)',context),'ON');
for(let i=0;i<140;i++)tx();
const allowed=new Set([1,4,5,10,22,25,27,72,90,91]);
assert.ok(sent.every(p=>allowed.has(p[4])));
assert.ok(sent.filter(p=>p[4]===27).every(p=>p.length===6),'TRACK queries must never select a track');
assert.throws(()=>vm.runInContext('tx(3,0,1,0,64)',context),/Read-only/);
// A stale connection clears values, descriptors and queued work before reconnect.
now=7000;intervals.find(i=>i.ms===1500).fn();assert.equal(vm.runInContext('out',context),null);
assert.equal(vm.runInContext('q.length',context),0);assert.equal(vm.runInContext('scr',context),null);
output.state='disconnected';input.state='disconnected';midi.onstatechange();
assert.match(element('st').textContent,/No FM-1 found/);
output.state=input.state='connected';midi.onstatechange();await new Promise(resolve=>setImmediate(resolve));
assert.equal(typeof input.onmidimessage,'function');
console.log('screen app: packaged paths, relative install scope, handshake, malformed state, enum labels, read-only commands and USB reconnection passed');
