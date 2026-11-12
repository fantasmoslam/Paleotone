// SPDX-License-Identifier: GPL-3.0-only
const $ = id => document.getElementById(id);
const COLORS = ["#e6dcc2","#ff284e","#eaff00","#a63aff"];
const STORE = "nightclaw-browser-v1";
const WHITE = [0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26];
const BLACK = [1,3,5,8,10,13,15,17,20,22,25], AFTER = [0,1,2,4,5,7,8,9,11,12,14];
const WHITE_CODES = "QWERTYUIASDFGHJK".split(""), BLACK_CODES = "1234567890-".split("");
let ac, engine, gain, state, scenes = [], starting = false, started = false, muted = false;
let voiceSignature = "", storageFailed = false;
const held = new Map(), keyButtons = [], parameterEls = [];
function send(type, data = {}) { engine?.port.postMessage({ type, ...data }); }
function tell(text) { $("status").textContent = text; }
function releaseAll() { held.clear(); syncKeys(); }
function syncKeys() {
  let mask = 0; for (const n of held.values()) mask |= 1 << n;
  send("keys", { mask });
  keyButtons.forEach((b,i) => { if (b) { b.classList.toggle("down", !!(mask & (1 << i))); b.setAttribute("aria-pressed", String(!!(mask & (1 << i)))); } });
}
function act(type, value) { releaseAll(); send(type, { value }); }
function loadStore() {
  try {
    const raw = JSON.parse(localStorage.getItem(STORE) || "{}"), data = {};
    for (const [o,b] of Object.entries(raw)) {
      if (!/^[0-3]$/.test(o) || typeof b !== "string" || b.length > 5120) continue;
      data[o] = Uint8Array.from(atob(b), c => c.charCodeAt(0)).buffer;
    }
    return data;
  } catch { return {}; }
}
function saveStore(data) {
  try {
    const raw = {};
    for (const [o,b] of Object.entries(data)) { let s = ""; for (const c of b) s += String.fromCharCode(c); raw[o] = btoa(s); }
    localStorage.setItem(STORE, JSON.stringify(raw)); storageFailed = false;
    $("storage-note").textContent = "Saved in this browser. Your physical FM-1 is unchanged.";
  } catch {
    storageFailed = true;
    $("storage-note").textContent = "Browser storage is unavailable. Edits will last only for this open session.";
  }
}
const screenContext = $("screen").getContext("2d"), screenImage = screenContext.createImageData(240,240);
function drawScreen(fb) {
  const d = screenImage.data;
  for (let i=0; i<240*240; i++) {
    const v=fb[i], p=((v>>8)|(v<<8))&65535;
    d[i*4]=((p>>11)&31)*255/31; d[i*4+1]=((p>>5)&63)*255/63; d[i*4+2]=(p&31)*255/31; d[i*4+3]=255;
  }
  screenContext.putImageData(screenImage,0,0);
}
for (let i=0;i<4;i++) {
  const card=document.createElement("div"); card.className="parameter"; card.style.setProperty("--color",COLORS[i]);
  const title=document.createElement("h4"); title.id="parameter-name-"+i; title.textContent=["Level","Filter","LFO","Reverb"][i];
  const pie=document.createElement("div"); pie.className="indicator"; pie.setAttribute("aria-hidden","true");
  const input=document.createElement("input"); input.type="range"; input.min=0; input.max=100; input.value=0; input.disabled=true; input.setAttribute("aria-labelledby",title.id);
  input.addEventListener("input",()=>{ send("control",{col:i,value:+input.value}); });
  const buttons=document.createElement("div"); buttons.className="step-buttons";
  for (const [symbol,n] of [["−",-1],["+",1]]) {
    const b=document.createElement("button"); b.textContent=symbol; b.disabled=true;
    b.setAttribute("aria-label",["Decrease ","Increase "][n===1?1:0]+title.textContent);
    b.addEventListener("click",()=>{input.value=String(+input.value+n);send("control",{col:i,value:+input.value});}); buttons.append(b);
  }
  const word=document.createElement("span"); word.className="word";
  card.append(title,pie,input,buttons,word); $("controls").append(card); parameterEls.push({title,pie,input,buttons,word});
}
function makeKey(index,white,pos,label) {
  const b=document.createElement("button"); b.className="piano-key "+(white?"white":"black"); b.disabled=true;
  b.style.left=(white ? pos*6.25 : (pos+1)*6.25-1.85)+"%"; b.style.width=(white?6.0:3.7)+"%"; b.textContent=label;
  b.setAttribute("aria-label",(white?"White key "+(WHITE.indexOf(index)+1):"Black key "+(BLACK.indexOf(index)+1))+"; computer key "+label);
  b.setAttribute("aria-pressed","false");
  b.addEventListener("pointerdown",e=>{if(!started||e.button!==0)return;e.preventDefault();b.focus({preventScroll:true});b.setPointerCapture(e.pointerId);held.set("p"+e.pointerId,index);syncKeys();});
  for(const ev of ["pointerup","pointercancel","lostpointercapture"]) b.addEventListener(ev,e=>{held.delete("p"+e.pointerId);syncKeys();});
  b.addEventListener("keydown",e=>{if(e.key!==" "&&e.key!=="Enter")return;e.preventDefault();e.stopPropagation();if(!e.repeat){held.set("b"+index,index);syncKeys();}});
  b.addEventListener("keyup",e=>{if(e.key===" "||e.key==="Enter"){e.preventDefault();held.delete("b"+index);syncKeys();}});
  b.addEventListener("blur",()=>{held.delete("b"+index);syncKeys();});
  b.addEventListener("click",e=>{if(e.detail===0&&!held.has("b"+index)&&started){held.set("s"+index,index);syncKeys();setTimeout(()=>{held.delete("s"+index);syncKeys();},160);}});
  keyButtons[index]=b; $("keys").append(b);
}
WHITE.forEach((k,i)=>makeKey(k,true,i,WHITE_CODES[i]));
BLACK.forEach((k,i)=>makeKey(k,false,AFTER[i],BLACK_CODES[i]));
function update(s) {
  state=s; $("sound-page").value=String(s.view);
  $("scene").value=String(s.scene);
  $("play").textContent=s.playing?"Rest":"Play"; $("play").setAttribute("aria-pressed",String(s.playing));
  $("hold").setAttribute("aria-pressed",String(s.held));
  $("save-label").textContent=s.dirty?"You have unsaved scene edits.":storageFailed?"Playing with session-only storage.":"Scene settings are saved.";
  $("page-caption").textContent=s.page+" · "+(scenes[s.scene]||"NightClaw");
  const home=s.view===7;
  $("track-controls").hidden=!home; $("track").value=String(s.track);
  document.querySelectorAll("[data-page]").forEach(b=>b.setAttribute("aria-pressed",String(+b.dataset.page===s.view)));
  document.querySelectorAll(".scene-pick").forEach(b=>b.setAttribute("aria-pressed",String(+b.dataset.scene===s.scene)));
  $("voice-wrap").hidden=!s.voices.length;
  const sig=s.voices.join("|");
  if(sig!==voiceSignature) { voiceSignature=sig; $("voice").replaceChildren(...s.voices.map((v,i)=>new Option(v,String(i)))); }
  $("voice").value=String(s.voice);
  $("voice-label").textContent=home&&s.track===5?"Drum beat":s.context===1?"Creature":"Instrument";
  s.controls.forEach((p,i)=>{
    const el=parameterEls[i]; el.title.textContent=p.name;
    el.input.min=p.min;el.input.max=p.max; if(document.activeElement!==el.input)el.input.value=p.value;
    el.input.disabled=p.max<=p.min;
    el.input.setAttribute("aria-valuetext",p.text);
    [...el.buttons.children].forEach((b,j)=>{b.disabled=p.max<=p.min;b.setAttribute("aria-label",(j?"Increase ":"Decrease ")+p.name);});
    const percent=p.max>p.min?Math.max(0,Math.min(1,(p.value-p.min)/(p.max-p.min))):1;
    const missing=(1-percent)*360,half=missing/2;
    el.pie.style.background=percent<=0?"#443b45":percent>=1?COLORS[i]:`conic-gradient(from ${half}deg,${COLORS[i]} 0deg ${360-missing}deg,#443b45 ${360-missing}deg 360deg)`;
    el.word.textContent=/^[\d\s%+−\-.,]+$/.test(p.text)?"":p.text;
  });
  const calls=s.context===1,perform=s.context===2;
  $("auto-wrap").hidden=!calls; $("auto").setAttribute("aria-pressed",String(s.auto));$("auto").textContent="Automatic calls: "+(s.auto?"on":"off");
  $("keys-title").textContent=calls?"Call into the night":perform?"Perform with mutes & moves":s.context===3?"Edit with the keys":"Play your Lead";
  $("keys-help").textContent=calls?"The first eight white keys trigger creatures. Black 1 toggles Auto; black 2 stops a call.":perform?"White 1–5 mute layers; black 1–5 solo them. White 7–11 hold performance moves.":s.context===3?"These keys edit the scale, chords or pattern on this page. Return Home and select Lead to play.":"Click and hold the keys, use touch, or enable your computer keyboard.";
  $("context-help").textContent=home?"Select a track, choose its "+(s.track===5?"beat":"voice")+", then edit the four controls.":perform?"Filter, Wet, Energy and Sweep are performance gestures. Return Home for track editing.":"The four browser controls follow the four controls on the firmware screen.";
  $("oct-down").disabled=$("oct-up").disabled=!(home&&[1,2,3].includes(s.track)||[2,3,4,9,10,11,13,14,16,23,24].includes(s.view));
  keyButtons.forEach((b,i)=>b?.classList.toggle("lit",!!(s.keyLights&(1<<i))));
}
function setEnabled(on) {
  for(const id of ["scene","track","voice","play","hold","mute","release","save","auto","sound-page"]) $(id).disabled=!on;
  document.querySelectorAll("[data-page]").forEach(b=>b.disabled=!on);
  keyButtons.forEach(b=>{if(b)b.disabled=!on;});
}
async function startAudio() {
  if(starting||started)return; starting=true;
  const start=$("start");start.disabled=true;start.textContent="Opening the night…";tell("Loading the instrument.");
  let readyTimer;
  try {
    if(!window.isSecureContext)throw new Error("Audio needs an HTTPS page or localhost.");
    ac=new AudioContext({sampleRate:44100,latencyHint:"interactive"});
    if(!ac.audioWorklet)throw new Error("This browser does not support AudioWorklet. Try a current Chrome, Edge, Firefox or Safari.");
    // Resume during the user's gesture, before any network awaits.
    const resume=ac.resume();
    const [response]=await Promise.all([fetch("nightclaw.wasm"),ac.audioWorklet.addModule("worklet.js"),resume]);
    if(!response.ok)throw new Error("The instrument download failed ("+response.status+").");
    const wasm=await response.arrayBuffer(),store=loadStore();
    engine=new AudioWorkletNode(ac,"nightclaw",{numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[2]});
    gain=ac.createGain();gain.gain.value=+$("volume").value/100;engine.connect(gain).connect(ac.destination);
    await new Promise((resolve,reject)=>{
      readyTimer=setTimeout(()=>reject(new Error("The instrument took too long to start. Please retry.")),20000);
      engine.onprocessorerror=()=>{tell("The audio engine stopped. Reload the page to restart.");releaseAll();setEnabled(false);};
      engine.port.onmessage=e=>{
        const m=e.data;
        if(m.type==="ready"){
          scenes=m.scenes; $("sound-page").replaceChildren(...m.pages.map((name,i)=>({name,i})).filter(x=>x.i>0).map(x=>new Option(x.name,String(x.i))));
          $("scene").replaceChildren(...scenes.map((v,i)=>new Option(v,String(i))));
          const picks=[["RAPTOR NIGHT","The signature night"],["BONE RIDER","Out on the trail"],["RED MESA","Western percussion"],["HIGH FRONTIER","Room for your Lead"]];
          $("scene-picks").replaceChildren(...picks.flatMap(([name,desc])=>{
            const i=scenes.indexOf(name); if(i<0)return[];
            const b=document.createElement("button");b.className="scene-pick";b.dataset.scene=i;b.setAttribute("aria-pressed","false");
            const title=document.createElement("span");title.textContent=name;const small=document.createElement("small");small.textContent=desc;b.append(title,small);
            b.addEventListener("click",()=>act("scene",i));return[b];
          }));
          if(!Object.keys(store).length){const i=scenes.indexOf("RAPTOR NIGHT");if(i>=0)send("scene",{value:i});}
          clearTimeout(readyTimer);resolve();
        }else if(m.type==="error"){reject(new Error(m.message));tell("Audio error: "+m.message);setEnabled(false);}
        else if(m.type==="frame"){if(m.fb)drawScreen(m.fb);if(m.state)update(m.state);if(m.store)saveStore(m.store);}
      };
      engine.port.postMessage({type:"load",wasm,store},[wasm]);
    });
    started=true;starting=false;setEnabled(true);$("screen-off").hidden=true;$("audio-dot").classList.add("on");
    start.textContent="Audio on";start.disabled=true;
    tell("Ready. Choose a scene and play.");
    send("visible",{value:!document.hidden});
  }catch(e){
    clearTimeout(readyTimer);engine?.disconnect();engine=null;await ac?.close().catch(()=>{});
    starting=false;started=false;setEnabled(false);start.disabled=false;start.textContent="Retry audio";
    tell(e.message||String(e));
  }
}
$("start").addEventListener("click",startAudio);
$("scene").addEventListener("change",()=>act("scene",+$("scene").value));
$("track").addEventListener("change",()=>act("track",+$("track").value));
$("sound-page").addEventListener("change",()=>act("page",+$("sound-page").value));
$("voice").addEventListener("change",()=>send("voice",{value:+$("voice").value}));
document.querySelectorAll("[data-page]").forEach(b=>b.addEventListener("click",()=>act("page",+b.dataset.page)));
$("play").addEventListener("click",()=>send("play",{value:state?.playing?0:1}));
$("hold").addEventListener("click",()=>send("hold",{value:state?.held?0:1}));
$("auto").addEventListener("click",()=>send("auto",{value:state?.auto?0:1}));
$("save").addEventListener("click",()=>{send("save");$("storage-note").textContent="Saving scene settings in this browser…";});
$("mute").addEventListener("click",()=>{muted=!muted;$("mute").textContent=muted?"Unmute":"Mute";$("mute").setAttribute("aria-pressed",String(muted));gain?.gain.setTargetAtTime(muted?0:+$("volume").value/100,ac.currentTime,.015);});
$("volume").addEventListener("input",()=>{if(gain)gain.gain.setTargetAtTime(muted?0:+$("volume").value/100,ac.currentTime,.015);});
$("release").addEventListener("click",releaseAll);
$("keyboard").addEventListener("change",()=>{releaseAll();$("keyboard-hint").textContent=$("keyboard").checked?"Keyboard enabled: Q–I and A–K play white keys; 1–0 and − play black keys. Space plays or rests.":"White keys: Q–I, then A–K. Black keys: 1–0, then −. Release a key to release the note.";});
$("zoom").addEventListener("click",()=>{const on=$("instrument").classList.toggle("zoomed");$("zoom").setAttribute("aria-pressed",String(on));$("zoom").textContent=on?"Normal size":"Enlarge";});
const keyMap=new Map();WHITE_CODES.forEach((k,i)=>keyMap.set("Key"+k,WHITE[i]));BLACK_CODES.forEach((k,i)=>keyMap.set(k==="-"?"Minus":"Digit"+k,BLACK[i]));
document.addEventListener("keydown",e=>{
  if(!started||!$("keyboard").checked||$("play-panel").hidden||e.ctrlKey||e.metaKey||e.altKey)return;
  if(e.target.closest("input,select,textarea,button,a,summary"))return;
  if(e.code==="Space"){e.preventDefault();if(!e.repeat)send("play",{value:state?.playing?0:1});return;}
  if(keyMap.has(e.code)){e.preventDefault();if(!e.repeat){held.set("k"+e.code,keyMap.get(e.code));syncKeys();}}
});
document.addEventListener("keyup",e=>{if(held.delete("k"+e.code)){e.preventDefault();syncKeys();}});
addEventListener("blur",releaseAll);
document.addEventListener("visibilitychange",()=>{if(document.hidden)releaseAll();send("visible",{value:!document.hidden});});
function oct(bit) {send("octave",{value:bit===13?1:-1});}
$("oct-down").addEventListener("click",()=>oct(12));$("oct-up").addEventListener("click",()=>oct(13));

