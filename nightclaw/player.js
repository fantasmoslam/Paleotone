import { tabletLayout } from "./tablet-layout.mjs?v=tablet-1";
// SPDX-License-Identifier: GPL-3.0-only
// Physical FM-1 drawing/input adapted from Paleotone main 6f027f0, after X0X.
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments.
const NS="http://www.w3.org/2000/svg";
const B={FX:0,SEL:1,ENV:2,LFO:3,EDIT:4,GLO:5,HOME:6,SAVE:7,ARP:8,SEQ:9,PLAY:10,REC:11,"OCT-":12,"OCT+":13};
const EN={SELECT:0,ALGORITHM:1,PRESETS:2,"KNOB 1":3,"KNOB 2":4,"KNOB 3":5,"KNOB 4":6};
const PLAY_GREEN=14;
const WHITE=[0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26],BLACK=[1,3,5,8,10,13,15,17,20,22,25],BLACK_AFTER=[0,1,2,4,5,7,8,9,11,12,14];
const STORE="nightclaw-browser-v1";
let tabletMode=new URLSearchParams(location.search).get("view")==="tablet";
document.body.classList.toggle("tablet-mode",tabletMode);
if(location.hash==="#guide")location.replace("guide.html");
// ------------------------------------------------------------------ the panel
// Two drawings of the same controls: WIDE after the FM-1's own layout, TALL for a phone held upright (the
// screen larger, the keys across the whole width). Rebuilt when the orientation changes; the state stays.
const ROW1 = ["FX", "SEL", "ENV", "LFO", "EDIT", "GLO"], ROW2 = ["HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC"];
const KNOBS = ["MASTER", "SELECT", "PRESETS", "ALGORITHM", "KNOB 1", "KNOB 2", "KNOB 3", "KNOB 4"];
// the computer keyboard, every key one control: F3..A#4 on the Z (white) and A (black) rows, B4..G5 on Q..O
// in semitones, the twelve buttons on the number row in the panel's order (held while the key is), [ ] OCT
const KEYMAP = {};
["KeyZ", "KeyX", "KeyC", "KeyV", "KeyB", "KeyN", "KeyM", "Comma", "Period", "Slash"].forEach((c, i) => KEYMAP[c] = { key: WHITE[i] });
[["KeyS", 1], ["KeyD", 3], ["KeyF", 5], ["KeyH", 8], ["KeyJ", 10], ["KeyL", 13], ["Semicolon", 15], ["Quote", 17]].forEach(([c, k]) => KEYMAP[c] = { key: k });
["KeyQ", "KeyW", "KeyE", "KeyR", "KeyT", "KeyY", "KeyU", "KeyI", "KeyO"].forEach((c, i) => KEYMAP[c] = { key: 18 + i });
["Digit1", "Digit2", "Digit3", "Digit4", "Digit5", "Digit6", "Digit7", "Digit8", "Digit9", "Digit0", "Minus", "Equal"]
  .forEach((c, i) => KEYMAP[c] = { btn: B[[...ROW1, ...ROW2][i]] });
