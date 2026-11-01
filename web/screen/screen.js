
"use strict";
/* Read-only second screen. Needs firmware with editor_screen.c (cmds 90, 91). Never sends anything that changes the FM-1. */
const C = {bg:"#000", fg:"#33ff66", dim:"#0f5a22", am:"#ffb000"};
const cv = document.getElementById("s"), g = cv.getContext("2d"), st = document.getElementById("st");
let midi = null, input = null, selected = null, lastReply = 0, connectedAt = 0;
let lastRetry = 0;
const RECONNECT_COOLDOWN_MS = 4000;
let out = null, info = null, dump = null, scr = null, pages = [], desc = {}, asked = {}, names = {}, menu = [], menuAsked = false, hot = {};
const q = [];
let selectedTrack = 0;
const READ_ONLY = new Set([1,4,5,10,22,25,27,72,90,91]);
const tx = (cmd, ...a) => { if (!READ_ONLY.has(cmd)) throw new Error('Read-only display command required'); if (out && q.length < 128) q.push([0xF0, 0x7D, 0x46, 0x4C, cmd, ...a, 0xF7]); };
setInterval(() => { if (out && q.length) { try { out.send(q.shift()); } catch(e) { disconnected('USB connection lost. Reconnect the FM-1.'); } } }, 12);
setInterval(() => { if (out) tx(25); }, 1000);                 /* PING: keeps WATCH alive (it ends after 3 s) */
const v14 = (a, i) => (a[i] | a[i + 1] << 7) - 8192;
function str(a, i) { let s = ""; while (i < a.length && a[i]) s += String.fromCharCode(a[i++]); return [s, i + 1]; }

/* ---- incoming ---- */
function onMsg(e) {
  const d = e.data;
  if (d[0] !== 0xF0 || d[1] !== 0x7D || d[2] !== 0x46 || d[3] !== 0x4C) return;
  if (d.length < 6 || d[d.length-1] !== 0xF7) return;
  lastReply = performance.now();
  const cmd = d[4], p = Array.from(d.slice(5, d.length - 1));
  if (cmd === 1) {                                              /* INFO */
    let [ver, i] = str(p, 0);
    info = {ver, ne: p[i], pc: p[i + 1], gc: p[i + 2], ns: p[i + 3], e0: p[i + 4], eng: []};
    i += 5;
    for (let k = 0; k < info.ne; k++) { let n; [n, i] = str(p, i); info.eng.push(n); }
    tx(22, 1); tx(4); tx(27); tx(91, 0); tx(90);
  } else if (cmd === 91) {                                      /* PAGE_DESC */
    const k = p[0], n = p[1];
    if (k < n) {
      const [t, j] = str(p, 2);
      pages[k] = {t, fam: p[j], sc: p[j + 1], gr: p[j + 2], id: p.slice(j + 3, j + 7).map(x => x === 127 ? -1 : x)};
      tx(91, k + 1);
    }
  } else if (cmd === 90 && p.length >= 12) { scr = p.slice(0, 12); if (scr[2] === 1 && !menuAsked) { menuAsked = true; menu = []; tx(72, 0); } if (scr[2] !== 1) menuAsked = false; }
  else if (cmd === 4 && info && p.length >= 2 + 2*(info.pc+info.gc)) {                                         /* DUMP */
    dump = {eng: p[0], pre: p[1], v: [], g: []};
    for (let i = 0; i < info.pc; i++) dump.v.push(v14(p, 2 + 2 * i));
    for (let i = 0; i < info.gc; i++) dump.g.push(v14(p, 2 + 2 * info.pc + 2 * i));
    if (!names[dump.eng]) tx(10, dump.eng);
  } else if (cmd === 10) {                                      /* NAMES */
    let i = 2; const n = [];
    for (let k = 0; k < p[1]; k++) { let s; [s, i] = str(p, i); n.push(s); }
    names[p[0]] = n;
  } else if (cmd === 5) {                                       /* DESC */
    const fmt = p[2], mn = v14(p, 3), mx = v14(p, 5);
    let [label, j] = str(p, 9), unit; [unit, j] = str(p, j);
    const en = [];
    if (fmt === 8) for (let k = mn; k <= mx && k - mn < 24; k++) { let s; [s, j] = str(p, j); en.push(s); }
    desc[p[0] + ":" + p[1]] = {fmt, mn, mx, label, unit, en};
  } else if (cmd === 23 && dump) {                              /* CHANGED */
    const sc = p[0], id = p[1], v = v14(p, 2);
    (sc ? dump.g : dump.v)[id] = v;
    hot[sc + ":" + id] = performance.now();
  } else if (cmd === 24) { desc = {}; asked = {}; tx(4); tx(27); }
  else if (cmd === 27 && p.length >= 2) selectedTrack = p[0];      /* RELOAD */
  else if (cmd === 72) {                                        /* MENU_DESC */
    if (p[1] === 127) return;
    const kind = p[2], mn = v14(p, 5), mx = v14(p, 7);
    let [name, j] = str(p, 9), vn = [], unit = "";
    if (kind === 0) for (let k = mn; k <= mx; k++) { let s; [s, j] = str(p, j); vn.push(s); } else [unit, j] = str(p, j);
    menu[p[0]] = {name, kind, val: v14(p, 3), vn, unit, tab: p[j]};
    tx(72, p[0] + 1);
  }
}

