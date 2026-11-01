// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual page's outer pane cache with the real presenter.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
import {FramePresenter} from './frame_presenter.js';
import {PanePixels} from './display_performance.js';
import {joinScreens} from './second_screen.js';

function surface(){
  const s={width:240,height:240,events:{},lost:false,draws:0,encodes:0,
    insertAdjacentElement(){},addEventListener(n,fn){this.events[n]=fn;},
    toDataURL(){this.encodes++;return 'data:image/png;base64,fixture';}};
  const ctx={createImageData(w,h){return {data:new Uint8ClampedArray(w*h*4)};},
    putImageData(frame){s.draws++;s.pixels=frame.data.slice();},isContextLost:()=>s.lost};
  s.getContext=()=>ctx;return s;
}
const html=fs.readFileSync(new URL('./screen.html',import.meta.url),'utf8');
const code=html.slice(html.indexOf('function paint('),html.indexOf('async function loadRenderer()'));
for(const wide of [false,true])for(const side of ['left','right']){
  let time=0;
  const canvas=surface(),doc={createElement:tag=>tag==='canvas'?surface():{}},
    presenter=new FramePresenter(canvas,{doc,mobile:false,now:()=>time});
  const memory={buffer:new ArrayBuffer(240*240*2)},raw=new Uint16Array(memory.buffer);
  raw.fill(0xf800);
  const sandbox={ex:{memory,web_screen:()=>0},
    focus:{snapshot:()=>({side,left:{},right:{track:0}})},
    second:{layout:{views:[]},rawPixels:()=>raw},contextSnapshot:b=>b,joinScreens,
    panes:[new PanePixels(),new PanePixels()],joined:new Uint8ClampedArray(480*240*4),
    lastWide:null,lastSurface:null,paintPending:false,drawMs:0,wide,presenter,
    performance:{now:()=>time},Uint16Array,$:()=>({}),
    document:{body:{classList:{toggle(){}}}}};
  vm.createContext(sandbox);vm.runInContext(code+';this.paintFrame=paint;',sandbox);
  const bytes=new Uint8Array([1]);sandbox.paintFrame(bytes);
  const initial=canvas.pixels.slice(),draws=canvas.draws;
  sandbox.paintFrame(bytes);assert.equal(canvas.draws,draws,'outer cache skips identical panes');
  // The browser event switches surfaces while firmware pixels stay unchanged.
  canvas.lost=true;canvas.events.contextlost({preventDefault(){}});
  sandbox.paintFrame(bytes);
  assert.equal(presenter.compatible,true);assert.ok(presenter.image.src);
  assert.deepEqual(presenter.buffer.pixels,initial,'static image repainted after context loss');
  assert.equal(presenter.buffer.encodes,1);
  // A final changed snapshot inside the pacing interval must not be lost.
  time=10;raw[0]=0x07e0;sandbox.paintFrame(bytes);
  assert.equal(sandbox.paintPending,true);
  assert.deepEqual(presenter.buffer.pixels,initial);
  time=70;sandbox.paintFrame(bytes);
  assert.equal(sandbox.paintPending,false);
  assert.notDeepEqual(presenter.buffer.pixels,initial);
  assert.equal(presenter.buffer.encodes,2);
  // Returning to Canvas also repaints unchanged pixels on the new surface.
  canvas.lost=false;presenter.setCompatible(false);sandbox.paintFrame(bytes);
  assert.deepEqual(canvas.pixels,presenter.buffer.pixels);
}
console.log('frame pipeline: static context-loss recovery and deferred final frames in both layouts/focus sides passed');
