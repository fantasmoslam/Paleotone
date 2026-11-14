// SPDX-License-Identifier: GPL-3.0-only
import assert from "node:assert/strict";
import { tabletLayout, fm1Keybed } from "../../web/nightclaw/tablet-layout.mjs";
const sizes=[[2560,1300],[1920,960],[1366,650],[1280,700],[1024,660],[900,480],[900,1120],[800,1160],[768,920],[768,800],[600,800],[390,740],[320,720]];
function within(x,y,w,h,L,label){assert([x,y,w,h].every(Number.isFinite),label+" finite");assert(x>=-.01&&y>=-.01&&x+w<=L.w+.01&&y+h<=L.h+.01,label+" inside viewport at "+L.w+"x"+L.h);}
for(const [w,h] of sizes){
 const L=tabletLayout(w,h),[sx,sy,ss]=L.screen;
 within(sx,sy,ss,ss,L,"screen");assert(ss>=140,"readable live screen");
 assert.equal(L.trays.length,0,"no hardware shell/trays");
 const [gx,gy,bw,bh,dx,dy]=L.grid;assert(bw>=44&&bh>=44,"page buttons at least 44px at "+w+"x"+h);
 const rectangles=[];
 for(let row=0;row<2;row++)for(let c=0;c<6;c++){const rect=[gx+c*dx,gy+row*dy,bw,bh];within(...rect,L,"page button");rectangles.push(rect);}
 for(const rect of L.oct){within(...rect,L,"octave button");assert(rect[2]>=44&&rect[3]>=44);rectangles.push(rect);}
 for(let i=0;i<rectangles.length;i++)for(let j=i+1;j<rectangles.length;j++){
  const a=rectangles[i],b=rectangles[j];assert(a[0]+a[2]<=b[0]+.01||b[0]+b[2]<=a[0]+.01||a[1]+a[3]<=b[1]+.01||b[1]+b[3]<=a[1]+.01,"button targets do not overlap");
 }
 assert.equal(L.knobs.length,8);
 for(const [x,y,r] of L.knobs){within(x-r,y-r,r*2,r*2,L,"knob");assert((r+12)*2>=44,"knob target large enough");assert(x+r<sx||x-r>sx+ss||y+r<sy||y-r>sy+ss,"knob does not cover screen");}
 const [kx,ky,kw,kh]=L.keybed;within(kx,ky,kw,kh,L,"keybed");
 const pad=fm1Keybed(kw),scale=kw/928;
 assert.equal(L.compactKeys,false,"retain the original two keypad rows; no split banks");
 assert.equal(kh,pad.height,"the keypad area scales with the pads");
 assert(pad.pitch>=44,"key touch cells stay at least 44px wide");assert(kh*.48>=44,"key touch cells stay at least 44px tall");
 assert(Math.abs(pad.keyWidth/pad.pitch-.78)<1e-9,"restore original spacing relative to pad width");
 assert(Math.abs(pad.keyHeight/pad.keyWidth-108.8/44.07)<1e-8,"original elongated pad aspect ratio");
 assert(Math.abs(pad.padding-12*scale)<1e-9,"scale the outer padding uniformly too");
 const whiteX=Array.from({length:16},(_,i)=>pad.padding+(i+.5)*pad.pitch);
 const blackAfter=[0,1,2,4,5,7,8,9,11,12,14];
 for(const i of blackAfter){const x=pad.padding+(i+1)*pad.pitch;assert(Math.abs(x-(whiteX[i]+whiteX[i+1])/2)<1e-9,"upper pads keep their FM-1 stagger");}
 assert(ky>=sy+ss,"keys do not cover screen");
 console.log("Tablet layout: "+w+"x"+h+" passes bounds, target size, screen separation and button spacing.");
}
