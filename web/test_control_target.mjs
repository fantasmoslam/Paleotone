// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {ControlTarget} from './control_target.js';
import {contextSnapshot,readContext} from './second_screen.js';
const layout=JSON.parse(await readFile(new URL('./remote_views.json',import.meta.url),'utf8'));
const initial={home:1,page:0,track:0,cursor:3,bank:0,lane:0,menu:0,confirm:0,layer:0};
let state={...initial},held=0,failTurn=false;
const turns=[],calls=[];
const link={closed:false,
  async request(cmd,args,match){
    calls.push([cmd,...args]);let p;
    if(cmd===90)p=[state.home,state.page,state.menu,0,state.confirm,state.layer,state.bank,state.cursor,state.lane,0,0,0];
    else if(cmd===27){if(args.length)state.track=args[0];p=[state.track,4];}
    else throw new Error('Unexpected command');
    assert.ok(match(p));return p;
  },
  async inputCommand(args){
    calls.push([94,...args]);
    if(args[0]===2){held=0;return;}
    if(args[0]===1){
      if(failTurn){failTurn=false;throw new Error('lost acknowledgement');}
      turns.push({...state,role:args[1],delta:args[2]-64});
      if(args[1]===3&&layout.pages.find(p=>p.page===state.page)?.label==='STEP')state.cursor=Math.max(0,Math.min(63,state.cursor+args[2]-64));
      return;
    }
    const mask=args[1]|args[2]<<7;
    if(held&&!mask){
      const button=Math.log2(held),family=layout.pages.filter(p=>p.button===button),at=family.findIndex(p=>p.page===state.page);
      if(button===6){
        if(state.home){state.home=0;state.page=family[0].page;}
        else if(at>=0&&at+1<family.length)state.page=family[at+1].page;
        else state.home=1;
      }else{state.home=0;state.page=family[at<0?0:(at+1)%family.length].page;}
    }
    held=mask;
  }
};
const router=new ControlTarget(link,layout,{delay:async()=>{}});
router.contexts.left={...initial};
router.contexts.right={...initial,home:0,page:layout.views.find(v=>v.id==='envelope').page,track:2};
await router.select('right');assert.equal(state.track,2);assert.equal(state.home,0);
assert.equal(router.contexts.left.track,0);assert.equal(router.contexts.left.home,1);
await router.turn(3,1,'right');assert.equal(turns.at(-1).track,2);assert.equal(turns.at(-1).page,0);
await router.select('left');assert.equal(state.home,1);assert.equal(state.track,0);
await Promise.all([router.turn(4,1,'right'),router.turn(5,-1,'left')]);
assert.equal(turns.at(-2).track,2);assert.equal(turns.at(-1).track,0);assert.equal(turns.at(-1).home,1);
const before=calls.length;failTurn=true;
await assert.rejects(router.turn(4,1,'right'),/lost acknowledgement/);
assert.equal(calls.slice(before).filter(c=>c[0]===94&&c[1]===1).length,1,'never replay an uncertain encoder turn');
await router.select('left');state.menu=1;
await assert.rejects(router.select('right'),/Close the FM-1 menu/);assert.equal(router.active,'left');assert.equal(router.busy,false);
state.menu=0;
router.naming=true;await assert.rejects(router.select('right'),/Finish naming/);router.naming=false;
router.contexts.right={...initial,home:0,page:layout.views.find(v=>v.id==='sequencer').page,track:1,cursor:29};
await router.select('right');assert.equal(state.cursor,29);
await router.buttons(1<<2,'right');await router.clear();assert.equal(held,0);
const bytes=new Uint8Array(16384),ctx={home:0,page:2,track:3,cursor:17,bank:1,lane:0};
const projected=contextSnapshot(bytes,layout,ctx);
assert.deepEqual(readContext(projected,layout),ctx);assert.ok(bytes.every(x=>x===0));
link.closed=true;await assert.rejects(router.turn(3,1,'right'),/Disconnected/);
console.log('control targeting: page/track restore, serialized gestures, held release, modal guard, cursor restore, and non-retried turns passed');
