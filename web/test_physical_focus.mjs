// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {PhysicalFocus} from './physical_focus.js';
const targets=[],calls=[];let response=[1,0,0,0,0],fail=false;
const link={async request(cmd,args,match){calls.push([cmd,...args]);assert.ok(match(response));return response;}};
const focus=new PhysicalFocus(link,async side=>{targets.push(side);if(fail)throw new Error('navigation interrupted');});
await focus.poll(true);assert.deepEqual(targets,[]);
response=[1,1,1,0,1];await focus.poll(true);assert.deepEqual(targets,[]);
response[4]=0;assert.equal(await focus.poll(true),true);assert.deepEqual(targets,['right']);
await focus.poll(true);assert.deepEqual(targets,['right']);
response=[1,0,2,0,0];await focus.poll(true);assert.deepEqual(targets,['right','left']);
response=[1,1,3,0,0];fail=true;await assert.rejects(focus.poll(true),/navigation interrupted/);
fail=false;await focus.poll(true);assert.equal(targets.length,3,'uncertain navigation must not replay');
await focus.poll(false);response=[1,0,4,0,0];await focus.poll(true);assert.equal(targets.length,3,'do not replay an old event when entering Expanded');
focus.serial=16383;response=[1,0,0,0,0];await focus.poll(true);assert.equal(targets.at(-1),'left');
assert.ok(calls.every(c=>c[0]===96&&c.length===2));
console.log('physical focus: release-gated left/right, event deduplication, no stale replay, failure and serial wrap passed');