Object.assign(KEYMAP, { BracketLeft: { btn: B["OCT-"] }, BracketRight: { btn: B["OCT+"] } });
// what the panel prints for each
const CAP = { Comma: ",", Period: ".", Slash: "/", Semicolon: ";", Quote: "'", Minus: "-", Equal: "=", BracketLeft: "[", BracketRight: "]" };
const KEYCAP = {}, BTNCAP = {};
for (const [c, m] of Object.entries(KEYMAP)) {
  const t = CAP[c] || c.replace(/^(Key|Digit)/, "");
  if (m.key !== undefined) KEYCAP[m.key] = t;
  else BTNCAP[m.btn] = t;
}
const LAYOUTS = {
  wide: {
    w: 1000, h: 624, screen: [292, 44, 256],
    knobs: [[105, 100, 24], [215, 100, 24], [105, 205, 24], [215, 205, 24], [610, 100, 22], [714, 100, 22], [818, 100, 22], [922, 100, 22]],
    trays: [[50, 250, 220, 50], [568, 160, 400, 140]],
    oct: [[60, 258, 90, 34], [170, 258, 90, 34]],
    grid: [582, 174, 54, 48, 63, 62], keybed: [36, 332, 928, 256],
  },
  tall: {
    w: 600, h: 1070, screen: [130, 104, 340],
    knobs: [[66, 64, 24], [186, 64, 24], [414, 64, 24], [534, 64, 24], [90, 512, 26], [230, 512, 26], [370, 512, 26], [510, 512, 26]],
    trays: [[14, 572, 572, 146], [150, 728, 300, 62]],
    oct: [[164, 737, 128, 44], [308, 737, 128, 44]],
    grid: [24, 582, 86, 60, 94, 70], keybed: [14, 800, 572, 256],
  },
};
const svg = document.getElementById("panel"), device = document.getElementById("device"), canvas = document.getElementById("screen");
function el(tag, attrs, parent = svg) {
  const e = document.createElementNS(NS, tag);
  for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, v);
  parent.appendChild(e);
  return e;
}
const SANS = "'Clayborn', Arial, sans-serif", DOT = SANS;
function label(parent, x, y, s, size, font = SANS, ink = "var(--label)") {
  const t = el("text", { x, y, "text-anchor": "middle", "font-family": font,
                         "font-size": size, "font-weight": 600, fill: ink, "letter-spacing": ".04em" }, parent);
  t.textContent = s;
  return t;
}
const KEYNAMES = ["F", "F♯", "G", "G♯", "A", "A♯", "B", "C", "C♯", "D", "D♯", "E"];
const keyName = (k) => KEYNAMES[k % 12] + (3 + Math.floor((k + 5) / 12));

let audioContext = null, node = null, btnMask = 0, keyMask = 0, latched = 0, masterValue = 2800, lastLeds = [0, 0, 0, 0, 0], layout = null;
let requestId = 0, toolBusy = false, recording = null;
const requests = new Map(), midiLast = new Map(), midiDiverted = new Set();
const angle = Object.fromEntries(KNOBS.map((n) => [n, 0]));
angle.MASTER = -135 + 270 * masterValue / 4096;
const knobs = [], buttons = [], keys = [];
const btnBy = new Map(), keyBy = new Map();                   // pointer / key code -> label bit or key
const send = (m) => { if (node) node.port.postMessage(m); };

