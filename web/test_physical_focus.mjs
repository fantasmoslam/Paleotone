// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {PhysicalFocus} from './physical_focus.js';
const calls=[];let side=0,rc=0;
const contexts=[[1,0,0,1,17,2,0,0],[0,2,3,0,9,1,1,1]];
const link={async request(cmd,args,match){calls.push([cmd,...args]);const p=[2,rc,side,0,0,0,...contexts.flat()];assert.ok(match(p));return p;}};
const ex={web_panel_side:()=>side,web_panel_context:(side,field)=>contexts[side][field]};
const focus=new PhysicalFocus(link,ex);
await focus.poll(true);assert.equal(focus.snapshot().side,'left');
side=1;assert.equal(focus.snapshot().side,'right');assert.equal(focus.snapshot().left.cursor,17);
assert.equal(focus.snapshot().right.track,3);assert.equal(focus.snapshot().right.seqMode,1);
await focus.configure({home:0,page:7},2);assert.deepEqual(calls.at(-1),[96,2,0,7,2]);
await focus.poll(false);assert.deepEqual(calls.at(-1),[96,0]);
assert.ok(calls.every(c=>c[0]===96),'no browser-generated track changes, button taps or encoder turns');
rc=1;await assert.rejects(focus.configure({home:1,page:0},0),/Close the FM-1/);
console.log('device focus: atomic snapshot contexts, manual assignment, lease and zero synthesized panel actions passed');
