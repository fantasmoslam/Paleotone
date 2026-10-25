// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';import fs from 'node:fs';import vm from 'node:vm';import {performance} from 'node:perf_hooks';
import {rgb565} from './remote_screen.js';import {PanePixels} from './display_performance.js';
import {SecondScreen,contextSnapshot,joinScreens} from './second_screen.js';
const html=fs.readFileSync(new URL('./screen.html',import.meta.url),'utf8'),code=html.slice(html.indexOf('function paint('),html.indexOf('async function loadRenderer()'));
const mod=await WebAssembly.compile(fs.readFileSync(process.argv[2])),boot=()=>{const e=new WebAssembly.Instance(mod,{}).exports;e._initialize();e.web_nor_erase();e.web_boot();return e;};
const ex=boot(),second=new SecondScreen(boot(),JSON.parse(fs.readFileSync(new URL('remote_views.json',new URL('file:///'+process.argv[2].replaceAll('\\','/'))),'utf8')));
const f=fs.readFileSync(process.argv[3]),n=f.readUInt32LE(0),original=new Uint8Array(f.subarray(4,4+n));
const apply=bytes=>{new Uint8Array(ex.memory.buffer,ex.web_remote_buffer(),bytes.length).set(bytes);assert.equal(ex.web_remote_apply(bytes.length),1);};
const contexts=()=>Object.fromEntries([0,1].map(side=>[side?'right':'left',Object.fromEntries(['home','page','track','bank','cursor','lane','seqMode','grid'].map((k,i)=>[k,ex.web_panel_context(side,i)]))]));
apply(original);const ctx=contexts(),elements=new Map();let paints=0,actual,width;
const sandbox={ex,second,contextSnapshot,joinScreens,performance,Uint16Array,
 focus:{snapshot:()=>({side:ex.web_panel_side()?'right':'left',...ctx})},
 panes:[new PanePixels(),new PanePixels()],joined:new Uint8ClampedArray(480*240*4),lastWide:null,paintPending:false,drawMs:0,
 document:{body:{classList:{toggle(){}}}},$:id=>{if(!elements.has(id))elements.set(id,{});return elements.get(id);},
 presenter:{present(rgba,w){paints++;actual=rgba.slice();width=w;return true;}}
};
const c=vm.createContext(sandbox);vm.runInContext(code+';this.paintFrame=paint;',c);
for(const side of [0,1]){
 let bytes=original.slice();bytes[bytes.length-17]=side;bytes=contextSnapshot(bytes,second.layout,ctx[side?'right':'left']);apply(bytes);
 sandbox.sync={bytes,frameBytes:bytes};
 const active=rgb565(new Uint16Array(ex.memory.buffer,ex.web_screen(),240*240));
 for(const wide of [false,true]){
  sandbox.wide=wide;
  const expected=wide?joinScreens(rgb565(second.rawPixels(contextSnapshot(bytes,second.layout,ctx.left))),rgb565(second.rawPixels(contextSnapshot(bytes,second.layout,ctx.right)))):active;
  sandbox.paintFrame(bytes,true);assert.equal(width,wide?480:240);assert.deepEqual(actual,expected);
  const count=paints;sandbox.paintFrame(bytes);assert.equal(paints,count,'unchanged exact frames reuse their presentation');
 }
}
console.log('display paint: Original/Expanded and both physical sides equal full reference renderer pixels; unchanged panes are cached');