function build(name) {
  const bounds=device.getBoundingClientRect();
  const L = name === "tablet" ? tabletLayout(bounds.width,bounds.height) : LAYOUTS[name];
  layout = name;
  svg.innerHTML = "";
  svg.setAttribute("viewBox", `0 0 ${L.w} ${L.h}`);
  device.style.aspectRatio = name === "tablet" ? "auto" : `${L.w} / ${L.h}`;
  for (const d of device.querySelectorAll(".hit")) d.remove();
  knobs.length = buttons.length = keys.length = 0;
  if (!L.tablet) el("rect", { x: 4, y: 4, width: L.w - 8, height: L.h - 8, rx: 40, fill: "var(--body)" });
  for (const [x, y, w, h] of L.trays) el("rect", { x, y, width: w, height: h, rx: 14, fill: "var(--tray)" });
  const [sx, sy, ss] = L.screen;
  if (!L.tablet) el("rect", { x: sx, y: sy, width: ss, height: ss, rx: 18, fill: "#0b0b0c" });
  const inset = L.tablet ? 0 : ss / 32;                                         // the 240 x 240 panel inside its bezel
  Object.assign(canvas.style, { left: `${(sx + inset) / L.w * 100}%`, top: `${(sy + inset) / L.h * 100}%`,
                                width: `${(ss - 2 * inset) / L.w * 100}%`, height: `${(ss - 2 * inset) / L.h * 100}%` });
  KNOBS.forEach((n, i) => {
    const [x, y, r] = L.knobs[i], g = el("g", {});
    el("circle", { cx: x, cy: y, r: r + 5, fill: "var(--tray)" }, g);
    el("circle", { cx: x, cy: y, r, fill: "var(--part)", stroke: "var(--edge)", "stroke-width": 2 }, g);
    const ind = n === "MASTER" ? el("line", { x1: x, y1: y - r + 4, x2: x, y2: y - r * 0.35, stroke: "#fff", "stroke-width": 4, "stroke-linecap": "round" }, g) : null;   // (GHOULBOX: white, to near the centre)
    const caption=label(g, x, y - r - 12, n, L.tablet ? (L.w<720?16:20) : 16, DOT);
    const arrows = { SELECT: "\u2191 \u2193", ALGORITHM: "\u2190 \u2192" }[n];
    if (arrows && name === "wide") el("text", { x: x + r + 10, y: y + 5, class: "hint" }, g).textContent = arrows;
    el("circle", { cx: x, cy: y, r: r + 12, fill: "transparent" }, g);
    knobs.push({ g, ind, caption, x, y, name: n, role: n === "MASTER" ? "master" : EN[n] });
  });
  const button = ([x, y, w, h], n, sub) => {
    const g = el("g", {});
    el("rect", { x, y, width: w, height: h, rx: 8, fill: "var(--part)", stroke: "var(--edge)", "stroke-width": 1.5, class: "face btn" }, g);
    const ink = n === "REC" ? "#ff453a" : undefined;
    if (sub) { label(g, x + w / 2, y + h / 2 - 2, n, L.tablet ? 17 : 10.8, SANS, ink); label(g, x + w / 2, y + h / 2 + 11, sub, L.tablet ? 14 : 9.9); }
    else label(g, x + w / 2, y + h / 2 + 4.5, n, L.tablet ? Math.max(18,Math.min(26,h*.34)) : 12.6, SANS, ink);
    if (name === "wide" && BTNCAP[B[n]]) el("text", { x: x + w - 6, y: y + 12, "text-anchor": "end", class: "hint small" }, g).textContent = BTNCAP[B[n]];
    buttons.push({ g, bit: B[n], name: sub ? `${n} / ${sub}` : n });
  };
  button(L.oct[0], "OCT-");
  button(L.oct[1], "OCT+");
  const [gx, gy, gw, gh, dx, dy] = L.grid;
  for (let i = 0; i < 6; i++) {
    button([gx + i * dx, gy, gw, gh], ROW1[i]);
    button([gx + i * dx, gy + dy, gw, gh], ROW2[i], ROW2[i] === "PLAY" ? "STOP" : "");
  }
  const [x0, y0, w, h] = L.keybed;
  if (!L.tablet) el("rect", { x: x0, y: y0, width: w, height: h, rx: 18, fill: "var(--tray)" });
  // the keys: two rows of the same black keys, each with a slit down its middle that lights
  const pitch = (w - 24) / (L.compactKeys ? 8 : 16), kw = pitch * 0.78, kh = L.compactKeys ? h*.215 : h * 0.425, sw = Math.max(3, kw * 0.14);
  const key = (cx, cy, k) => {
    const g = el("g", {}), y = cy - kh / 2;
    el("rect", { x: cx - kw / 2, y, width: kw, height: kh, rx: kw / 2, class: "face key" }, g);
    el("rect", { x: cx - sw / 2, y: y + kw * 0.4, width: sw, height: kh - kw * 0.8, rx: sw / 2, class: "slit" }, g);
    if ((name === "wide" || L.tablet) && KEYCAP[k]) el("text", { x: cx, y: y + kh - 5, "text-anchor": "middle", class: "hint" }, g).textContent = L.tablet ? keyName(k) : KEYCAP[k];
    el("rect", { x: cx - pitch / 2, y: cy - (L.compactKeys ? h*.1225 : h * 0.24), width: pitch, height: L.compactKeys ? h*.245 : h * 0.48, fill: "transparent" }, g);   // its target
    keys[k] = { g, name: keyName(k) };
  };
  if(L.compactKeys){
    WHITE.forEach((k,i)=>key(x0+12+(i%8+.5)*pitch,y0+h*(i<8?.375:.875),k));
    BLACK_AFTER.forEach((a,j)=>key(Math.min(x0+w-12-kw/2,x0+12+(a%8+1)*pitch),y0+h*(a<8?.125:.625),BLACK[j]));
  }else{
    WHITE.forEach((k, i) => key(x0 + 12 + (i + 0.5) * pitch, y0 + h * 0.75, k));
    BLACK_AFTER.forEach((a, j) => key(x0 + 12 + (a + 1) * pitch, y0 + h * 0.25, BLACK[j]));
  }
  // HTML targets over the drawing (pointer-events: none): touch-action and pointer capture work on them everywhere
  const hit = (c, cls, role) => {
    const b = c.g.getBBox(), d = document.createElement("div");
    d.className = "hit" + (cls ? " " + cls : "");
    Object.assign(d.style, { left: `${b.x / L.w * 100}%`, top: `${b.y / L.h * 100}%`,
                             width: `${b.width / L.w * 100}%`, height: `${b.height / L.h * 100}%` });
    d.setAttribute("role", role);
    d.setAttribute("aria-label", c.name);
    d.title = c.name;
    d.tabIndex = 0;
    d.addEventListener("touchstart", (e) => e.preventDefault(), { passive: false });
    d.addEventListener("contextmenu", (e) => e.preventDefault());
    device.insertBefore(d, document.getElementById("start"));
    c.hit = d;
  };
  knobs.forEach((k) => {
    hit(k, "knob", k.role === "master" ? "slider" : "button");
    k.hit.setAttribute("aria-description", k.role === "master" ? "Volume. Drag up or down, scroll, or use arrow keys." : "Drag up or down, scroll, or use arrow keys. Each step is one detent.");
    if(k.role==="master"){k.hit.setAttribute("aria-valuemin","0");k.hit.setAttribute("aria-valuemax","100");k.hit.setAttribute("aria-valuenow",Math.round(masterValue*100/4096));}
    bindKnob(k); drawKnob(k);
  });
  buttons.forEach((b) => { hit(b, "", "button"); bindButton(b); });
  keys.forEach((k, i) => { hit(k, "", "button"); bindKey(k, i); });
  syncButtons(true);
  syncKeys(true);
  drawLights(lastLeds);
  enablePanel(!!node);
}

