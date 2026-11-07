// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual page event wiring without opening MIDI or a browser.
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
class Element extends EventTarget {
  constructor(tag='DIV'){super();this.tagName=tag;this.children=[];this.attributes={};const classes=new Set();this.classList={add:x=>classes.add(x),remove:x=>classes.delete(x),toggle:(x,on)=>on?classes.add(x):classes.delete(x)};}
  append(...children){this.children.push(...children);}
  setAttribute(k,v){this.attributes[k]=String(v);}
  setPointerCapture(){}
  getContext(){return {};}
}
const elements=new Map(),windowEvents=new EventTarget();
globalThis.document={body:new Element('BODY'),getElementById(id){if(!elements.has(id))elements.set(id,new Element());return elements.get(id);},createElement:tag=>new Element(tag.toUpperCase()),querySelectorAll:()=>[],addEventListener:()=>{}};
globalThis.addEventListener=(...args)=>windowEvents.addEventListener(...args);
globalThis.localStorage={getItem:()=>null,setItem:()=>{}};
const html=await readFile(new URL('./screen.html',import.meta.url),'utf8');
let script=html.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
script=script.replace(/from (["'])(\.\/[^"']+)\1/g,(_m,_q,path)=>'from '+JSON.stringify(new URL(path,import.meta.url).href));
script+='\nexport {mode,effectiveTarget,shiftTarget,stop}; export function attachForTest(value){router=value;running=true;}';
const page=await import('data:text/javascript;base64,'+Buffer.from(script).toString('base64'));
const selected=[],router={active:'left',contexts:{right:{home:0,page:0,track:0}},busy:false,clear:async()=>{},select:async side=>{selected.push(side);router.active=side;}};
page.attachForTest(router);page.mode(true);
assert.equal(page.effectiveTarget(),'left');
elements.get('target-right').onclick();assert.equal(page.effectiveTarget(),'right');assert.equal(selected.at(-1),'right');
page.shiftTarget(true);assert.equal(page.effectiveTarget(),'left');assert.equal(selected.at(-1),'left');
page.shiftTarget(false);assert.equal(page.effectiveTarget(),'right');
elements.get('target-left').onclick();
function key(type,fields){const event=new Event(type);Object.assign(event,{key:'Shift',repeat:false,ctrlKey:false,altKey:false,metaKey:false,...fields});windowEvents.dispatchEvent(event);}
key('keydown');assert.equal(page.effectiveTarget(),'right');
key('keyup');assert.equal(page.effectiveTarget(),'left');
key('keydown',{ctrlKey:true});assert.equal(page.effectiveTarget(),'left','browser shortcuts must not switch sides');
elements.get('target-shift').onpointerdown({pointerId:1});assert.equal(page.effectiveTarget(),'right');
elements.get('target-shift').dispatchEvent(new Event('pointercancel'));assert.equal(page.effectiveTarget(),'left');
elements.get('target-shift').onkeydown({key:' ',repeat:false,preventDefault(){}});assert.equal(page.effectiveTarget(),'right');
elements.get('target-shift').onblur();assert.equal(page.effectiveTarget(),'left');
page.shiftTarget(true);windowEvents.dispatchEvent(new Event('blur'));assert.equal(page.effectiveTarget(),'left');
elements.get('target-right').onclick();page.mode(false);assert.equal(page.effectiveTarget(),'left');assert.equal(selected.at(-1),'left');
assert.equal(elements.get('target-controls').hidden,true);
page.stop();assert.equal(page.effectiveTarget(),'left');
console.log('control widget: left/right buttons, keyboard and onscreen Shift, cancellation, blur recovery, and Original-mode reset passed');
