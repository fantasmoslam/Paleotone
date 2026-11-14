// SPDX-License-Identifier: GPL-3.0-only
const $ = id => document.getElementById(id);
const COLORS = ["#e6dcc2","#ff284e","#eaff00","#a63aff"];
const AFTER = [0,1,2,4,5,7,8,9,11,12,14];
const LESSONS=[
 {title:"Start the expedition",short:"Start & scenes",intro:"A scene is a starting world: its instruments, drum beat, scale, tempo and mix. You can reshape every one.",
  steps:["On the FM-1, turn PRESETS to choose a scene. In the browser, drag or scroll the PRESETS knob on the FM-1 panel.","Try RAPTOR NIGHT first. BONE RIDER, RED MESA and HIGH FRONTIER explore the western side.","Press PLAY to rest or resume. In the browser, press Switch on once before playing."],
  note:"The browser generates sound live. You do not need an FM-1 connected.",targets:["presets","play"],image:"home",caption:"PRESETS chooses the world. PLAY starts or rests it.",page:7,track:2},
 {title:"One track. Four controls.",short:"Home & the mix",intro:"Home is your control table. Select a track, then edit its four colored columns.",
  steps:["Press HOME until the track table appears. Turn SELECT to move between Drone, Sequence, Lead, Sequence 2, Atmosphere, Skin and Calls.","K1 controls Level; K2 controls Filter; K3 controls LFO; K4 controls Reverb send.","Skin uses Ghosts in the third column. Calls uses Chance for automatic calling."],
  note:"The colored circles are indicators. The four physical knobs do the editing. Each detent is one parameter step, with no speed acceleration.",targets:["home","select","controls"],image:"home",caption:"SELECT chooses the row. K1–K4 edit its four columns.",page:7,track:0},
 {title:"Choose your own voice.",short:"Instruments & Lead",intro:"The instrument is your choice. It is independent of the scene’s drum beat.",
  steps:["On Home, select Lead with SELECT. Turn ALGORITHM to choose Theremin for the RAPTOR NIGHT signature combination.","Try Cello or Cello Low for a bowed, cutting voice. Sub Pad and Fat Pad offer deep sustained sounds, especially on Drone.","The white keys play the Lead in the scene’s scale. On the FM-1, press LFO for the Lead page, then use OCT− or OCT+. In the browser, use the same LFO and OCT buttons on the panel."],
  note:"Changing an instrument does not choose a new beat. Home K3 is filter motion; Lead’s sound pages provide further voice controls.",targets:["select","algorithm","keys"],image:"home",caption:"Select Lead, choose its voice with ALGORITHM, then play.",page:7,track:2},
 {title:"Shape each instrument’s envelope.",short:"Attack to Release",intro:"One page brings amplitude shaping together: Attack, Decay, Sustain and Release for each melodic track, Calls and Atmosphere.",
  steps:["On Home, select a track with SELECT, then tap SAVE to open Envelope. SELECT moves between rows; K1–K4 edit Attack, Decay, Sustain and Release.","Attack sets the rise, Decay the fall toward the held level, Sustain that level, and Release the fade after the note ends. Try a slower Attack on a pad, or a short Release on a cello.","ALGORITHM changes the selected row’s instrument while keeping your manual envelope edits. Turn a stage below zero to restore that stage’s voice setting. Tap SAVE again for the track’s sound page."],
  note:"Untouched scenes keep their original envelopes. GLO saves your edits with the scene. Sequences use their existing note lengths; Atmosphere follows Play/rest. Recorded and modeled sounds retain their natural decay. Skin keeps its percussion Decay control.",targets:["save","select","controls","algorithm"],image:"envelope",caption:"Four columns: Attack, Decay, Sustain and Release. The lit row follows SELECT.",page:28,track:2},
 {title:"Put the drums in front.",short:"Beats & Skin",intro:"Skin is NightClaw’s independent drum track: deep, resonant percussion with fourteen selectable beat families.",
  steps:["On Home, select Skin. Turn ALGORITHM to choose the beat.","Use Level to bring the drums forward and Filter to darken or brighten them. Reverb adds space.","Ghosts adds secondary accents. At zero, the main beat remains."],
  note:"GALOP, TRES, WAR, RAIN, QUAKE, WAR2, TALK, SEVEN, PENTA, SPIRAL and CHAOS come from Paleotone/Gurdy. TRAIL, OUTRIDER and HUNT add western variations.",targets:["select","algorithm","controls"],image:"home",caption:"Skin chooses the drums. Its beat stays separate from your Lead.",page:7,track:5},
 {title:"Call into the dark.",short:"Creature calls",intro:"Creature calls are playable instruments, not something you have to leave running.",
  steps:["On Home, select Calls. The first eight white keys trigger Herd, Raptor, Hoot, Moan, Nest, Croak, Hadro and Snort.","The first black key toggles Manual / Auto. The second black key stops the current call.","Level zero silences the calls. Select Lead again to return the keyboard to instrument playing."],
  note:"Manual is the startup default. In this browser, the first eight white keys are Z X C V B N M and comma. The shortcuts are printed on the panel.",targets:["select","calls"],image:"calls",caption:"White 1–8: creatures. Black 1: Auto. Black 2: stop.",page:7,track:6},
 {title:"Perform the changes.",short:"Performance & space",intro:"Keep the beat going and shape the whole scene with broad gestures.",
  steps:["Press SEL to enter Performance. The four controls become Filter, Wet, Energy and Sweep.","White keys 1–5 mute or restore layers; black keys 1–5 solo them. White keys 7–11 hold performance moves.","Press a page button to leave Performance. FX opens Space; another press opens Echo. Tap REC to toggle Hold."],
  note:"Performance keys are commands, so return Home and select Lead before playing a melody. Hold repeats the sound it has captured; use it as a deliberate performance effect.",targets:["sel","fx","controls","keys"],image:"home",caption:"SEL changes the keys’ job: mutes and moves instead of Lead notes.",page:17,track:2},
 {title:"Shape the key & chords.",short:"Key & chords",intro:"The scene provides a harmonic world. Key and Chords let you guide it.",
  steps:["From Home, use HOME again to find Key. The four controls set Tempo, Root, Mode and Transpose.","Tap EDIT for Chords. Choose a slot with K1; the keys choose its root, K2 its chord kind and K3 its type.","K4 chooses Endless or Loop. Return Home when you want to mix tracks or play Lead."],
  note:"The keys edit harmony on Key and Chords pages. They return to live instrument playing when you leave those pages.",targets:["home","edit","controls"],image:"edit",caption:"EDIT opens the chord cards. K1–K4 follow the screen’s labels.",page:15,track:2},
 {title:"Keep the world you made.",short:"Save your settings",intro:"On the hardware, saving is a GLO gesture. The printed SAVE button has a different job.",
  steps:["Hold GLO for about half a second, then release to save your edits.","A short GLO tap opens Setup. Holding to about 1.5 seconds resets the current scene, so release after the save gesture.","In this player, hold the same GLO button, then release. It keeps your scene settings on this browser and device."],
  note:"From Home, SAVE opens the selected track’s Envelope page, or Skin’s drum page. Browser saves and the FM-1’s saved settings are separate; neither transfers to the other.",targets:["glo"],image:"home",caption:"Hold GLO briefly to save. SAVE is a sound-page button.",page:7,track:2}
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
$("try-lesson").addEventListener("click",()=>{const l=LESSONS[lesson];location.href="./?track="+l.track+"&page="+l.page;});
showLesson(0);