// ------------------------------------------------------------------ input
function syncButtons(redraw) {
  let m = latched;
  for (const b of btnBy.values()) m |= 1 << b;
  if (m !== btnMask) { btnMask = m; send({ type: "buttons", mask: m }); }
  else if (!redraw) return;
  for (const b of buttons) {
    const on = !!(m & (1 << b.bit));
    b.g.classList.toggle("held", on);
    b.g.classList.toggle("latched", !!(latched & (1 << b.bit)));
    b.hit.setAttribute("aria-pressed", on);
  }
}
function syncKeys(redraw) {
  let m = 0;
  for (const k of keyBy.values()) m |= 1 << k;
  if (m !== keyMask) { keyMask = m; send({ type: "keys", mask: m }); }
  else if (!redraw) return;
  keys.forEach((k, i) => k.g.classList.toggle("held", !!(m & (1 << i))));
}
function bindButton(b) {
  bindAccessibleClick(b.hit, btnBy, b.bit, syncButtons);
  b.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    if (!node) return;
    if (e.shiftKey || e.button === 2) { latched ^= 1 << b.bit; syncButtons(true); return; }
    if (latched & (1 << b.bit)) { latched &= ~(1 << b.bit); syncButtons(true); return; }
    b.hit.setPointerCapture(e.pointerId);
    btnBy.set("p" + e.pointerId, b.bit);
    syncButtons();
  });
  b.hit.addEventListener("keydown", (e) => {
    if (node && (e.key === "Enter" || e.key === " ") && !e.repeat) { btnBy.set("f" + b.bit, b.bit); syncButtons(); e.preventDefault(); e.stopPropagation(); }
  });
  b.hit.addEventListener("keyup", () => { if (btnBy.delete("f" + b.bit)) syncButtons(); });
}
function bindKey(k, i) {
  bindAccessiblePress(k.hit, keyBy, i, syncKeys);
  k.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    if (!node) return;
    k.hit.setPointerCapture(e.pointerId);
    keyBy.set("p" + e.pointerId, i);
    syncKeys();
  });
}
function release(e) {
  const id = "p" + e.pointerId;
  if (btnBy.delete(id)) syncButtons();
  if (keyBy.delete(id)) syncKeys();
}
for (const t of ["pointerup", "pointercancel", "lostpointercapture"]) device.addEventListener(t, release);

