// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';import fs from 'node:fs/promises';import {resolve,join} from 'node:path';import {createHash,webcrypto} from 'node:crypto';import vm from 'node:vm';
import {productOf,logicalImage} from './fm1pkg.js';
const root=resolve(process.argv[2]),read=p=>fs.readFile(join(root,p),'utf8');
const release=JSON.parse(await read('release.json')),pkg=await fs.readFile(join(root,release.firmware.file));
assert.equal(createHash('sha256').update(pkg).digest('hex'),release.firmware.sha256);assert.equal(pkg.length,release.firmware.size);assert.equal(productOf(pkg),release.product);assert.ok(logicalImage(pkg).length);
const inst=await read('webapp/installer/index.html');assert.doesNotMatch(inst,/\/\*(LIB|META)\*\//);assert.match(inst,/await sha256hex\(pkg\)/);assert.match(inst,/const RESUME_KEY=/);
const meta=JSON.parse(inst.match(/const meta = (\{[^\n]+\});/)[1]);assert.equal(meta.sha256,release.firmware.sha256);assert.equal(meta.product,release.product);assert.equal(meta.pkg.split('?')[0],'../../'+release.firmware.file);
const installerScript=inst.match(/<script(?: type="module")?>([\s\S]*?)<\/script>/)[1];
new vm.Script(installerScript);
for(const [bytes,enabled] of [[pkg,true],[new Uint8Array([1,2,3]),false]]){
 const elements=new Map(),element=id=>{if(!elements.has(id))elements.set(id,{disabled:true,addEventListener(){},textContent:''});return elements.get(id);};let midiCalls=0;
 const context=vm.createContext({document:{getElementById:element,querySelectorAll:()=>[],documentElement:{}},window:{addEventListener(){}},navigator:{language:'en',requestMIDIAccess(){midiCalls++;throw Error('Unexpected MIDI access');}},fetch:async()=>({ok:true,arrayBuffer:async()=>bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength)}),crypto:webcrypto,TextEncoder,TextDecoder,Uint8Array,Uint16Array,URL,Blob,setTimeout,clearTimeout,sessionStorage:{getItem:()=>null,removeItem(){}}});
 vm.runInContext(installerScript.replace('(async () => {','this.loading=(async () => {'),context);await context.loading;
 assert.equal(element('go').disabled,!enabled);assert.equal(midiCalls,0,'Loading an installer must not connect or flash');
 if(!enabled)assert.match(element('status').textContent,/SHA-256/);
}
const emu=await read('webapp/emu/index.html');assert.match(emu,/unified SCAPE \+ SAURUS 2/);assert.match(emu,/felucca\.wasm\?v=/);assert.match(emu,/worklet\.js\?v=/);assert.match(emu,/app-setup/);assert.doesNotMatch(emu,/@GBVERSION@|@VERSION@/);
const {instance}=await WebAssembly.instantiate(await fs.readFile(join(root,'webapp/emu/felucca.wasm')),{});assert.equal(instance.exports.web_remote_id()>>>0,release.rendererId);assert.equal(instance.exports.web_remote_schema(),release.schema);
const companion=JSON.parse(await read('webapp/screen-unified/build.json'));assert.equal(companion.rendererId,release.rendererId);assert.equal(companion.firmware.sha256,release.firmware.sha256);
const setup=await read('webapp/app-setup/index.html');assert.match(setup,/does not patch or install/);assert.match(setup,/source/);assert.match(setup,/firmware_support\.js\?v=/);
for(const name of ['worklet.js','emu_io.js','audio_session.js','audio_encode.js','audio_encode_worker.js','audio_export.js','fonts/DotGothic16-subset.woff','vendor/libflac/libflac.min.wasm.wasm','vendor/mp3/mp3.wasm'])assert.ok((await fs.stat(join(root,'webapp/emu',name))).size);
for(const page of ['index.html','webapp/installer/index.html','webapp/emu/index.html','webapp/app-setup/index.html']){
 const html=await read(page);for(const [,href] of html.matchAll(/href="([^"#]+)"/g)){
  if(/^(https?:|mailto:|data:|blob:)/.test(href))continue;
  const path=resolve(root,page,'..',href.split(/[?#]/)[0]);assert.ok(path.startsWith(root));assert.ok(await fs.stat(path),`${page}: missing ${href}`);
 }
}
console.log('site release: exact firmware hash, executable installer, matched emulator/display, complete audio assets, honest compatibility flow and local links passed');
