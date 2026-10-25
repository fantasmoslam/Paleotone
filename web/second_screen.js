// SPDX-License-Identifier: GPL-3.0-only
// A separate, silent renderer: local UI projection only; no device input API.
export function viewSnapshot(bytes, layout, viewId, track = 'follow') {
  const view = layout.views.find(v => v.id === viewId);
  if (!view) throw new Error('Unknown second-screen view');
  const offsets = Object.values(layout.ui).concat(layout.selectedTrack);
  if (!offsets.every(n => Number.isInteger(n) && n >= 0 && n < bytes.length))
    throw new Error('Invalid second-screen snapshot layout');
  if (track !== 'follow' && !['0','1','2','3'].includes(String(track)))
    throw new Error('Invalid second-screen track');
  const copy = bytes.slice();
  copy[layout.ui.home] = view.home;
  copy[layout.ui.page] = view.page;
  for (const key of ['menu','confirm','layer','uboot','msg_t','hot_t']) copy[layout.ui[key]] = 0;
  if (track !== 'follow') copy[layout.selectedTrack] = Number(track);
  return copy;
}

export function readContext(bytes, layout) {
  const context={track:bytes[layout.selectedTrack]};
  for(const key of ['home','page','bank','cursor','lane']) context[key]=bytes[layout.ui[key]];
  return context;
}

export function contextSnapshot(bytes,layout,context) {
  const copy=bytes.slice();
  for(const key of ['home','page','bank','cursor','lane']) copy[layout.ui[key]]=context[key];
  copy[layout.selectedTrack]=context.track;
  for(const key of ['menu','confirm','layer','uboot','msg_t','hot_t'])copy[layout.ui[key]]=0;
  return copy;
}

export class SecondScreen {
  constructor(ex, layout) {
    if ((ex.web_remote_id() >>> 0) !== layout.rendererId || ex.web_remote_schema() !== layout.schema)
      throw new Error('Second-screen layout does not match this renderer');
    this.ex = ex; this.layout = layout;
  }
  pixels(bytes, viewId, track) {
    return this.rawPixels(viewSnapshot(bytes, this.layout, viewId, track));
  }
  rawPixels(copy) {
    const ex = this.ex;
    if (copy.length > ex.web_remote_capacity()) throw new Error('Second-screen snapshot too large');
    new Uint8Array(ex.memory.buffer, ex.web_remote_buffer(), copy.length).set(copy);
    if (!ex.web_remote_apply(copy.length)) throw new Error('Second-screen renderer rejected snapshot');
    return new Uint16Array(ex.memory.buffer, ex.web_screen(), 240 * 240);
  }
}

export function joinScreens(left, right) {
  if (left.length !== 240*240*4 || right.length !== left.length) throw new Error('Invalid screen size');
  const out = new Uint8ClampedArray(480*240*4);
  for (let y=0; y<240; y++) {
    out.set(left.subarray(y*960, (y+1)*960), y*1920);
    out.set(right.subarray(y*960, (y+1)*960), y*1920+960);
  }
  return out;
}