function drawKnob(k) { if (k.ind) k.ind.setAttribute("transform", `rotate(${angle[k.name]} ${k.x} ${k.y})`); }
function setMaster(value) {
  masterValue = Math.max(0, Math.min(4096, value | 0));
  angle.MASTER = -135 + 270 * masterValue / 4096;
  send({ type: "master", value: masterValue });
  const k = knobs.find((x) => x.role === "master");
  if (k) { drawKnob(k); k.hit.setAttribute("aria-valuenow",Math.round(masterValue*100/4096)); }
}
function turn(k, n) {
  if (!node || !k || !n) return;
  if (k.role === "master") {
    setMaster(masterValue + n * 64);
  } else {
    angle[k.name] = (angle[k.name] + n * 18) % 360;            // 20 detents a turn
    send({ type: "enc", role: k.role, n });
  }
  if (k.role !== "master") drawKnob(k);
}
function bindKnob(k) {
  let drag = null, wheel = 0;
  k.hit.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    k.hit.setPointerCapture(e.pointerId);
    drag = { y: e.clientY, acc: 0 };
  });
  k.hit.addEventListener("pointermove", (e) => {
    if (!drag) return;
    drag.acc += drag.y - e.clientY;
    drag.y = e.clientY;
    const step = k.role === "master" ? 3 : 8, n = Math.trunc(drag.acc / step);
    if (n) { drag.acc -= n * step; turn(k, n); }
  });
  const end = () => { drag = null; };
  k.hit.addEventListener("pointerup", end);
  k.hit.addEventListener("pointercancel", end);
  k.hit.addEventListener("lostpointercapture", end);
  k.hit.addEventListener("wheel", (e) => {
    e.preventDefault();
    const dy = e.deltaMode === 1 ? e.deltaY * 33 : e.deltaY;
    if (Math.abs(dy) >= 50) { turn(k, dy < 0 ? 1 : -1); wheel = 0; return; }   // a wheel notch: one detent
    wheel -= dy;                                                                   // a trackpad: by distance
    const n = Math.trunc(wheel / 25);
    if (n) { wheel -= n * 25; turn(k, n); }
  }, { passive: false });
  k.hit.addEventListener("keydown", (e) => {
    const d = { ArrowUp: 1, ArrowRight: 1, ArrowDown: -1, ArrowLeft: -1 }[e.key];
    if (d) { turn(k, d); e.preventDefault(); e.stopPropagation(); }
  });
}
const tall = matchMedia("(orientation: portrait) and (max-width: 700px)");
build(tabletMode ? "tablet" : tall.matches ? "tall" : "wide");
tall.addEventListener("change", () => { if(!tabletMode){releaseAll(); build(tall.matches ? "tall" : "wide");} });
document.fonts?.load("16px Clayborn").then(() => build(layout), () => {});   // the labels' width: the hit areas

const ARROWS = { ArrowUp: [EN.SELECT, 1], ArrowDown: [EN.SELECT, -1], ArrowRight: [EN.ALGORITHM, 1], ArrowLeft: [EN.ALGORITHM, -1] };
addEventListener("keydown", (e) => {
  if (!node || !document.getElementById("shortcuts").checked || e.metaKey || e.ctrlKey || e.altKey || e.target.closest?.(".hit, input, select, textarea, button, a, summary")) return;
  if (ARROWS[e.code]) { const [r, n] = ARROWS[e.code]; turn(knobs.find((k) => k.role === r), n); e.preventDefault(); return; }
  const m = KEYMAP[e.code];
  if (!m) return;
  e.preventDefault();
  if (e.repeat) return;
  if (m.key !== undefined) { keyBy.set("k" + e.code, m.key); syncKeys(); }
  else { btnBy.set("k" + e.code, m.btn); syncButtons(); }
});
addEventListener("keyup", (e) => {
  const id = "k" + e.code;
  if (keyBy.delete(id)) syncKeys();
  if (btnBy.delete(id)) syncButtons();
});
function releaseAll() { latched = 0; btnBy.clear(); keyBy.clear(); syncButtons(true); syncKeys(true); }
addEventListener("blur", releaseAll);

