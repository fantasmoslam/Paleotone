// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {SecondScreen,viewSnapshot,joinScreens} from './second_screen.js';
const layoutFile=process.argv[2]?new URL('remote_views.json',pathToFileURL(resolve(process.argv[2]))):new URL('./remote_views.json',import.meta.url);
const layout=JSON.parse(await readFile(layoutFile,'utf8'));
assert.doesNotThrow(()=>new SecondScreen({web_remote_id:()=>-2147483648,web_remote_schema:()=>1},{rendererId:2147483648,schema:1}),
  'WASM returns signed i32 while the renderer descriptor stores unsigned identity');
assert.throws(()=>new SecondScreen({web_remote_id:()=>-2147483648,web_remote_schema:()=>1},{rendererId:2147483649,schema:1}),/does not match/);
const bytes=new Uint8Array(16384).fill(5), pristine=bytes.slice();
for(const view of layout.views){
  const copy=viewSnapshot(bytes,layout,view.id,'2');
  assert.equal(copy[layout.ui.home],view.home);assert.equal(copy[layout.ui.page],view.page);
  assert.equal(copy[layout.selectedTrack],2);assert.deepEqual(bytes,pristine);
  const allowed=new Set([layout.selectedTrack,...['home','page','menu','confirm','layer','uboot','msg_t','hot_t'].map(k=>layout.ui[k])]);
  for(let i=0;i<bytes.length;i++)if(!allowed.has(i))assert.equal(copy[i],bytes[i]);
}
assert.equal(viewSnapshot(bytes,layout,'home','follow')[layout.selectedTrack],5);
assert.throws(()=>viewSnapshot(bytes,layout,'unknown'));
assert.throws(()=>viewSnapshot(bytes,layout,'home','4'));
assert.throws(()=>viewSnapshot(new Uint8Array(10),layout,'home'));
const left=new Uint8ClampedArray(240*240*4).fill(7),right=left.slice().fill(9),wide=joinScreens(left,right);
for(let y=0;y<240;y++){
  assert.deepEqual(wide.subarray(y*1920,y*1920+960),left.subarray(y*960,(y+1)*960));
  assert.deepEqual(wide.subarray(y*1920+960,(y+1)*1920),right.subarray(y*960,(y+1)*960));
}
if(process.argv[2]){
  const {module}=await WebAssembly.compile(await readFile(process.argv[2])).then(module=>({module}));
  const boot=()=>{const ex=new WebAssembly.Instance(module,{}).exports;ex._initialize();ex.web_nor_erase();ex.web_boot();return ex;};
  const main=boot(),other=boot(),second=new SecondScreen(other,layout);
  const fixture=await readFile(process.argv[3]),n=fixture.readUInt32LE(0),snapshot=new Uint8Array(fixture.subarray(4,4+n));
  new Uint8Array(main.memory.buffer,main.web_remote_buffer(),n).set(snapshot);
  assert.equal(main.web_remote_apply(n),1);
  const original=Buffer.from(new Uint8Array(main.memory.buffer,main.web_screen(),240*240*2));
  const time=main.web_now_ms(),writes=other.web_flash_writes_count(),playing=main.web_playing();
  const output=new Set();
  for(const view of layout.views)for(const track of ['follow','0','1','2','3']){
    const pixels=second.pixels(snapshot,view.id,track);
    output.add(Buffer.from(pixels).toString('base64'));
    assert.deepEqual(Buffer.from(main.memory.buffer,main.web_screen(),240*240*2),original);
    assert.equal(main.web_now_ms(),time);assert.equal(main.web_playing(),playing);
    assert.equal(other.web_flash_writes_count(),writes);
  }
  assert.ok(output.size>=layout.views.length,'view choices must produce different screens');
  assert.throws(()=>new SecondScreen(other,{...layout,rendererId:0}));
}
console.log('second screen: immutable snapshots, isolated views, track selection, and original-pixel preservation passed');
