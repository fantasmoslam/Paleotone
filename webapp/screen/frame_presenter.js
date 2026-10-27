// SPDX-License-Identifier: GPL-3.0-only
// Display the same firmware pixels through either Canvas or a PNG image surface.
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
    canvas.addEventListener('contextlost',event=>{event.preventDefault();this.setCompatible(true);});
    this.setCompatible(mobile);
  }
  setCompatible(on){this.compatible=!!on;this.last=-Infinity;this.canvas.hidden=this.compatible;this.image.hidden=!this.compatible;}
  present(rgba,width,height=240){
    if(rgba.length!==width*height*4)throw new Error('Invalid firmware image size');
    let visible=false;
    for(let i=0;i<rgba.length;i+=4)if(rgba[i]||rgba[i+1]||rgba[i+2]){visible=true;break;}
    this.black=!visible;
    if(this.context.isContextLost?.())this.setCompatible(true);
    const time=this.now();
    if(this.compatible&&time-this.last<180)return;
    const target=this.compatible?this.buffer:this.canvas;
    const ctx=this.compatible?this.bufferContext:this.context;
    if(target.width!==width)target.width=width;if(target.height!==height)target.height=height;
    if(ctx.isContextLost?.())throw new Error('Screen drawing surface was lost. Reload the page.');
    const frame=ctx.createImageData(width,height);frame.data.set(rgba);ctx.putImageData(frame,0,0);
    if(this.compatible){
      const png=this.buffer.toDataURL('image/png');
      if(!png.startsWith('data:image/png'))throw new Error('The browser could not create the compatible screen image.');
      this.image.src=png;
    }
    this.last=time;
  }
  status(){return this.black?' · renderer returned a black frame':this.compatible?' · compatible display':'';}
}
