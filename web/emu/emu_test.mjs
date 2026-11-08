// SPDX-License-Identifier: GPL-3.0-only
// The browser build, checked in Node: it boots, draws the splash and then HOME, plays the demo song on PLAY (sound,
// bounded, not clipping all the time), the keys play notes, the knobs reach the UI, and it runs well faster than
// real time.  node web/emu/emu_test.mjs build/emu/paleotone.wasm [OUT_DIR: screens as .ppm]
import { readFileSync, writeFileSync, mkdirSync } from "node:fs";
import { wasiStubs } from "./wasi_stubs.js";

const [wasmPath = "build/emu/paleotone.wasm", outDir] = process.argv.slice(2);
let ex = null;
const { instance } = await WebAssembly.instantiate(readFileSync(wasmPath), wasiStubs(() => ex.memory));
ex = instance.exports;
ex._initialize();
ex.web_master(700);
ex.web_boot();
let fails = 0;
const check = (ok, what) => { console.log(`emu: ${what.padEnd(70)} ${ok ? "ok" : "FAIL"}`); if (!ok) fails++; };
const B = { FX: 0, SCL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11 };
const rate = ex.web_sample_rate();
function run(seconds) {
  let peak = 0, sum = 0, n = 0, clip = 0;
  for (let f = 0; f < seconds * rate; f += 128) {
    ex.web_render(128);
    const l = new Float32Array(ex.memory.buffer, ex.web_out_l(), 128), r = new Float32Array(ex.memory.buffer, ex.web_out_r(), 128);
    for (let i = 0; i < 128; i++) {
      const a = Math.max(Math.abs(l[i]), Math.abs(r[i]));
      peak = Math.max(peak, a); sum += l[i] * l[i] + r[i] * r[i]; n += 2; clip += a >= 0.999;
    }
  }
  return { peak, rms: Math.sqrt(sum / Math.max(1, n)), clip };
}
function screen() { return new Uint16Array(ex.memory.buffer, ex.web_screen(), 240 * 240).slice(); }
function lit(fb) { let k = 0; for (const p of fb) if (p) k++; return k; }
function ppm(name) {
  if (!outDir) return;
  mkdirSync(outDir, { recursive: true });
  const fb = screen(), out = Buffer.alloc(240 * 240 * 3);
  fb.forEach((v, i) => { const p = ((v & 0xFF) << 8) | (v >> 8); out[3 * i] = (p >> 11) * 255 / 31; out[3 * i + 1] = ((p >> 5) & 63) * 255 / 63; out[3 * i + 2] = (p & 31) * 255 / 31; });
  writeFileSync(`${outDir}/${name}.ppm`, Buffer.concat([Buffer.from("P6\n240 240\n255\n"), out]));
}
const tap = (b) => { ex.web_buttons(1 << b); run(0.08); ex.web_buttons(0); run(0.15); };

run(0.3);
check(lit(screen()) > 3000, "the splash is drawn at power-on");
ppm("splash");
run(1.5);
const home = screen();
check(lit(home) > 3000, "HOME after the splash");
ppm("home");
const quiet = run(0.5);
check(quiet.peak < 0.05, "stopped: (almost) silence");
const t0 = performance.now();
tap(B.PLAY);
const play = run(6);
const speed = 6 / ((performance.now() - t0) / 1000);
check(ex.web_playing() === 1, "PLAY starts the demo song");
check(play.rms > 0.01 && play.peak <= 1.0, `the demo song sounds (rms ${play.rms.toFixed(3)}, peak ${play.peak.toFixed(2)})`);
check(play.clip < rate * 6 * 0.001, `it does not clip (${play.clip} samples at full scale)`);
check(speed > 1.5, `faster than real time (x${speed.toFixed(1)})`);
ppm("playing");
tap(B.PLAY);
run(3);
check(ex.web_playing() === 0, "PLAY again stops it");
ex.web_keys(1 << 7); const note = run(0.4); ex.web_keys(0); run(1);
check(note.rms > 0.005, "a key plays a note");
const before = screen();
ex.web_enc(2, 3); run(0.3);                       // PRESETS: the sound changes, a message bar comes up
const after = screen();
check(before.some((v, i) => v !== after[i]), "PRESETS turned: the screen changes");
ppm("presets");
tap(B.EDIT); ppm("edit");
check(ex.web_screen_draws() > 10, "the screen keeps drawing");
console.log(fails ? `emu: ${fails} FAILED` : "emu: all ok");
process.exit(fails ? 1 : 0);
