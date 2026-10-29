// SPDX-License-Identifier: GPL-3.0-only
// Display the same firmware pixels through either Canvas or a PNG image surface.
export class FramePresenter {
  constructor(canvas,{doc=document,mobile=false,now=()=>performance.now()}={}){
    this.canvas=canvas;this.doc=doc;this.now=now;this.last=-Infinity;this.black=false;
    this.buffer=doc.createElement('canvas');
    this.image=doc.createElement('img');this.image.id='screen-image';this.image.alt='FM-1 live screen';this.image.hidden=true;
    canvas.insertAdjacentElement('afterend',this.image);
    this.compatible=mobile;
    this.context=canvas.getContext('2d',{alpha:false,desynchronized:true});
    this.bufferContext=this.buffer.getContext('2d',{alpha:false,willReadFrequently:true});
    if(!this.context||!this.bufferContext)throw new Error('This browser could not create the screen drawing surface.');
    canvas.addEventListener('contextlost',event=>{event.preventDefault();this.setCompatible(true);});
    this.frame=null;this.previous=null;this.surfaceRevision=0;
    this.frames=0;this.changed=0;this.skipped=0;this.setCompatible(mobile);
  }
  setCompatible(on){
    this.compatible=!!on;this.last=-Infinity;this.frame=null;this.previous=null;
    this.surfaceRevision++;
    this.canvas.hidden=this.compatible;this.image.hidden=!this.compatible;
  }
  present(rgba,width,height=240){
    if(rgba.length!==width*height*4)throw new Error('Invalid firmware image size');
    this.frames++;
    // Check surface health before skipping identical pixels. The page also
    // watches surfaceRevision, so a context-loss event invalidates its pane cache.
    if(!this.compatible&&this.context.isContextLost?.())this.setCompatible(true);
    const target=this.compatible?this.buffer:this.canvas;
    const ctx=this.compatible?this.bufferContext:this.context;
    if(ctx.isContextLost?.())throw new Error('Screen drawing surface was lost. Reload the page.');
    if(this.previous&&this.previousWidth===width&&this.previousHeight===height){
      let same=true;
      for(let i=0;i<rgba.length;i++)if(this.previous[i]!==rgba[i]){same=false;break;}
      if(same){this.skipped++;return true;}
    }
    const time=this.now();
    // false means the caller must retry even if the next snapshot is unchanged.
    if(this.compatible&&time-this.last<1000/15){this.skipped++;return false;}
    let visible=false;
    for(let i=0;i<rgba.length;i+=4)if(rgba[i]||rgba[i+1]||rgba[i+2]){visible=true;break;}
    this.black=!visible;
    if(target.width!==width)target.width=width;if(target.height!==height)target.height=height;
    if(!this.frame||this.frame.width!==width||this.frame.height!==height)this.frame={width,height,data:ctx.createImageData(width,height)};
    this.frame.data.data.set(rgba);ctx.putImageData(this.frame.data,0,0);
    if(this.compatible){
      const png=this.buffer.toDataURL('image/png');
      if(!png.startsWith('data:image/png'))throw new Error('The browser could not create the compatible screen image.');
      this.image.src=png;
    }
    if(!this.previous||this.previous.length!==rgba.length)this.previous=new Uint8ClampedArray(rgba.length);
    this.previous.set(rgba);this.previousWidth=width;this.previousHeight=height;
    this.last=time;this.changed++;return true;
  }
  status(){return this.black?' · renderer returned a black frame':this.compatible?' · compatible display':'';}
  metrics(){return {received:this.frames,painted:this.changed,skipped:this.skipped};}
}
