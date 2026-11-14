// SPDX-License-Identifier: GPL-3.0-only
// Positions are CSS pixels: the fullscreen layout grows with its actual viewport.
export function tabletLayout(width, height) {
  const w=Math.max(736,width),wide=w>=900&&w>height,gap=12;
  const compactKeys=false;
  const minTop=wide?312:540;
  const keyW=Math.max(736,Math.min(w,(height-minTop-gap)*928/256));
  const keyH=fm1Keybed(keyW).height;
  const h=Math.max(height,minTop+gap+keyH);
  const top=h-keyH-gap,landscape=wide;
  let screen,knobs,grid,oct;
  if(landscape){
    const left=w*.22,side=Math.min(top-2*gap,w*.31),sx=w*.23,right=sx+side+gap,rw=w-right-gap;
    const r=Math.min(44,left/6,top*.12),rr=Math.min(44,rw/12,top*.11);
    screen=[sx,(top-side)/2,side];
    knobs=[[left*.25,top*.30,r],[left*.75,top*.30,r],[left*.25,top*.73,r],[left*.75,top*.73,r]];
    for(let i=0;i<4;i++)knobs.push([right+(i+.5)*rw/4,rr+42,rr]);
    const gy=2*rr+74,bh=Math.min(80,(top-gy-3*gap)/3),bw=(rw-5*gap)/6;
    grid=[right,gy,bw,bh,bw+gap,bh+gap];
    oct=[[right,gy+2*(bh+gap),(rw-gap)/2,bh],[right+(rw+gap)/2,gy+2*(bh+gap),(rw-gap)/2,bh]];
  }else{
    const band=top*.48,side=Math.min(w*.46,band-gap),r=Math.min(42,w*.055,band*.16),rr=Math.min(42,w/12,top*.075);
    screen=[gap,(band-side)/2,side];
    knobs=[[w*.65,band*.25,r],[w*.87,band*.25,r],[w*.65,band*.72,r],[w*.87,band*.72,r]];
    for(let i=0;i<4;i++)knobs.push([(i+.5)*w/4,band+top*.10,rr]);
    const gy=band+top*.20,bh=Math.min(80,(top-gy-3*gap)/3),bw=(w-7*gap)/6;
    grid=[gap,gy,bw,bh,bw+gap,bh+gap];
    oct=[[gap,gy+2*(bh+gap),(w-3*gap)/2,bh],[2*gap+(w-3*gap)/2,gy+2*(bh+gap),(w-3*gap)/2,bh]];
  }
  return {w,h,screen,knobs,grid,oct,trays:[],keybed:[(w-keyW)/2,top+gap,keyW,keyH],compactKeys,tablet:true};
}

// One uniform scale for the entire original FM-1 keypad, including its gaps.
export function fm1Keybed(width){
  const scale=width/928;
  return {height:256*scale,padding:12*scale,pitch:(928-24)/16*scale,keyWidth:(928-24)/16*.78*scale,keyHeight:256*.425*scale};
}
