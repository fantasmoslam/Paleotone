// SPDX-License-Identifier: GPL-3.0-only
export const HEADER = [0xF0, 0x7D, 0x46, 0x4C];
export const u14 = (n) => [n & 127, n >> 7 & 127];
export const r14 = (a, i = 0) => a[i] | a[i + 1] << 7;
export function unpack7(a, count) {
  const out = new Uint8Array(count);
  let i = 0, j = 0;
  while (j < count) {
    const n = Math.min(7, count - j), mask = a[i++];
    if (mask === undefined || mask >> n) throw new Error("Invalid pixel/state packing");
    for (let k = 0; k < n; k++) {
      const v = a[i++];
      if (!Number.isInteger(v) || v < 0 || v > 127) throw new Error("Truncated state block");
      out[j++] = v | ((mask >> k & 1) << 7);
    }
  }
  if (i !== a.length) throw new Error("Trailing state bytes");
  return out;
}
export function parseStatus(p) {
  if (p.length !== 46 || p[1]) throw new Error("Device could not capture its display state");
  if (p[7] > 15) throw new Error("Invalid renderer identity");
  const id = (p[3] | p[4] << 7 | p[5] << 14 | p[6] << 21 | p[7] << 28) >>> 0;
  const dirty = Uint8Array.from({length: 16}, (_, i) => {
    if (p[15 + 2 * i] > 1) throw new Error("Invalid dirty-block mask");
    return p[14 + 2 * i] | p[15 + 2 * i] << 7;
  });
  const status = {op: p[0], schema: p[2], id, rev: r14(p, 8), base: r14(p, 10), length: r14(p, 12), dirty};
  if (status.length > 16384) throw new Error("Display state exceeds capacity");
  return status;
}
export class MidiTimeoutError extends Error {
  constructor(cmd) {
    super("FM-1 did not reply to MIDI request " + cmd + ". Check the USB connection.");
    this.name = "MidiTimeoutError"; this.code = "MIDI_TIMEOUT"; this.command = cmd;
  }
}
export function recoverableScreenError(e) {
  return e?.code === "MIDI_TIMEOUT" || e?.code === "SNAPSHOT_STALE";
}
export class MidiLink {
  constructor(input, output, timeout = 1200) {
    this.input = input; this.output = output; this.timeout = timeout;
    this.tail = Promise.resolve(); this.pending = null; this.closed = false;
    input.onmidimessage = (e) => this.receive(e.data);
  }
  receive(data) {
    const d = Array.from(data);
    if (d.length < 6 || !HEADER.every((v, i) => d[i] === v) || d.at(-1) !== 0xF7 ||
        d.slice(1, -1).some((v) => !Number.isInteger(v) || v < 0 || v > 127)) return;
    const p = d.slice(5, -1), pending = this.pending;
    if (pending && d[4] === pending.cmd && pending.match(p)) {
      clearTimeout(pending.timer); this.pending = null; pending.resolve(p);
    }
  }
  request(cmd, args, match = () => true) {
    const run = () => new Promise((resolve, reject) => {
      if (this.closed || this.input.state === "disconnected" || this.output.state === "disconnected")
        return reject(new Error("FM-1 disconnected"));
      const pending = {cmd, match, resolve, reject};
      pending.timer = setTimeout(() => {
        if (this.pending === pending) this.pending = null;
        reject(new MidiTimeoutError(cmd));
      }, this.timeout);
      this.pending = pending;
      try { this.output.send([...HEADER, cmd, ...args, 0xF7]); }
      catch (e) { clearTimeout(pending.timer); this.pending = null; reject(e); }
    });
    const result = this.tail.then(run, run);
    this.tail = result.catch(() => {});
    return result;
  }
  /* Read/capture/acknowledge operations affect only the display transfer.
   * Never retry an input command: a lost encoder acknowledgement must not
   * turn the knob twice. Each block matcher rejects stale/duplicate replies. */
  async readRequest(cmd, args, match = () => true) {
    if (cmd !== 92 && cmd !== 93) throw new Error("Only display transfers may be retried");
    for (let attempt = 0; ; attempt++) {
      try { return await this.request(cmd, args, match); }
      catch (e) {
        if (e?.code !== "MIDI_TIMEOUT" || attempt >= 2 || this.closed) throw e;
        this.onRetry?.({command: cmd, attempt: attempt + 1});
      }
    }
  }
  async status(op, rev) {
    return parseStatus(await this.readRequest(92, [op, ...(rev === undefined ? [] : u14(rev))], (p) => p[0] === op));
  }
  inputCommand(args) { return this.request(94, args, (p) => p.length === 2 && p[0] === args[0] && p[1] === 0); }
  close() {
    this.closed = true; this.input.onmidimessage = null;
    if (this.pending) { clearTimeout(this.pending.timer); this.pending.reject(new Error("Disconnected")); this.pending = null; }
  }
}
export class DisplaySync {
  constructor(link, id, schema, apply) {
    this.link = link; this.id = id >>> 0; this.schema = schema; this.apply = apply;
    this.bytes = null; this.rev = 0;
  }
  check(s) {
    if (s.id !== this.id || s.schema !== this.schema)
      throw new Error("Firmware and webpage versions do not match. Use the screen page supplied with this firmware build.");
  }
  async update() {
    const s = await this.link.status(1); this.check(s);
    if (!s.length || !s.rev) throw new Error("Empty device snapshot");
    const full = !this.bytes || this.bytes.length !== s.length || this.rev !== s.base;
    const next = full ? new Uint8Array(s.length) : this.bytes.slice();
    for (let block = 0; block * 128 < next.length; block++) {
      if (!full && !(s.dirty[block >> 3] & 1 << (block & 7))) continue;
      const p = await this.link.readRequest(93, [...u14(s.rev), ...u14(block)],
        (p) => p.length >= 5 && r14(p) === s.rev && r14(p, 2) === block);
      if (p[4] || p.length < 7) {
        const e = new Error("Display snapshot changed; requesting a fresh screen");
        e.code = "SNAPSHOT_STALE"; throw e;
      }
      const count = r14(p, 5), expected = Math.min(128, next.length - block * 128);
      if (count !== expected) throw new Error("Wrong display block size");
      next.set(unpack7(p.slice(7), count), block * 128);
    }
    if (!this.apply(next)) throw new Error("Renderer rejected the device state");
    const ack = await this.link.status(2, s.rev); this.check(ack);
    if (ack.base !== s.rev) {
      const e = new Error("Display synchronization was interrupted");
      e.code = "SNAPSHOT_STALE"; throw e;
    }
    this.bytes = next; this.rev = s.rev;
    return {bytes: next.length, full};
  }
}
export function rgb565(pixels) {
  const rgba = new Uint8ClampedArray(pixels.length * 4);
  for (let i = 0; i < pixels.length; i++) {
    const sw = pixels[i], c = (sw >>> 8 | sw << 8) & 65535;
    rgba[4*i] = Math.round((c >> 11) * 255 / 31);
    rgba[4*i+1] = Math.round((c >> 5 & 63) * 255 / 63);
    rgba[4*i+2] = Math.round((c & 31) * 255 / 31);
    rgba[4*i+3] = 255;
  }
  return rgba;
}
