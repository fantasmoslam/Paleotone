// SPDX-License-Identifier: GPL-3.0-only
// The actual firmware runs only on the audio thread. The page sends bounded commands.
class NightClaw extends AudioWorkletProcessor {
  constructor() {
    super(); this.ex = null; this.mem = null; this.lastFrame = -1; this.lastBlit = -1;
    this.lastWrites = -1; this.visible = true; this.phase = 0; this.sourceIndex = 128;
    this.aL = this.aR = this.bL = this.bR = 0; this.primed = false;
    this.port.onmessage = (e) => this.message(e.data);
  }
  str(p) { const b = new Uint8Array(this.mem.buffer); let s = ""; for (let i = p; b[i] && i < p + 128; i++) s += String.fromCharCode(b[i]); return s; }
  async message(m) {
    try {
      if (m.type === "load") {
        const wasi = new Proxy({
          clock_time_get: (id, prec, p) => { new DataView(this.mem.buffer).setBigUint64(p, BigInt(Math.round(currentTime * 1e9)), true); return 0; },
          fd_write: (fd, iov, n, p) => { new DataView(this.mem.buffer).setUint32(p, 0, true); return 0; },
          proc_exit: () => {},
        }, { get: (t, k) => t[k] || (() => 0) });
        const { instance } = await WebAssembly.instantiate(m.wasm, { wasi_snapshot_preview1: wasi, env: new Proxy({}, { get: () => () => 0 }) });
        this.ex = instance.exports; this.mem = this.ex.memory;
        this.ex._initialize?.();
        for (const [o, b] of Object.entries(m.store || {})) {
          if (!/^\d+$/.test(o) || +o >= this.ex.web_nobj() || !(b instanceof ArrayBuffer) || b.byteLength > this.ex.web_store_max()) continue;
          new Uint8Array(this.mem.buffer, this.ex.web_store(+o), b.byteLength).set(new Uint8Array(b));
          this.ex.web_store_set_len(+o, b.byteLength);
        }
        this.ex.web_master(Number.isFinite(m.master)?Math.max(0,Math.min(4096,m.master|0)):2800); this.ex.web_boot(); this.ex.nc_track(2);
        this.left = new Float32Array(this.mem.buffer, this.ex.web_out_l(), 1024);
        this.right = new Float32Array(this.mem.buffer, this.ex.web_out_r(), 1024);
        this.lastWrites = this.ex.web_store_writes();
        this.port.postMessage({ type: "ready", scenes: Array.from({ length: this.ex.nc_scene_count() }, (_, i) => this.str(this.ex.nc_scene_name(i))), pages: Array.from({ length: this.ex.nc_page_count() }, (_, i) => this.str(this.ex.nc_page_title(i))) });
        this.publish();
        return;
      }
      if (!this.ex) return;
      const ex = this.ex, n = Number.isFinite(m.value) ? Math.trunc(m.value) : 0;
      switch (m.type) {
        case "scene": ex.web_keys(0); ex.nc_scene(n); break;
        case "track": ex.web_keys(0); ex.nc_track(n); break;
        case "page": ex.web_keys(0); ex.nc_page(n); break;
        case "control": ex.nc_control(m.col | 0, n); break;
        case "voice": ex.nc_voice(n); break;
        case "keys": ex.web_keys((m.mask >>> 0) & 0x7ffffff); break;
        case "buttons": ex.web_buttons((m.mask >>> 0) & 0x3fff); break;
        case "enc": ex.web_enc(m.role | 0, Math.max(-100, Math.min(100, Number.isFinite(m.n)?Math.trunc(m.n):n))); break;
        case "master": ex.web_master(Math.max(0,Math.min(4096,n))); break;
        case "play": ex.nc_play(n); break;
        case "hold": ex.nc_hold(n); break;
        case "auto": ex.nc_calls_auto(n); break;
        case "save": ex.nc_save(); break;
        case "octave": ex.nc_octave(n); break;
        case "visible": this.visible = !!m.value; break;
      }
      if (this.visible) this.publish();
    } catch (e) { this.port.postMessage({ type: "error", message: String(e.message || e) }); }
  }
  publish() {
    const ex = this.ex, st = new Int32Array(this.mem.buffer, ex.nc_state(), 32);
    const state = {
      scene: st[0], view: st[1], track: st[2], playing: !!st[3], held: !!st[4], dirty: !!st[5],
      auto: !!st[6], voice: st[7], context: st[9], page: this.str(ex.nc_page_name()),
      voices: Array.from({ length: st[8] }, (_, i) => this.str(ex.nc_voice_name(i))),
      controls: Array.from({ length: 4 }, (_, i) => ({ name: this.str(ex.nc_control_name(i)), text: this.str(ex.nc_control_text(i)),
        value: st[12 + i * 3], min: st[13 + i * 3], max: st[14 + i * 3] })),
      keyLights: ex.web_lit_keys() >>> 0,
    };
    const msg = { type: "frame", state, buttons: ex.web_lit_buttons() >>> 0, sceneName: this.str(ex.nc_scene_name(st[0])) }, transfer = [];
    if (ex.web_blits() !== this.lastBlit) {
      this.lastBlit = ex.web_blits();
      msg.fb = new Uint16Array(this.mem.buffer, ex.web_fb(), 240 * 240).slice(); transfer.push(msg.fb.buffer);
    }
    if (ex.web_store_writes() !== this.lastWrites) {
      this.lastWrites = ex.web_store_writes(); msg.store = {};
      for (let i = 0; i < ex.web_nobj(); i++) { const n = ex.web_store_len(i); if (n >= 0) msg.store[i] = new Uint8Array(this.mem.buffer, ex.web_store(i), n).slice(); }
    }
    this.port.postMessage(msg, transfer);
  }
  source() {
    if (this.sourceIndex >= 128) { this.ex.web_render(128); this.sourceIndex = 0; }
    this.bL = this.left[this.sourceIndex]; this.bR = this.right[this.sourceIndex++];
  }
  process(inputs, outputs) {
    if (!this.ex) return true;
    const out = outputs[0]; if (!out?.[0]) return true;
    try {
      if (sampleRate === 44100) {
        this.ex.web_render(out[0].length);
        out[0].set(this.left.subarray(0, out[0].length)); if (out[1]) out[1].set(this.right.subarray(0, out[0].length));
      } else {
        // Preserve the firmware's pitch/tempo on browsers with a different output rate.
        if (!this.primed) { this.source(); this.aL = this.bL; this.aR = this.bR; this.source(); this.primed = true; }
        const step = 44100 / sampleRate;
        for (let i = 0; i < out[0].length; i++) {
          out[0][i] = this.aL + (this.bL - this.aL) * this.phase;
          if (out[1]) out[1][i] = this.aR + (this.bR - this.aR) * this.phase;
          this.phase += step;
          while (this.phase >= 1) { this.phase--; this.aL = this.bL; this.aR = this.bR; this.source(); }
        }
      }
      if (this.visible && currentTime - this.lastFrame >= 1 / 30) { this.lastFrame = currentTime; this.publish(); }
    } catch(e) {
      this.port.postMessage({type:"error", message:String(e.message || e)}); this.ex=null;
      out.forEach(ch=>ch.fill(0));
    }
    return true;
  }
}
registerProcessor("nightclaw", NightClaw);