// ------------------------------------------------------------------ screen and lights
const ctx = document.getElementById("screen").getContext("2d");
const img = ctx.createImageData(240, 240);
function drawScreen(fb) {
  const d = img.data;
  for (let i = 0; i < 240 * 240; i++) {
    const v = fb[i], p = ((v >> 8) | (v << 8)) & 0xFFFF;          // the panel's byte order -> RGB565
    d[i * 4] = ((p >> 11) & 31) * 255 / 31;
    d[i * 4 + 1] = ((p >> 5) & 63) * 255 / 63;
    d[i * 4 + 2] = (p & 31) * 255 / 31;
    d[i * 4 + 3] = 255;
  }
  ctx.putImageData(img, 0, 0);
}
// the power-on sweep (hal/fm1_led_anim.h): a level per LED, lit frames /64 (| 128: the glow under them), drawn as the
// LED's share of lit between its dark (or glowing) colour and the lit one, mixed in linear light
const lin = (h) => [1, 3, 5].map((i) => (parseInt(h.slice(i, i + 2), 16) / 255) ** 2.2);
const ANIM = { btn: [lin("#3a3934"), lin("#3a3934"), lin("#fff4d6")], slit: [lin("#3a3a3c"), lin("#77777b"), lin("#ececee")] };
function animFill(kind, q) {
  const [dark, glow, lit] = ANIM[kind], s = Math.min(q & 127, 64) / 64, base = q & 128 ? glow : dark;
  const c = base.map((v, i) => Math.round(255 * (v + (lit[i] - v) * s) ** (1 / 2.2)));
  return `rgb(${c.join(",")})`;
}
function drawAnim(anim) {
  for (const b of buttons) b.g.querySelector(".face").style.fill = anim ? animFill("btn", anim[b.bit]) : "";
  keys.forEach((k, i) => { k.g.querySelector(".slit").style.fill = anim ? animFill("slit", anim[14 + i]) : ""; });
}
function drawLights([litB, litK, dimB, dimK, dimLo = 0, brB = 0, brK = 0, midK = 0], anim = null) {
  drawAnim(anim);
  document.documentElement.classList.toggle("dimlo", !!dimLo);   // (the breath's lower peak)
  for (const b of buttons) {
    const bit = 1 << b.bit, green = b.bit === B.PLAY && (litB & (1 << PLAY_GREEN));
    b.g.classList.toggle("lit", !!(litB & bit) && !green);
    b.g.classList.toggle("green", !!green);
    b.g.classList.toggle("dim", !!(dimB & bit));
    b.g.classList.toggle("breath", !!(brB & bit));
  }
  keys.forEach((k, i) => {
    k.g.classList.toggle("lit", !!(litK & (1 << i)));
    k.g.classList.toggle("dim", !!(dimK & (1 << i)));
    k.g.classList.toggle("breath", !!(brK & (1 << i)));
    k.g.classList.toggle("mid", !!(midK & (1 << i)));
  });
}


