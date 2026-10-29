// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {FramePresenter} from './frame_presenter.js';
function surface(){
  const element={width:240,height:240,hidden:false,events:{},lost:false,
    insertAdjacentElement(where,image){this.image=image;},addEventListener(name,fn){this.events[name]=fn;},
    getContext(type,options){this.options=options;return this.ctx;},
    toDataURL(type){assert.equal(type,'image/png');return 'data:image/png;base64,fixture';}};
  element.ctx={createImageData(w,h){return {data:new Uint8ClampedArray(w*h*4)};},
    putImageData(frame){element.pixels=frame.data.slice();},isContextLost:()=>element.lost};return element;
}
const created=[],doc={createElement(tag){const e=tag==='canvas'?surface():{};created.push(e);return e;}};
let time=0;
const canvas=surface(),p=new FramePresenter(canvas,{doc,mobile:true,now:()=>time});
const rgba=new Uint8ClampedArray(240*240*4);for(let i=3;i<rgba.length;i+=4)rgba[i]=255;rgba[40]=128;
p.present(rgba,240);assert.equal(canvas.hidden,true);assert.equal(p.image.hidden,false);
assert.deepEqual(p.buffer.pixels,rgba,'the compatible surface must preserve every original firmware pixel');
assert.match(p.image.src,/^data:image\/png/);assert.equal(p.black,false);
p.setCompatible(false);p.present(rgba,240);assert.deepEqual(canvas.pixels,rgba);assert.equal(canvas.hidden,false);
canvas.lost=true;time=200;p.present(rgba,240);assert.equal(p.compatible,true);
const black=rgba.slice();black[40]=0;time=400;p.present(black,240);assert.match(p.status(),/black frame/);
assert.throws(()=>p.present(new Uint8ClampedArray(3),240),/Invalid/);
p.buffer.lost=true;time=600;assert.throws(()=>p.present(rgba,240),/lost/);
const fast=new FramePresenter(surface(),{doc,now:()=>time});assert.equal(fast.compatible,false);
fast.present(rgba,240);const reused=fast.frame;time+=34;fast.present(rgba,240);assert.equal(fast.frame,reused);
assert.deepEqual(fast.metrics(),{received:2,painted:1,skipped:1});
for(let n=0;n<100;n++)fast.present(rgba,240);
assert.equal(fast.metrics().painted,1,'identical pixels require no additional writes');
const single=rgba.slice();single[41]=17;fast.present(single,240);
assert.equal(fast.canvas.pixels[41],17,'single-channel changes are visible');
fast.canvas.lost=true;time+=100;fast.present(single,240);
assert.equal(fast.compatible,true,'health check precedes identical-pixel skipping');
assert.deepEqual(fast.buffer.pixels,single);
time+=10;single[41]=18;assert.equal(fast.present(single,240),false,'deferred presentation is explicit');
time+=60;assert.equal(fast.present(single,240),true);
assert.deepEqual(fast.buffer.pixels,single,'latest frame survives throttling');
console.log('frame presenter: exact pixels, Android image surface, Canvas fallback, context loss and black-frame reporting passed');
