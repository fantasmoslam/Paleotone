// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import {resolve,join,dirname} from 'node:path';
import {createHash,webcrypto} from 'node:crypto';
import vm from 'node:vm';
import {productOf,logicalImage} from './fm1pkg.js';
const root=resolve(process.argv[2]),read=p=>fs.readFile(join(root,p),'utf8');
const build=JSON.parse(await read('build.json')),pkg=await fs.readFile(join(root,build.firmware.file));
assert.equal(productOf(pkg),'FM-1_900');assert.ok(logicalImage(pkg).length);
assert.equal(createHash('sha256').update(pkg).digest('hex'),build.firmware.sha256);
for(const sub of ['', 'screen/']) {
  const layout=JSON.parse(await read(sub+'remote_views.json'));
  const {instance}=await WebAssembly.instantiate(await fs.readFile(join(root,sub+'felucca.wasm')),{});
  assert.equal(instance.exports.web_remote_id()>>>0,build.rendererId);
  assert.equal(layout.rendererId,build.rendererId);assert.equal(layout.schema,build.schema);
}
assert.match(await read('screen/index.html'),/remote_views\.json\?v=/);
const html=await read('index.html');assert.match(html,/paleotone-scape-preview-flash/);assert.doesNotMatch(html,/ghoulbox-emu-flash/);
assert.match(html,/SCAPE candidate preview/);assert.match(html,/worklet\.js\?v=/);assert.match(html,/felucca\.wasm\?v=/);
for(const page of ['index.html','installer/index.html','editor/index.html','manual/index.html','screen/index.html']) {
  const html=await read(page);
  for(const [,href]of html.matchAll(/(?:href|src)="([^"#]+)"/g)) {
    if(/^(https?:|mailto:|data:|blob:)/.test(href))continue;
    const target=resolve(root,dirname(page),href.split(/[?#]/)[0]);
    assert.ok(target.startsWith(root),page+': escaped preview '+href);
    assert.ok(await fs.stat(target),page+': missing '+href);
  }
}
const installer=await read('installer/index.html'),meta=JSON.parse(installer.match(/const meta = (\{[^\n]+\});/)[1]);
assert.equal(meta.sha256,build.firmware.sha256);assert.match(meta.pkg,/^\.\.\/paleotone-scape\.fwsc\?v=/);
assert.match(installer,/has not yet been tested on a physical FM-1/);assert.doesNotMatch(installer,/has been tested on an FM-1 with compressed/);
const script=installer.match(/<script(?: type="module")?>([\s\S]*?)<\/script>/)[1];new vm.Script(script);
for(const bytes of [pkg,new Uint8Array([1,2,3])]) {
  const elements=new Map(),el=id=>{if(!elements.has(id))elements.set(id,{disabled:true,addEventListener(){},textContent:''});return elements.get(id);};let midi=0;
  const context=vm.createContext({document:{getElementById:el,querySelectorAll:()=>[],documentElement:{}},window:{addEventListener(){}},navigator:{language:'en',requestMIDIAccess(){midi++;throw Error('Unexpected MIDI');}},fetch:async()=>({ok:true,arrayBuffer:async()=>bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength)}),crypto:webcrypto,TextEncoder,TextDecoder,Uint8Array,Uint16Array,URL,Blob,setTimeout,clearTimeout,sessionStorage:{getItem:()=>null,removeItem(){}}});
  vm.runInContext(script.replace('(async () => {','this.loading=(async () => {'),context);await context.loading;
  assert.equal(el('go').disabled,bytes!==pkg);assert.equal(midi,0);
}
console.log('SCAPE preview: matched firmware/WASM/display, isolated saves, complete local links/assets, exact-hash installer and zero MIDI on load passed');
