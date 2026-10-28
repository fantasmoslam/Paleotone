// SPDX-License-Identifier: GPL-3.0-only
// Preserve exact firmware pixels; avoid redundant canvas writes and expensive PNG encoding.
export class FramePresenter {
  constructor(canvas,{doc=document,mobile=/Android/i.test(navigator.userAgent),now=()=>performance.now()}={}){
    this.canvas=canvas;this.doc=doc;this.now=now;this.last=-Infinity;this.black=false;
    this.buffer=doc.createElement('canvas');
    this.image=doc.createElement('img');this.image.id='screen-image';this.image.alt='FM-1 live screen';this.image.hidden=true;
    canvas.insertAdjacentElement('afterend',this.image);
    this.compatible=mobile;
    this.context=canvas.getContext('2d',{alpha:false,willReadFrequently:true});
    this.bufferContext=this.buffer.getContext('2d',{alpha:false,willReadFrequently:true});
    if(!this.context||!this.bufferContext)throw new Error('This browser could not create the screen drawing surface.');
    this.frames=0;this.skipped=0;this.changed=0;
    this.frame=null;this.frameWidth=0;this.frameHeight=0;
    this.previous=null;this.previousWidth=0;this.previousHeight=0;
    canvas.addEventListener('contextlost',event=>{event.preventDefault();this.setCompatible(true);});
    this.setCompatible(mobile);
  }
  setCompatible(on){
    this.compatible=!!on;this.last=-Infinity;this.previous=null;
    this.canvas.hidden=this.compatible;this.image.hidden=!this.compatible;
  }
  present(rgba,width,height=240){
    if(rgba.length!==width*height*4)throw new Error('Invalid firmware image size');
    this.frames++;
    // Compare full pixels, not a lossy sampled hash: one-pixel changes must remain visible.
    let same=this.previous && this.previousWidth===width && this.previousHeight===height;
    if(same)for(let i=0;i<rgba.length;i++){if(this.previous[i]!==rgba[i]){same=false;break;}}
    if(same){this.skipped++;return;}
    // A throttled compatible frame must not replace the last *displayed* frame in the cache.
    const time=this.now();
    if(this.compatible&&time-this.last<90){this.skipped++;return;}
    if(this.context.isContextLost?.())this.setCompatible(true);
    const target=this.compatible?this.buffer:this.canvas;
    const ctx=this.compatible?this.bufferContext:this.context;
    if(target.width!==width)target.width=width;
    if(target.height!==height)target.height=height;
    if(ctx.isContextLost?.())throw new Error('Screen drawing surface was lost. Reload the page.');
    if(!this.frame||this.frameWidth!==width||this.frameHeight!==height){
      this.frame=ctx.createImageData(width,height);this.frameWidth=width;this.frameHeight=height;
    }
    this.frame.data.set(rgba);
    ctx.putImageData(this.frame,0,0);
    if(this.compatible){
      const png=this.buffer.toDataURL('image/png');
      if(!png.startsWith('data:image/png'))throw new Error('The browser could not create the compatible screen image.');
      this.image.src=png;
    }
    if(!this.previous||this.previous.length!==rgba.length)this.previous=new Uint8ClampedArray(rgba.length);
    this.previous.set(rgba);this.previousWidth=width;this.previousHeight=height;
    let visible=false;
    for(let i=0;i<rgba.length;i+=4)if(rgba[i]||rgba[i+1]||rgba[i+2]){visible=true;break;}
    this.black=!visible;this.last=time;this.changed++;
  }
  status(){return this.black?' · renderer returned a black frame':this.compatible?' · compatible display':'';}
  metrics(){return {received:this.frames,painted:this.changed,skipped:this.skipped};}
}