const LESSONS=[
 {title:"Start the expedition",short:"Start & scenes",intro:"A scene is a starting world: its instruments, drum beat, scale, tempo and mix. You can reshape every one.",
  steps:["On the FM-1, turn PRESETS to choose a scene. In the browser, choose a name in the scene menu.","Try RAPTOR NIGHT first. BONE RIDER, RED MESA and HIGH FRONTIER explore the western side.","Press PLAY to rest or resume. In the browser, press Start audio once before playing."],
  note:"The browser generates sound live. You do not need an FM-1 connected.",targets:["presets","play"],image:"home",caption:"PRESETS chooses the world. PLAY starts or rests it.",page:7,track:2},
 {title:"One track. Four controls.",short:"Home & the mix",intro:"Home is your control table. Select a track, then edit its four colored columns.",
  steps:["Press HOME until the track table appears. Turn SELECT to move between Drone, Sequence, Lead, Sequence 2, Atmosphere, Skin and Calls.","K1 controls Level; K2 controls Filter; K3 controls LFO; K4 controls Reverb send.","Skin uses Ghosts in the third column. Calls uses Chance for automatic calling."],
  note:"The colored circles are indicators. The four physical knobs do the editing. Each detent is one parameter step, with no speed acceleration.",targets:["home","select","controls"],image:"home",caption:"SELECT chooses the row. K1–K4 edit its four columns.",page:7,track:0},
 {title:"Choose your own voice.",short:"Instruments & Lead",intro:"The instrument is your choice. It is independent of the scene’s drum beat.",
  steps:["On Home, select Lead with SELECT. Turn ALGORITHM to choose Theremin for the RAPTOR NIGHT signature combination.","Try Cello or Cello Low for a bowed, cutting voice. Sub Pad and Fat Pad offer deep sustained sounds, especially on Drone.","The white keys play the Lead in the scene’s scale. On the FM-1, press LFO for the Lead page, then use OCT− or OCT+. In the browser, use the Octave buttons below the keys."],
  note:"Changing an instrument does not choose a new beat. Home K3 is filter motion; Lead’s sound pages provide further voice controls.",targets:["select","algorithm","keys"],image:"home",caption:"Select Lead, choose its voice with ALGORITHM, then play.",page:7,track:2},
 {title:"Put the drums in front.",short:"Beats & Skin",intro:"Skin is NightClaw’s independent drum track: deep, resonant percussion with fourteen selectable beat families.",
  steps:["On Home, select Skin. Turn ALGORITHM to choose the beat.","Use Level to bring the drums forward and Filter to darken or brighten them. Reverb adds space.","Ghosts adds secondary accents. At zero, the main beat remains."],
  note:"GALOP, TRES, WAR, RAIN, QUAKE, WAR2, TALK, SEVEN, PENTA, SPIRAL and CHAOS come from Paleotone/Gurdy. TRAIL, OUTRIDER and HUNT add western variations.",targets:["select","algorithm","controls"],image:"home",caption:"Skin chooses the drums. Its beat stays separate from your Lead.",page:7,track:5},
 {title:"Call into the dark.",short:"Creature calls",intro:"Creature calls are playable instruments, not something you have to leave running.",
  steps:["On Home, select Calls. The first eight white keys trigger Herd, Raptor, Hoot, Moan, Nest, Croak, Hadro and Snort.","The first black key toggles Manual / Auto. The second black key stops the current call.","Level zero silences the calls. Select Lead again to return the keyboard to instrument playing."],
  note:"Manual is the startup default. In this browser, the first eight white keys are Q W E R T Y U I when computer keyboard control is enabled.",targets:["select","calls"],image:"calls",caption:"White 1–8: creatures. Black 1: Auto. Black 2: stop.",page:7,track:6},
 {title:"Perform the changes.",short:"Performance & space",intro:"Keep the beat going and shape the whole scene with broad gestures.",
  steps:["Press SEL to enter Performance. The four controls become Filter, Wet, Energy and Sweep.","White keys 1–5 mute or restore layers; black keys 1–5 solo them. White keys 7–11 hold performance moves.","Press a page button to leave Performance. FX opens Space; another press opens Echo. Tap REC to toggle Hold."],
  note:"Performance keys are commands, so return Home and select Lead before playing a melody. Hold repeats the sound it has captured; use it as a deliberate performance effect.",targets:["sel","fx","controls","keys"],image:"home",caption:"SEL changes the keys’ job: mutes and moves instead of Lead notes.",page:17,track:2},
 {title:"Shape the key & chords.",short:"Key & chords",intro:"The scene provides a harmonic world. Key and Chords let you guide it.",
  steps:["From Home, use HOME again to find Key. The four controls set Tempo, Root, Mode and Transpose.","Tap EDIT for Chords. Choose a slot with K1; the keys choose its root, K2 its chord kind and K3 its type.","K4 chooses Endless or Loop. Return Home when you want to mix tracks or play Lead."],
  note:"The keys edit harmony on Key and Chords pages. They return to live instrument playing when you leave those pages.",targets:["home","edit","controls"],image:"edit",caption:"EDIT opens the chord cards. K1–K4 follow the screen’s labels.",page:15,track:2},
 {title:"Keep the world you made.",short:"Save your settings",intro:"On the hardware, saving is a GLO gesture. The printed SAVE button has a different job.",
  steps:["Hold GLO for about half a second, then release to save your edits.","A short GLO tap opens Setup. Holding to about 1.5 seconds resets the current scene, so release after the save gesture.","In this player, use Save browser session. It keeps your scene settings on this browser and device."],
  note:"The physical SAVE button opens sound pages. Browser saves and the FM-1’s saved settings are separate; neither transfers to the other.",targets:["glo"],image:"home",caption:"Hold GLO briefly to save. SAVE is a sound-page button.",page:7,track:2}
];
let lesson=0;
function diagram(item) {
 const active=new Set(item.targets);
 const hi=(id,x,y,w,h,rx=12)=>active.has(id)?`<rect x="${x-6}" y="${y-6}" width="${w+12}" height="${h+12}" rx="${rx}" class="diagram-highlight"/>`:"";
 let s=`<svg viewBox="0 0 1000 550" role="img" aria-label="FM-1 diagram highlighting ${item.targets.join(", ")}"><title>${item.caption}</title><rect x="8" y="8" width="984" height="534" rx="32" fill="#28232b" stroke="#554759" stroke-width="2"/><text x="50" y="47" fill="#e6dcc2" font-family="Barlow,Arial" font-size="22" font-weight="700">M-VAVE FM-1</text>`;
 const knobs=[["master",92,105,"MASTER"],["select",205,105,"SELECT"],["presets",92,222,"PRESETS"],["algorithm",205,222,"ALGORITHM"]];
 for(const [id,x,y,label] of knobs) s+=hi(id,x-33,y-35,66,80)+`<circle cx="${x}" cy="${y}" r="26" fill="#0c0a0e" stroke="#706477" stroke-width="2"/><path d="M ${x} ${y-20} v 9" stroke="#e6dcc2" stroke-width="3"/><text x="${x}" y="${y+48}" class="diagram-label">${label}</text>`;
 s+=`<rect x="306" y="70" width="222" height="222" rx="6" fill="#050508"/><image href="assets/${item.image}.png" x="312" y="76" width="210" height="210" style="image-rendering:pixelated"/>`;
 s+=hi("controls",565,68,388,88);
 for(let i=0;i<4;i++)s+=`<circle cx="${604+i*100}" cy="100" r="24" fill="#0c0a0e" stroke="${COLORS[i]}" stroke-width="3"/><path d="M ${604+i*100} 82 v 10" stroke="${COLORS[i]}" stroke-width="3"/><text x="${604+i*100}" y="148" class="diagram-label">K${i+1}</text>`;
 const rows=[["FX","SEL","ENV","LFO","EDIT","GLO"],["HOME","SAVE","ARP","SEQ","PLAY","REC"]];
 rows.forEach((row,r)=>row.forEach((name,i)=>{const x=567+i*66,y=185+r*62;s+=hi(name.toLowerCase(),x,y,56,44,9)+`<rect x="${x}" y="${y}" width="56" height="44" rx="6" fill="#151117" stroke="#68596b"/><text x="${x+28}" y="${y+28}" class="diagram-label" style="font-size:14px">${name}</text>`;}));
 s+=hi("keys",42,346,916,150)+hi("calls",42,346,457,150);
 for(let i=0;i<16;i++)s+=`<rect x="${48+i*57}" y="402" width="44" height="105" rx="18" fill="${active.has("calls")&&i<8?COLORS[i%4]:"#e6dcc2"}"/><text x="${70+i*57}" y="480" text-anchor="middle" font-family="Barlow,Arial" font-size="18" fill="#050508">${i+1}</text>`;
 AFTER.forEach((a,i)=>{s+=`<rect x="${76+a*57}" y="347" width="40" height="46" rx="15" fill="${active.has("calls")&&i<2?"#eaff00":"#7c727e"}"/>`;});
 s+="</svg>"; return s;
}
LESSONS.forEach((l,i)=>{const b=document.createElement("button");const num=document.createElement("span");num.textContent=String(i+1).padStart(2,"0");b.append(num,document.createTextNode(l.short));b.addEventListener("click",()=>showLesson(i));$("lessons").append(b);});
function showLesson(i){
 lesson=i;const l=LESSONS[i];
 [...$("lessons").children].forEach((b,n)=>{if(n===i)b.setAttribute("aria-current","step");else b.removeAttribute("aria-current");});
 $("diagram").innerHTML=diagram(l);$("diagram-caption").textContent=l.caption;
 $("lesson-number").textContent="CHAPTER "+String(i+1).padStart(2,"0")+" / "+String(LESSONS.length).padStart(2,"0");
 $("lesson-title").textContent=l.title;$("lesson-intro").textContent=l.intro;
 $("lesson-steps").replaceChildren(...l.steps.map(t=>{const li=document.createElement("li");li.textContent=t;return li;}));
 $("lesson-note").textContent=l.note;$("lesson-position").textContent=(i+1)+" / "+LESSONS.length;
 $("previous").disabled=i===0;$("next").disabled=i===LESSONS.length-1;
}
$("previous").addEventListener("click",()=>showLesson(Math.max(0,lesson-1)));
$("next").addEventListener("click",()=>showLesson(Math.min(LESSONS.length-1,lesson+1)));
$("try-lesson").addEventListener("click",()=>{
 const l=LESSONS[lesson];location.hash="play";
 if(started){act("track",l.track);if(l.page!==7)send("page",{value:l.page});}
 else tell("Press Start audio, then select the track or page shown in the guide.");
 $("instrument").scrollIntoView({behavior:matchMedia("(prefers-reduced-motion: reduce)").matches?"instant":"smooth",block:"start"});
});
function route(){
 const guide=location.hash==="#guide";$("guide-panel").hidden=!guide;$("play-panel").hidden=guide;
 document.querySelectorAll("[data-tab]").forEach(a=>{if(a.dataset.tab===(guide?"guide":"play"))a.setAttribute("aria-current","page");else a.removeAttribute("aria-current");});
 if(guide)releaseAll();
}
addEventListener("hashchange",route);showLesson(0);route();
