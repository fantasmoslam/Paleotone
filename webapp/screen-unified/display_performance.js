// SPDX-License-Identifier: GPL-3.0-only
import {rgb565} from './remote_screen.js?v=2540340739-1f68c41';
// Cache exact C-renderer pixels, never an approximation of instrument state.
export class PanePixels {
  constructor(){this.raw=null;this.rgba=null;}
  update(pixels){
    let changed=!this.raw||this.raw.length!==pixels.length;
    if(!changed)for(let i=0;i<pixels.length;i++)if(this.raw[i]!==pixels[i]){changed=true;break;}
    if(changed){
      if(!this.raw||this.raw.length!==pixels.length){this.raw=new Uint16Array(pixels.length);this.rgba=new Uint8ClampedArray(pixels.length*4);}
      this.raw.set(pixels);rgb565(pixels,this.rgba);
    }
    return {rgba:this.rgba,changed};
  }
}
export function refreshDelay(elapsed,renderMs,{hidden=false,compatible=false}={}){
  const fps=hidden?4:Math.min(compatible?15:30,1000/Math.max(1,renderMs*3));
  return Math.max(0,1000/fps-elapsed);
}
export class DisplayTiming {
  constructor(now=()=>performance.now()){this.now=now;this.started=now();this.frames=0;this.last={};}
  record(metrics,drawMs){this.frames++;this.last={...metrics,drawMs};}
  text(){
    const seconds=(this.now()-this.started)/1000;
    return (this.frames/Math.max(.001,seconds)).toFixed(1)+' updates/s · '+
      (this.last.transferMs||0).toFixed(1)+' ms transfer · '+(this.last.drawMs||0).toFixed(1)+' ms draw';
  }
}
