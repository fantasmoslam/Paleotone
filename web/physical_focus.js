// SPDX-License-Identifier: GPL-3.0-only
// The instrument owns focus and navigation. The browser only sets a page manually
// or renews the gesture lease; it never synthesizes button taps to swap sides.
export class PhysicalFocus {
  constructor(link,ex){this.link=link;this.ex=ex;}
  async request(args){
    const p=await this.link.request(96,args,p=>p.length===22&&p[0]===2&&p[2]<=1);
    if(p[1])throw new Error('Close the FM-1 menu or dialog and release any editing layer before assigning a page.');
    return p;
  }
  poll(enabled){return this.request([enabled?1:0]);}
  configure(view,track){return this.request([2,view.home,view.page,Number(track)]);}
  snapshot(){
    const fields=['home','page','track','bank','cursor','lane','seqMode','grid'];
    const contexts=[0,1].map(side=>Object.fromEntries(fields.map((field,i)=>[field,this.ex.web_panel_context(side,i)])));
    return {side:this.ex.web_panel_side()?'right':'left',left:contexts[0],right:contexts[1]};
  }
}
