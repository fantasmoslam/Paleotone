// SPDX-License-Identifier: GPL-3.0-only
// Exercise the page's physical-focus callback with the real module wiring.
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
class Element extends EventTarget {
  constructor(tag='DIV'){super();this.tagName=tag;this.children=[];this.attributes={};this.classList={add(){},remove(){},toggle(){}};}
  append(...children){this.children.push(...children);}
  setAttribute(k,v){this.attributes[k]=String(v);}
  getContext(){return {};}
}
const elements=new Map();
globalThis.document={body:new Element('BODY'),getElementById(id){if(!elements.has(id))elements.set(id,new Element());return elements.get(id);},createElement:tag=>new Element(tag.toUpperCase()),querySelectorAll:()=>[],addEventListener(){}};
globalThis.addEventListener=()=>{};globalThis.localStorage={getItem:()=>null,setItem:()=>{}};
const html=await readFile(new URL('./screen.html',import.meta.url),'utf8');
assert.doesNotMatch(html,/id="target-(left|right|shift)"/);
assert.match(html,/Hold HOME, tap OCT− for LEFT or OCT\+ for RIGHT/);
assert.match(html,/await focus.poll\(wide&&!document.hidden\)/);
let script=html.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
script=script.replace(/from (["'])(\.\/[^"']+)\1/g,(_m,_q,path)=>'from '+JSON.stringify(new URL(path,import.meta.url).href));
script+='\nexport {mode,effectiveTarget,chooseTarget,stop}; export function attachForTest(value){router=value;running=true;}';
const page=await import('data:text/javascript;base64,'+Buffer.from(script).toString('base64'));
const selected=[],router={active:'left',contexts:{right:{home:0,page:0,track:0}},busy:false,clear:async()=>{},select:async side=>{selected.push(side);router.active=side;}};
page.attachForTest(router);page.mode(true);
await page.chooseTarget('right');assert.equal(page.effectiveTarget(),'right');assert.equal(selected.at(-1),'right');
assert.equal(elements.get('target-status').textContent,'FM-1 controls: RIGHT');
await page.chooseTarget('left');assert.equal(selected.at(-1),'left');
await page.chooseTarget('right');page.mode(false);await new Promise(resolve=>setImmediate(resolve));
assert.equal(page.effectiveTarget(),'left');assert.equal(selected.at(-1),'left');
page.stop();assert.equal(page.effectiveTarget(),'left');
console.log('physical control widget: FM-1 shortcut instructions, callback routing, focus indication and Original reset passed');
