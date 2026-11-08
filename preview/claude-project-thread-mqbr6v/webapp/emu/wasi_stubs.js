// SPDX-License-Identifier: GPL-3.0-only
// paleotone.wasm is built against wasi-libc (web/emu/build.sh) but uses none of the system: its few WASI imports
// (the environment, exit) answer "empty" here, in the AudioWorklet and in the Node test alike.
// wasiStubs(() => memory): the module's memory, once it is instantiated.
export function wasiStubs(memory) {
  const zero = (...ptrs) => { const v = new DataView(memory().buffer); for (const p of ptrs) v.setUint32(p, 0, true); return 0; };
  return {
    wasi_snapshot_preview1: {
      environ_get: () => 0,
      environ_sizes_get: (count, size) => zero(count, size),
      proc_exit: (code) => { throw new Error(`paleotone.wasm exited (${code})`); },
    },
  };
}
