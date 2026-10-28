// SPDX-License-Identifier: GPL-3.0-only
// Deterministic software-only rendering workload: NOT a USB or hardware FPS benchmark.
import assert from 'node:assert/strict';
import {FramePresenter} from './frame_presenter.js';

function mockCanvas() {
  const canvas={width:0,height:0,hidden:false,draws:0,encodes:0,allocations:0,
    insertAdjacentElement(){},addEventListener(){},
    toDataURL(){this.encodes++;return 'data:image/png;base64,benchmark';}};
  const ctx={
    createImageData(w,h){canvas.allocations++;return {data:new Uint8ClampedArray(w*h*4)};},
    putImageData(){canvas.draws++;},
    isContextLost(){return false;}
  };
  canvas.getContext=()=>ctx;
  return canvas;
}
function run(mobile,width,frames,changeEvery) {
  let now=0;
  const canvas=mockCanvas(),created=[];
  const doc={createElement(tag){
    if(tag==='canvas'){const item=mockCanvas();created.push(item);return item;}
    return {hidden:false};
  }};
  const presenter=new FramePresenter(canvas,{doc,mobile,now:()=>now});
  const pixels=new Uint8ClampedArray(width*240*4);
  for(let i=3;i<pixels.length;i+=4)pixels[i]=255;
  for(let n=0;n<frames;n++){
    now=n*45;
    if(n%changeEvery===0)pixels[0]=(pixels[0]+1)&255;
    presenter.present(pixels,width,240);
  }
  return {metrics:presenter.metrics(),canvas,buffer:created[0]};
}
const still=run(false,480,1000,1000);
assert.equal(still.metrics.painted,1);
assert.equal(still.canvas.draws,1);
assert.equal(still.canvas.allocations,1);
const animated=run(false,480,1000,1);
assert.equal(animated.metrics.painted,1000);
assert.equal(animated.canvas.allocations,1,'reuse ImageData even as all frames change');
const mobile=run(true,480,1000,1);
assert.ok(mobile.metrics.painted<=500,'90 ms compatible pacing limits encodes at 45 ms inputs');
assert.equal(mobile.buffer.encodes,mobile.metrics.painted);
assert.equal(mobile.buffer.allocations,1);
console.log(JSON.stringify({
  kind:'synthetic browser presenter workload (no device)',
  frames:1000,frameSpacingMs:45,width:480,height:240,
  unchanged:still.metrics,animated: animated.metrics,compatible:mobile.metrics
},null,2));
