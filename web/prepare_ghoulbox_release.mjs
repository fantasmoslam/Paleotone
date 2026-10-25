// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs/promises';import {resolve,join} from 'node:path';
import {Package,sha256hex} from './fm1pkg.js';import {patchSupportedPackage} from './firmware_patch.js';
const [originalPath,appPath,companionPath,outPath,source]=process.argv.slice(2),out=resolve(outPath),comp=resolve(companionPath);
if(!/^[0-9a-f]{40}$/.test(source))throw Error('Expected complete integration source commit');
const original=new Uint8Array(await fs.readFile(originalPath)),app=new Uint8Array(await fs.readFile(appPath));
const inputHash='92630ae278d4a0b4233ff2512b62ad9b5dbe976a4c9083046222d9293a8df749';
if(await sha256hex(original)!==inputHash)throw Error('Not the official GHOULBOX 2.4 release');
const layout=JSON.parse(await fs.readFile(join(comp,'remote_views.json'),'utf8')),pkg=new Package(original);pkg.setApp(app);const expected=pkg.serialize();
const recipe={name:'GHOULBOX 2.4 + display',source,upstreamSource:'9f2b9a22e456d7fe9552b40f7a1d0c13200913bb',product:'FM-1_900',rendererId:layout.rendererId,schema:layout.schema,
 input:{file:'ghoulbox-2.4.fwsc',size:original.length,sha256:inputHash},app:{size:app.length,sha256:await sha256hex(app)},output:{size:expected.length,sha256:await sha256hex(expected)}};
const patched=await patchSupportedPackage(original,app,recipe),checked=new Package(patched),before=new Package(original);
if(!Buffer.from(checked.dec.subarray(checked.appOff,checked.appOff+app.length)).equals(Buffer.from(app)))throw Error('Patched app differs');
if(!Buffer.from(checked.flash.subarray(0,checked.appBase)).equals(Buffer.from(before.flash.subarray(0,before.appBase))))throw Error('Boot region differs');
for(const entry of before.entries.filter(e=>e.type!==0)){
 const lo=entry.dataOff+before.skew,hi=lo+entry.size;
 if(!Buffer.from(patched.subarray(lo,hi)).equals(Buffer.from(original.subarray(lo,hi))))throw Error('Non-application package entry differs: '+entry.name);
}
for(const corrupt of [original.slice(0,-1),Uint8Array.from(original,(v,i)=>i===100?v^1:v)]){
 try{await patchSupportedPackage(corrupt,app,recipe);throw Error('Bad input accepted');}catch(e){if(!/exact supported/.test(e.message))throw e;}
}
for(const badRecipe of [{...recipe,app:{...recipe.app,sha256:'0'.repeat(64)}},{...recipe,output:{...recipe.output,sha256:'0'.repeat(64)}}]){
 let rejected=false;try{await patchSupportedPackage(original,app,badRecipe);}catch{rejected=true;}if(!rejected)throw Error('Bad integration accepted');
}
const setup=join(out,'webapp/ghoulbox-setup'),screen=join(out,'webapp/screen-ghoulbox');await fs.mkdir(setup,{recursive:true});await fs.mkdir(screen,{recursive:true});
const token=recipe.output.sha256.slice(0,12),version=text=>text.replace(/(from\s+['"]\.\/[^'"?]+\.js)(['"])/g,'$1?v='+token+'$2');
for(const name of ['firmware_patch.js','firmware_support.js','fm1pkg.js','fm1ota.js','remote_screen.js'])await fs.writeFile(join(setup,name),version(await fs.readFile(new URL(name,import.meta.url),'utf8')));
let html=version(await fs.readFile(new URL('ghoulbox-setup.html',import.meta.url),'utf8')).replace("fetch('recipe.json')","fetch('recipe.json?v="+token+"')").replace("fetch('display-app.bin')","fetch('display-app.bin?v="+token+"')");
await fs.writeFile(join(setup,'index.html'),html);await fs.writeFile(join(setup,'recipe.json'),JSON.stringify(recipe,null,2)+'\n');await fs.writeFile(join(setup,'display-app.bin'),app);
for(const name of ['remote_screen.js','second_screen.js','physical_focus.js','frame_presenter.js','display_performance.js','control_target.js'])await fs.writeFile(join(screen,name),version(await fs.readFile(join(comp,name),'utf8')));
for(const name of ['felucca.wasm','remote_views.json'])await fs.copyFile(join(comp,name),join(screen,name));
html=version(await fs.readFile(join(comp,'screen.html'),'utf8')).replace("fetch('felucca.wasm')","fetch('felucca.wasm?v="+token+"')").replace("fetch('remote_views.json')","fetch('remote_views.json?v="+token+"')").replace('</header>','</header><p class="help">GHOULBOX 2.4 display integration · <a href="../ghoulbox-setup/">Package patcher and installer</a> · <a href="ghoulbox-display.fwsc">Download paired firmware</a></p>');
await fs.writeFile(join(screen,'index.html'),html);await fs.writeFile(join(screen,'ghoulbox-display.fwsc'),patched);
await fs.writeFile(join(screen,'build.json'),JSON.stringify({source,rendererId:recipe.rendererId,schema:recipe.schema,kind:'firmware-graphics-companion',presentation:'device-owned-focus',firmware:{file:'ghoulbox-display.fwsc',...recipe.output}},null,2)+'\n');
const manifestPath=join(out,'release.json'),release=JSON.parse(await fs.readFile(manifestPath,'utf8'));
release.supportedBuilds=[{name:recipe.name,rendererId:recipe.rendererId,schema:recipe.schema,display:'webapp/screen-ghoulbox/',firmwareSha256:recipe.output.sha256}];release.patchSources=[{name:'GHOULBOX 2.4',sha256:inputHash}];
await fs.writeFile(manifestPath,JSON.stringify(release,null,2)+'\n');
console.log(JSON.stringify(recipe,null,2));console.log('Patcher verified: exact input/output hashes, corrupt input rejection, exact app, unchanged boot region and unchanged non-flash package entries');