function bindAccessibleClick(target,map,bit,sync) {
  target.addEventListener("click",e=>{if(e.detail===0&&node){map.set("a"+bit,bit);sync(true);setTimeout(()=>{map.delete("a"+bit);sync(true);},120);}});
  target.addEventListener("blur",()=>{if(map.delete("f"+bit))sync(true);});
}
function bindAccessiblePress(target,map,bit,sync) {
  bindAccessibleClick(target,map,bit,sync);
  target.addEventListener("keydown",e=>{if(e.key!==" "&&e.key!=="Enter")return;e.preventDefault();e.stopPropagation();if(node&&!e.repeat){map.set("f"+bit,bit);sync(true);}});
  target.addEventListener("keyup",e=>{if(e.key===" "||e.key==="Enter"){map.delete("f"+bit);sync(true);e.preventDefault();}});
}
function loadFlash() {
  try {const raw=JSON.parse(localStorage.getItem(STORE)||"{}"),out={};
    for(const [o,b] of Object.entries(raw))if(/^[0-3]$/.test(o)&&typeof b==="string"&&b.length<=5120)out[o]=Uint8Array.from(atob(b),c=>c.charCodeAt(0)).buffer;
    return out;
  }catch{return {};}
}
function saveFlash(data) {
  try{const raw={};for(const [o,b] of Object.entries(data)){let s="";for(const v of b)s+=String.fromCharCode(v);raw[o]=btoa(s);}localStorage.setItem(STORE,JSON.stringify(raw));
    document.getElementById("tool-status").textContent="Scene edits saved in this browser.";
  }catch{document.getElementById("tool-status").textContent="Browser storage is unavailable; saves last for this session only.";}
}
let output=null,muted=false;
const toolSay=t=>{document.getElementById("tool-status").textContent=t;document.getElementById("tablet-message").textContent=t;};
const go=document.getElementById("go"),status=document.getElementById("status");
function enablePanel(on){device.querySelectorAll(".hit").forEach(d=>{d.inert=!on;});}
enablePanel(false);
async function switchOn() {
  if(node)return;go.disabled=true;go.textContent="Starting…";let timer;
  try{
    const ac=new AudioContext({sampleRate:44100,latencyHint:"interactive"});audioContext=ac;
    const unlocked=ac.resume();
    const [r]=await Promise.all([fetch("nightclaw.wasm"),ac.audioWorklet.addModule("worklet.js?v=fm1-panel-1"),unlocked]);
    if(!r.ok)throw new Error("Instrument download: HTTP "+r.status);
    const wasm=await r.arrayBuffer(),store=loadFlash();
    node=new AudioWorkletNode(ac,"nightclaw",{numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[2]});
    output=ac.createGain();node.connect(output).connect(ac.destination);
    await new Promise((resolve,reject)=>{
      timer=setTimeout(()=>reject(new Error("The audio engine did not start in time.")),15000);
      node.onprocessorerror=()=>{releaseAll();enablePanel(false);toolSay("The audio engine stopped. Reload this page to restart.");};
      node.port.onmessage=e=>{
        const m=e.data;
        if(m.type==="ready"){
          const params=new URLSearchParams(location.search),track=Number(params.get("track")),page=Number(params.get("page"));
          if(!Object.keys(store).length&&!params.has("track")){const i=m.scenes.indexOf("RAPTOR NIGHT");if(i>=0)send({type:"scene",value:i});}
          if(params.has("track")&&Number.isInteger(track)&&track>=0&&track<7)send({type:"track",value:track});
          if(params.has("page")&&Number.isInteger(page)&&page>0&&page<28)send({type:"page",value:page});
          clearTimeout(timer);resolve();
        }else if(m.type==="error"){reject(new Error(m.message));toolSay("Audio error: "+m.message);}
        else if(m.type==="frame"){
          if(m.fb)drawScreen(m.fb);
          if(m.state){
            if(tabletMode)knobs.forEach(k=>{if(typeof k.role==="number"&&k.role>=3){const text=m.state.controls[k.role-3].name;k.caption.textContent=text.toUpperCase();k.hit.setAttribute("aria-label",k.name+": "+text);}});
            lastLeds=[m.buttons||0,m.state.keyLights||0,0,0,0];drawLights(lastLeds);const text=m.state.page+" · "+(m.sceneName||"NightClaw")+(m.state.dirty?" · Unsaved edits":"");const s=document.getElementById("screen-status");if(s.textContent!==text)s.textContent=text;}
          if(m.store)saveFlash(m.store);
        }
      };
      node.port.postMessage({type:"load",wasm,store,master:masterValue},[wasm]);
    });
    document.getElementById("start").hidden=true;enablePanel(true);
    document.getElementById("mute").disabled=document.getElementById("release").disabled=false;
    device.focus({preventScroll:true});syncButtons(true);syncKeys(true);
    send({type:"visible",value:!document.hidden});
    ac.onstatechange=()=>{if(ac.state!=="running")toolSay("Audio is paused. Press Resume audio to continue.");document.getElementById("mute").textContent=ac.state==="running"?(muted?"Unmute":"Mute"):"Resume audio";};
  }catch(e){
    clearTimeout(timer);node?.disconnect();node=null;audioContext?.close().catch(()=>{});
    enablePanel(false);go.disabled=false;go.textContent="Switch on";status.textContent="Could not start: "+e.message;
  }
}
go.addEventListener("click",switchOn);
document.getElementById("mute").addEventListener("click",async()=>{
  if(audioContext?.state!=="running"){try{await audioContext.resume();toolSay("");}catch{toolSay("Audio did not resume. Reload the page.");}return;}
  muted=!muted;output.gain.setTargetAtTime(muted?0:1,audioContext.currentTime,.015);
  document.getElementById("mute").textContent=muted?"Unmute":"Mute";document.getElementById("mute").setAttribute("aria-pressed",String(muted));
});
document.getElementById("release").addEventListener("click",releaseAll);
document.getElementById("shortcuts").addEventListener("change",releaseAll);
document.addEventListener("visibilitychange",()=>{if(document.hidden)releaseAll();send({type:"visible",value:!document.hidden});});
function focusMode(on){if(on&&!node){toolSay("Switch on before enlarging the screen.");return;}document.body.classList.toggle("focus-mode",on);canvas.setAttribute("aria-label",on?"Live FM-1 screen; activate to return to panel":"Live FM-1 screen; activate to enlarge");document.getElementById("focus-mode").textContent=on?"Return to panel":"Enlarge screen";}
const toggleFocus=()=>focusMode(!document.body.classList.contains("focus-mode"));
canvas.addEventListener("click",toggleFocus);canvas.addEventListener("keydown",e=>{if(e.key==="Enter"||e.key===" "){e.preventDefault();toggleFocus();}});
document.getElementById("focus-mode").addEventListener("click",toggleFocus);
addEventListener("keydown",e=>{if(e.key==="Escape")focusMode(false);});

