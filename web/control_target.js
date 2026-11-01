// SPDX-License-Identifier: GPL-3.0-only
import {u14} from './remote_screen.js';
import {readContext} from './second_screen.js';
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));

// Switch the real panel context using normal panel taps, not raw state writes.
// All gestures and switches share one queue. Encoder/input commands are never retried.
export class ControlTarget {
  constructor(link,layout,{changed=()=>{},delay=sleep}={}) {
    this.link=link;this.layout=layout;this.changed=changed;this.delay=delay;
    this.active='left';this.busy=false;this.contexts={left:null,right:null};this.tail=Promise.resolve();
  }
  observe(bytes) {
    this.naming=bytes[this.layout.grid]===2;
    if(!this.busy)this.contexts[this.active]=readContext(bytes,this.layout);
  }
  configureRight(view,track,bytes) {
    const current=readContext(bytes,this.layout);
    this.contexts.right={...current,home:view.home,page:view.page,track:track==='follow'?current.track:Number(track)};
  }
  run(fn) {
    const task=this.tail.then(()=>{if(this.link.closed)throw new Error('Disconnected');return fn();});
    this.tail=task.catch(()=>{});return task;
  }
  async screen() {
    const p=await this.link.request(90,[],p=>p.length===12);
    return {home:p[0],page:p[1],menu:p[2],confirm:p[4],layer:p[5],bank:p[6],cursor:p[7],lane:p[8]};
  }
  async capture() {
    const screen=await this.screen();
    const track=await this.link.request(27,[],p=>p.length>1&&p[1]===4);
    return {...screen,track:track[0]};
  }
  async tap(button) {
    await this.link.inputCommand([0,...u14(1<<button)]);
    await this.delay(65);
    await this.link.inputCommand([0,0,0]);
    await this.delay(65); // firmware page taps are handled on release
  }
  async navigate(wanted,current) {
    if(current.menu||current.confirm||current.layer)
      throw new Error('Close the FM-1 menu or dialog and release its buttons before switching screens.');
    if(wanted.track!==current.track)
      await this.link.request(27,[wanted.track],p=>p[0]===wanted.track);
    const page=this.layout.pages.find(p=>p.page===wanted.page);
    if(!wanted.home&&!page)throw new Error('This page is unavailable for control targeting.');
    const button=wanted.home?6:page.button;
    const seen=new Set();
    let screen=await this.screen();
    for(;;) {
      if(screen.home===wanted.home&&(wanted.home||screen.page===wanted.page))break;
      const key=screen.home+':'+screen.page;
      if(seen.has(key))throw new Error('That screen is unavailable for the selected track.');
      seen.add(key);
      await this.tap(button);screen=await this.screen();
      if(screen.menu||screen.confirm||screen.layer)throw new Error('Close the FM-1 dialog before switching screens.');
    }
    // Each sequencer panel keeps its own cursor; other pages have no cursor action.
    if(!wanted.home&&page.label==='STEP') {
      for(let attempt=0;screen.cursor!==wanted.cursor&&attempt<64;attempt++) {
        const delta=Math.max(-8,Math.min(8,wanted.cursor-screen.cursor));
        await this.link.inputCommand([1,3,64+delta]);
        await this.delay(20);screen=await this.screen();
      }
      if(screen.cursor!==wanted.cursor)throw new Error('Could not restore the sequencer cursor.');
    }
  }
  async ensure(side,force=false) {
    if(!['left','right'].includes(side))throw new Error('Invalid control target');
    if(side===this.active&&!force)return;
    if(this.naming)throw new Error('Finish naming on the FM-1 before switching screens.');
    const wanted=this.contexts[side];
    if(!wanted)throw new Error('Choose a second-screen page before controlling it.');
    this.busy=true;this.changed();
    try {
      await this.link.inputCommand([2]);
      await this.delay(35);
      const current=await this.capture();
      if(!force)this.contexts[this.active]=current;
      await this.navigate(wanted,current);
      this.active=side;
      this.contexts[side]=await this.capture();
    } finally {this.busy=false;this.changed();}
  }
  select(side,force=false){return this.run(()=>this.ensure(side,force));}
  buttons(mask,side){return this.run(async()=>{await this.ensure(side);await this.link.inputCommand([0,...u14(mask)]);});}
  turn(role,delta,side){return this.run(async()=>{await this.ensure(side);await this.link.inputCommand([1,role,64+delta]);});}
  clear(){return this.run(()=>this.link.inputCommand([2]));}
}
