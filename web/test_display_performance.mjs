// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {PanePixels,refreshDelay,DisplayTiming} from './display_performance.js';
import {rgb565} from './remote_screen.js';
import {joinScreens} from './second_screen.js';
const pixels=Uint16Array.from({length:65536},(_,i)=>i),expected=new Uint8ClampedArray(65536*4);
for(let i=0;i<pixels.length;i++){const sw=pixels[i],c=(sw>>>8|sw<<8)&65535;expected.set([Math.round((c>>11)*255/31),Math.round((c>>5&63)*255/63),Math.round((c&31)*255/31),255],i*4);}
const target=new Uint8ClampedArray(expected.length);assert.equal(rgb565(pixels,target),target);assert.deepEqual(target,expected);
const p=new PanePixels(),src=new Uint16Array([0x00f8,0xe007]);
const a=p.update(src);assert.ok(a.changed);const rgba=a.rgba;
assert.equal(p.update(src).changed,false);src[0]=0x1f00;
assert.ok(p.update(src).changed);assert.equal(p.rgba,rgba);assert.equal(p.raw[0],src[0]);
const left=new Uint8ClampedArray(240*240*4).fill(7),right=left.slice().fill(9),out=new Uint8ClampedArray(480*240*4);
assert.equal(joinScreens(left,right,out),out);
assert.equal(refreshDelay(45,2),0,'a slow USB read must not add a further 20 ms floor');
assert.ok(refreshDelay(2,2)>30&&refreshDelay(2,2)<32);
assert.ok(refreshDelay(20,1,{compatible:true})>46);
assert.equal(refreshDelay(260,2,{hidden:true}),0);
// Simulate complete transfers across a range of USB loads. The trial must
// reduce request pressure without changing the legacy responsive cadence.
for(const transfer of [2,29,45,99,100,150,260]){
  const gap=refreshDelay(transfer,2,{audioPriority:true});
  assert.ok(gap>=12,'even slow transfers must leave a breathing gap');
  assert.ok(transfer+gap>=100,'audio-priority snapshot starts must be at most 10 Hz');
  assert.ok(gap>=refreshDelay(transfer,2),'the trial must never poll faster');
}
assert.equal(refreshDelay(45,2,{audioPriority:true}),55);
assert.equal(refreshDelay(150,2,{audioPriority:true}),12);
assert.equal(refreshDelay(20,1,{audioPriority:true,compatible:true}),80);
assert.equal(refreshDelay(20,1,{audioPriority:true,hidden:true}),230);
assert.equal(refreshDelay(260,2,{audioPriority:true,hidden:true}),12);
let now=0;const timing=new DisplayTiming(()=>now);now=1000;timing.record({transferMs:43},1.2);
assert.match(timing.text(),/1.0 updates\/s · 43.0 ms transfer · 1.2 ms draw/);
console.log('display performance: all 65536 exact colors, reusable buffers, pane caching and bounded adaptive cadence passed');
