// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {resolve,join} from 'node:path';
const dir=resolve(process.argv[2]);
const html=await readFile(join(dir,'index.html'),'utf8');
assert.match(html,/type="module"/);
assert.match(html,/remote_screen\.js/);assert.match(html,/second_screen\.js/);
assert.match(html,/frame_presenter\.js/);assert.match(html,/presenter\.present/);assert.match(html,/presenter\.status/);
assert.match(html,/Original/);assert.match(html,/Expanded/);
assert.doesNotMatch(html,/src="\.\/screen\.js"/,'the lightweight display must not occupy the graphics URL');
if(html.includes('display_performance.js'))assert.ok((await readFile(join(dir,'display_performance.js'))).length);
const layout=JSON.parse(await readFile(join(dir,'remote_views.json'),'utf8'));
const metadata=JSON.parse(await readFile(join(dir,'build.json'),'utf8'));
const {instance}=await WebAssembly.instantiate(await readFile(join(dir,'felucca.wasm')),{});
assert.equal(instance.exports.web_remote_id()>>>0,layout.rendererId);
assert.equal(instance.exports.web_remote_schema(),layout.schema);
assert.equal(metadata.rendererId,layout.rendererId);
assert.equal(metadata.kind,'firmware-graphics-companion');
if(metadata.physicalFocusProtocol===2){
  assert.match(html,/PhysicalFocus/);
  assert.match(html,/await focus.poll\(enabled\)/);
  assert.match(html,/focus.snapshot\(\)/);
  assert.doesNotMatch(html,/ControlTarget|shiftTarget/,'physical focus must survive companion packaging');
  for(const f of ['physical_focus.js','display_performance.js'])assert.ok((await readFile(join(dir,f))).length);
}
for(const f of ['remote_screen.js','second_screen.js','control_target.js','frame_presenter.js'])assert.ok((await readFile(join(dir,f))).length);
console.log('published companion: graphics renderer, Original/Expanded, complete assets and matching WASM identity passed');
