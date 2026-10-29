// SPDX-License-Identifier: GPL-3.0-only
// PALEOTONE: send the visitor to the screen page built for the firmware they have.
//
// The companion can only ask the instrument what it is once the visitor has pressed Connect (a
// browser gives no MIDI access before a gesture), so the check rides on that first request: this
// module wraps navigator.requestMIDIAccess, asks the FM-1 for its renderer identity, and if the
// page in front of the visitor is not the one built for it, replaces the page with the one that is
// (web/webapp/screens.json, written by web/make_site.py). The page's own version check never runs:
// it is handed a request that never settles. Anything unexpected leaves the page as it was.
//
// The identity is the reply to the companion's status request (command 92, op 1): the same bytes
// web/remote_screen.js parses, read here before its DisplaySync exists.
export const HEADER = [0xF0, 0x7D, 0x46, 0x4C];
export const STATUS = 92;
const TIMEOUT_MS = 1500;

/* the renderer id in a status reply, or 0. The reply is HEADER, the echoed command, then the
 * payload: op, ok, schema, the id as five 7-bit bytes, rev, base, length, the dirty-block mask and
 * (schema 2) one byte of capability. The echo is skipped when it is there (5..) and the older
 * layout is tried too (4..), so this reads both generations of companion firmware. */
export function decodeStatus(data) {
    if (!data || data.length < 10 || data[0] !== 0xF0) return 0;
    for (const start of [5, 4]) {
        const p = data.slice(start, data.length - 1);          // (drop the trailing F7)
        if ((p.length === 46 || p.length === 47) && p[1] === 0 && p[7] <= 15) {
            const id = (p[3] | p[4] << 7 | p[5] << 14 | p[6] << 21 | p[7] << 28) >>> 0;
            return id || 0;
        }
    }
    return 0;
}

/* the page for a renderer id: an entry of screens.json ({pages: [{rendererId, path, label}]},
 * path relative to the site's webapp/), unless it is the page we are already on */
export function choosePage(pages, id, here) {
    const list = Array.isArray(pages) ? pages : pages?.pages;
    if (!Array.isArray(list) || !id) return "";
    const hit = list.find((p) => (Number(p.rendererId) >>> 0) === id && p.path);
    if (!hit) return "";
    let url;
    try { url = new URL("../" + hit.path, here); } catch { return ""; }
    return url.href === here ? "" : url.href;
}

async function json(url) {
    const r = await fetch(url, {cache: "no-store"});
    if (!r.ok) throw new Error(String(r.status));
    return r.json();
}

/* the FM-1's renderer id, read over the access the page just asked for; 0 when it does not answer */
export async function readRendererId(access) {
    const inputs = [...(access.inputs?.values?.() ?? [])];
    const outputs = [...(access.outputs?.values?.() ?? [])];
    const input = inputs.find((p) => /felucca/i.test(p.name)) ?? inputs[0];
    const output = outputs.find((p) => /felucca/i.test(p.name)) ?? outputs[0];
    if (!input || !output) return 0;
    let reply = null;
    await input.open();
    await output.open();
    try {
        reply = await new Promise((resolve) => {
            const timer = setTimeout(() => resolve(null), TIMEOUT_MS);
            input.onmidimessage = (e) => {
                const d = e.data;
                if (d?.[0] === 0xF0 && d?.[1] === HEADER[1] && d?.[2] === HEADER[2] && d?.[3] === HEADER[3]) {
                    clearTimeout(timer);
                    resolve(d);
                }
            };
            output.send([...HEADER, STATUS, 1, 0xF7]);
        });
    } finally {
        if (input.onmidimessage) input.onmidimessage = null;
        await input.close().catch(() => {});
        await output.close().catch(() => {});
    }
    return decodeStatus(reply);
}

/* fetch the identities, compare, and forward: true when the page is being replaced */
export async function route(access, here, fetchJson = json) {
    const built = await fetchJson("build.json");                 // (this page's own identity)
    const expected = Number(built?.rendererId) >>> 0;
    if (!expected) return false;
    const id = await readRendererId(access);
    if (!id || id === expected) return false;
    const target = choosePage(await fetchJson("../screens.json"), id, here);
    if (!target) return false;
    location.replace(target);
    return true;
}

/* wrap the one request the page makes for MIDI; only the first is checked */
export function install(win = globalThis) {
    const nav = win.navigator;
    const original = nav?.requestMIDIAccess;
    if (typeof original !== "function" || original.screenRouterWrapped) return false;
    let checked = false;
    const wrapped = function requestMIDIAccess(...args) {
        const access = original.apply(this, args);
        if (checked) return access;
        checked = true;
        return access.then(async (granted) => {
            try {
                if (await route(granted, win.location.href)) return new Promise(() => {});   // (replaced)
            } catch { /* an unreadable answer leaves the page as it was */ }
            return granted;
        });
    };
    wrapped.screenRouterWrapped = true;
    nav.requestMIDIAccess = wrapped;
    return true;
}

if (typeof window !== "undefined" && typeof document !== "undefined") install();
