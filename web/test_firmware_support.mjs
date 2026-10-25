// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {classifyFirmware,inspectDevice,inspectPackage} from './firmware_support.js';
import {sha256hex} from './fm1pkg.js';
const pkg=new Uint8Array([1,2,3]),old=new Uint8Array([4,5,6]);
const release={rendererId:2540340739,schema:2,previousRenderers:[695244803],firmware:{sha256:await sha256hex(pkg)},previousPackages:[await sha256hex(old)]};
const identity={model:'FM-1',version:900,text:'FM-1_900'};
assert.equal(classifyFirmware(identity,{id:2540340739,schema:2},release).kind,'ready');
assert.equal(classifyFirmware(identity,{id:2540340739,schema:1},release).kind,'unsupported');
assert.equal(classifyFirmware(identity,{id:695244803,schema:2},release).kind,'upgrade');
assert.equal(classifyFirmware(identity,{id:7,schema:2},release).kind,'unsupported');
assert.equal(classifyFirmware(identity,null,release).kind,'unsupported');
assert.equal(classifyFirmware({...identity,model:'SMK-37'},null,release).kind,'wrong-model');
assert.equal((await inspectPackage(pkg,release)).kind,'ready');
assert.equal((await inspectPackage(old,release)).kind,'upgrade');
assert.equal((await inspectPackage(new Uint8Array([7]),release)).kind,'unsupported');
let closes=0,opened=0,queries=[],device={id:identity,link:{input:{open:async()=>opened++},output:{open:async()=>opened++},close:()=>closes++}};
class FakeUpdater {async find(){return device;} install(){throw Error('Must never install during inspection');} resume(){throw Error('Must never flash during inspection');}}
class FakeLink {async status(op){queries.push(op);return {id:2540340739,schema:2};}close(){closes++;}}
const deps={UpdaterClass:FakeUpdater,LinkClass:FakeLink};
assert.equal((await inspectDevice({},release,deps)).kind,'ready');assert.deepEqual(queries,[0]);assert.equal(opened,2);assert.equal(closes,2);
class SilentLink extends FakeLink {async status(){throw Object.assign(Error('timeout'),{code:'MIDI_TIMEOUT'});}}
assert.equal((await inspectDevice({},release,{...deps,LinkClass:SilentLink})).kind,'unsupported');
device={...device,id:{...identity,model:'SMK-37'}};queries=[];
assert.equal((await inspectDevice({},release,deps)).kind,'wrong-model');assert.deepEqual(queries,[]);
device=null;await assert.rejects(inspectDevice({},release,deps),/No FM-1/);
console.log('firmware support: exact identity, schema, package hashes, unknown builds, wrong model and strictly read-only detection passed');