/* ---- value text ---- */
function need(sc, id) { const k = sc + ":" + id; if (!desc[k] && !asked[k]) { asked[k] = 1; tx(5, sc, id); } return desc[k]; }
function fmtVal(d, v) {
  if (!d) return "";
  if (d.fmt === 8) return d.en[v - d.mn] || String(v);
  if (d.fmt === 11) return v ? "ON" : "OFF";
  return v + (d.unit || "");
}
const txt = (s, x, y, c = C.fg, a = "left", sz = 12) => { g.fillStyle = c; g.textAlign = a; g.font = sz + "px 'Share Tech Mono',monospace"; g.fillText(s, x, y); };

/* ---- drawing: the layout of ui.c (cards x=3+59c y=28 57x44, panel y=76 h=122, footer y=202) ---- */
function draw() {
  g.fillStyle = C.bg; g.fillRect(0, 0, 240, 240);
  if (!info || !dump || !scr || !pages.length) { txt(out ? "WAITING FOR FM-1..." : "PALEOTONE", 120, 124, C.dim, "center", 14); return; }
  const [home, pg, mnu, msel, conf, lay, bank, cur] = scr, P = pages[pg];
  const title = mnu ? "MENU" : home ? "HOME" : P ? P.t : "";
  txt(title, 6, 18, C.fg, "left", 14);
  txt((info.eng[dump.eng] || "") + " " + ((names[dump.eng] || [])[dump.pre] || ""), 234, 18, C.dim, "right", 11);
  g.fillStyle = C.dim; g.fillRect(3, 24, 234, 1);
  if (mnu === 1) drawMenu(msel);
  else if (!home && P && (P.sc === 0 || P.sc === 1)) drawPage(P);
  else if (home) {
    txt('TRACK '+(selectedTrack+1),120,86,C.am,'center',16);
    txt(info.eng[dump.eng]||'',120,119,C.fg,'center',18);
    txt(((names[dump.eng]||[])[dump.pre]||'').slice(0,25),120,145,C.fg,'center',13);
    txt('Use the FM-1 to navigate',120,181,C.dim,'center',11);
  } else txt(title,120,140,C.dim,'center',18);
  if (P && P.fam === 9 && !home && !mnu) txt("STEP " + (cur + 1) + "   BANK " + (bank + 1), 6, 226, C.am, "left", 12);
  if (lay) txt("LAYER " + lay, 234, 226, C.am, "right", 12);
  if (conf) { g.fillStyle = "#000"; g.fillRect(30, 90, 180, 56); g.strokeStyle = C.am; g.strokeRect(30.5, 90.5, 179, 55); txt("CONFIRM", 120, 122, C.am, "center", 16); }
}
function drawPage(P) {
  const now = performance.now();
  for (let c = 0; c < 4; c++) {
    const x = 3 + 59 * c, id = P.id[c];
    g.strokeStyle = C.dim; g.strokeRect(x + .5, 28.5, 56, 43);
    if (id < 0) continue;
    const d = need(P.sc, id), v = (P.sc ? dump.g : dump.v)[id], on = now - (hot[P.sc + ":" + id] || -1e9) < 600;
    txt(d ? d.label.slice(0, 6) : "", x + 28, 42, on ? C.am : C.dim, "center", 11);
    txt(fmtVal(d, v).slice(0, 7), x + 28, 62, on ? C.am : C.fg, "center", 14);
  }
  g.strokeStyle = C.dim; g.strokeRect(3.5, 76.5, 233, 121);
  if (P.gr === 1) {                                             /* GR_ADSR: A D S R from the four values */
    const [a, d, s, r] = P.id.map(i => (dump.v[i] || 0)), m = (n) => Math.max(0, Math.min(1, n / 127));
    const w = 55, y0 = 190, h = 100, x0 = 14, sx = x0 + w * m(a), dx = sx + w * m(d), rx = dx + 40, ex = rx + w * m(r);
    g.strokeStyle = C.fg; g.beginPath(); g.moveTo(x0, y0); g.lineTo(sx, y0 - h); g.lineTo(dx, y0 - h * m(s)); g.lineTo(rx, y0 - h * m(s)); g.lineTo(ex, y0); g.stroke();
  } else {
    for (let c=0;c<4;c++) {
      const id=P.id[c]; if(id<0)continue;
      const d=need(P.sc,id),v=(P.sc?dump.g:dump.v)[id];if(!d)continue;
      const y=94+c*26, span=d.mx-d.mn, width=Math.max(0,Math.min(1,span?(v-d.mn)/span:0))*132;
      txt(d.label.slice(0,7),10,y+9,C.dim,'left',11);
      g.fillStyle=C.dim;g.fillRect(93,y,132,10);g.fillStyle=C.fg;g.fillRect(93,y,width,10);
    }
  }
}
function drawMenu(sel) {
  const it = menu[sel], tab = it ? it.tab : 0, list = [];
  menu.forEach((m, i) => { if (m && m.tab === tab) list.push(i); });
  const first = Math.max(0, Math.min(list.indexOf(sel) - 5, list.length - 11));
  list.slice(first, first + 11).forEach((i, k) => {
    const m = menu[i], y = 44 + k * 16, on = i === sel;
    if (on) { g.fillStyle = C.dim; g.fillRect(4, y - 12, 232, 15); }
    txt((on ? "> " : "  ") + m.name, 8, y, on ? C.fg : C.dim, "left", 12);
    txt(m.kind === 0 ? (m.vn[m.val] || "") : m.val + m.unit, 232, y, on ? C.fg : C.dim, "right", 12);
  });
}
setInterval(() => { if (!document.hidden) draw(); }, 60);