// Tablet view shares the running instrument and physical input handlers.
function setTablet(on,updateUrl=true){
  if(tabletMode!==on){releaseAll();focusMode(false);}
  tabletMode=on;document.body.classList.toggle("tablet-mode",on);
  if(updateUrl){const url=new URL(location.href);if(on)url.searchParams.set("view","tablet");else url.searchParams.delete("view");history.pushState(null,"",url);}
  build(on?"tablet":tall.matches?"tall":"wide");
  if(!on&&(document.fullscreenElement||document.webkitFullscreenElement)){const exit=document.exitFullscreen||document.webkitExitFullscreen;Promise.resolve(exit?.call(document)).catch(()=>{});}
}
document.getElementById("tablet-mode").addEventListener("click",e=>{e.preventDefault();setTablet(true);});
document.getElementById("panel-mode").addEventListener("click",()=>setTablet(false));
addEventListener("popstate",()=>setTablet(new URLSearchParams(location.search).get("view")==="tablet",false));
const fullButton=document.getElementById("fullscreen");
fullButton.addEventListener("click",async()=>{
  releaseAll();
  try{
    if(document.fullscreenElement||document.webkitFullscreenElement){const exit=document.exitFullscreen||document.webkitExitFullscreen;await exit.call(document);return;}
    const request=document.documentElement.requestFullscreen||document.documentElement.webkitRequestFullscreen;
    if(request)await request.call(document.documentElement);
    else toolSay("Tablet view fills this window. This browser does not offer fullscreen; use Add to Home Screen to hide its bars.");
  }catch{toolSay("Fullscreen was unavailable. Tablet view still fills this window.");}
});
function fullscreenChanged(){
  releaseAll();fullButton.textContent=(document.fullscreenElement||document.webkitFullscreenElement)?"Exit fullscreen":"Fullscreen";
}
document.addEventListener("fullscreenchange",fullscreenChanged);
document.addEventListener("webkitfullscreenchange",fullscreenChanged);
let tabletResizeFrame=0,lastTabletSize="";
const tabletObserver=new ResizeObserver(()=>{
  if(!tabletMode)return;
  cancelAnimationFrame(tabletResizeFrame);
  tabletResizeFrame=requestAnimationFrame(()=>{
    const r=device.getBoundingClientRect(),size=Math.round(r.width)+"x"+Math.round(r.height);
    if(size!==lastTabletSize){lastTabletSize=size;releaseAll();build("tablet");}
  });
});
tabletObserver.observe(device);
