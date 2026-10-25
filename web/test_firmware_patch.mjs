// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';import fs from 'node:fs/promises';
import {patchSupportedPackage} from './firmware_patch.js';import {sha256hex,Package} from './fm1pkg.js';
const input=new Uint8Array([1,2,3]),app=new Uint8Array([4,5]),before=input.slice();
const recipe={product:'SMK-37_900',input:{size:3,sha256:await sha256hex(input)},app:{size:2,sha256:await sha256hex(app)},output:{sha256:'0'.repeat(64),size:3}};
await assert.rejects(patchSupportedPackage(new Uint8Array([1]),app,recipe),/exact supported/);
await assert.rejects(patchSupportedPackage(input,new Uint8Array([4,6]),recipe),/failed verification/);
await assert.rejects(patchSupportedPackage(input,app,recipe),/Wrong device model/);assert.deepEqual(input,before);
if(process.argv[2]){
 const original=new Uint8Array(await fs.readFile(process.argv[2])),binary=new Uint8Array(await fs.readFile(process.argv[3])),r=JSON.parse(await fs.readFile(process.argv[4],'utf8'));
 const saved=original.slice(),patched=await patchSupportedPackage(original,binary,r);assert.deepEqual(original,saved);
 assert.equal(await sha256hex(patched),r.output.sha256);const parsed=new Package(patched);
 assert.deepEqual(parsed.dec.subarray(parsed.appOff,parsed.appOff+binary.length),binary);
 const wrong={...r,output:{...r.output,sha256:'0'.repeat(64)}};await assert.rejects(patchSupportedPackage(original,binary,wrong),/patched package failed/);
 const damaged=original.slice();damaged[100]^=1;await assert.rejects(patchSupportedPackage(damaged,binary,r),/exact supported/);
 console.log('GHOULBOX patch: exact original/app/output hashes, app bytes, input immutability and corruption rejection passed');
}
console.log('firmware patch: unsupported input, corrupted integration and wrong-model rejection passed');