/* ---- connect ---- */

function resetDisplay() {
  q.length=0;info=dump=scr=null;pages=[];desc={};asked={};names={};menu=[];menuAsked=false;hot={};selectedTrack=0;
}
function disconnected(message) {
  if(input) input.onmidimessage=null;
  if(out) { try { out.clear(); } catch {} }
  input=out=null;lastRetry=performance.now();resetDisplay();st.textContent=message;
}
function pairs() {
  if(!midi)return [];
  const outputs=[...midi.outputs.values()].filter(p=>p.state!=='disconnected');
  return [...midi.inputs.values()].filter(p=>p.state!=='disconnected').flatMap(i=>{
    const exact=outputs.filter(o=>o.name===i.name&&o.manufacturer===i.manufacturer);
    return exact.map(o=>({i,o,key:i.id+'|'+o.id,label:i.name||'Unnamed MIDI device'}));
  });
}
async function attach(pair) {
  disconnected('Connecting to '+pair.label+'…');selected=pair.key;
  try {
    await pair.i.open();await pair.o.open();input=pair.i;out=pair.o;
    input.onmidimessage=onMsg;connectedAt=performance.now();lastReply=0;tx(1);
    document.getElementById('portbar').hidden=true;
  } catch(e){disconnected('Could not open FM-1: '+e.message);}
}
function refreshPorts(auto=false) {
  const list=pairs(), sel=document.getElementById('ports');sel.replaceChildren();
  list.forEach(p=>{const o=document.createElement('option');o.value=p.key;o.textContent=p.label;sel.append(o);});
  if(out&&out.state==='disconnected')disconnected('FM-1 disconnected. Waiting for its USB connection…');
  if(out)return;
  const remembered=list.find(p=>p.key===selected);
  const matching=list.filter(p=>/fm-?1|felucca|paleotone|ghoul/i.test(p.label));
  const candidate=remembered||(matching.length===1?matching[0]:null);
  if(auto&&candidate){lastRetry=performance.now();attach(candidate);return;}
  document.getElementById('portbar').hidden=!list.length;
  st.textContent=list.length?'Select your FM-1 ports above.':'No FM-1 found. Check the USB data cable and turn the instrument on.';
}
document.getElementById('use').onclick=()=>{const p=pairs().find(p=>p.key===document.getElementById('ports').value);if(p)attach(p);};
document.getElementById('go').onclick=async()=>{
  if(!window.isSecureContext){st.textContent='Open this page over HTTPS or localhost to use MIDI.';return;}
  if(!navigator.requestMIDIAccess){st.textContent='This browser does not offer Web MIDI. Try Chrome or Edge on a computer.';return;}
  try {midi=await navigator.requestMIDIAccess({sysex:true});midi.onstatechange=()=>refreshPorts(true);refreshPorts(true);}
  catch(e){st.textContent='MIDI permission was blocked. Allow MIDI and SysEx, then connect again.';}
};
setInterval(()=>{
  const now=performance.now();
  // Reattach known hardware after a stale-stream timeout or USB re-enumeration.
  // No additional permission prompt: reuse the user's existing MIDI access.
  if(!out){
    if(midi && selected && now-lastRetry>=RECONNECT_COOLDOWN_MS){
      lastRetry=now;
      refreshPorts(true);
    }
    return;
  }
  if(scr)st.textContent=out.name+' · screen connected';
  else if(info&&now-connectedAt>4000)st.textContent='FM-1 replied, but the second-screen firmware messages are missing. Use the matching tested firmware.';
  else if(!lastReply&&now-connectedAt>4000)st.textContent='FM-1 is not replying. Close other MIDI apps and check the selected ports.';
  if(scr&&now-lastReply>5000){disconnected('FM-1 stopped replying. Press Connect FM-1 to retry.');return;}
  if(!info)tx(1);else if(!scr)tx(90);
},1500);
document.getElementById('full').onclick=()=>document.fullscreenElement?document.exitFullscreen():cv.requestFullscreen?.();
let installPrompt;
addEventListener('beforeinstallprompt',e=>{e.preventDefault();installPrompt=e;document.getElementById('install').hidden=false;});
document.getElementById('install').onclick=async()=>{if(!installPrompt)return;await installPrompt.prompt();installPrompt=null;document.getElementById('install').hidden=true;};
if('serviceWorker' in navigator)navigator.serviceWorker.register('./sw.js').catch(()=>{});
addEventListener('pagehide',()=>disconnected('Disconnected'));
