// SPDX-License-Identifier: GPL-3.0-only
// PALEOTONE: the screen page's forward. The reply below was recorded from a real FM-1 running the
// unified firmware (renderer 2540340739, schema 2, revision 1, 14001-byte snapshot, capability 68).
import assert from "node:assert/strict";
import { decodeStatus, choosePage } from "./screen_router.js";

// the payload the companion's status request is answered with: op, ok, schema, the id as five 7-bit
// bytes, rev, base, length, 16 dirty-block masks, then (schema 2) the capability byte
const dirty = [];
for (let i = 0; i < 13; i++) dirty.push(0x7F, 0x01);        // blocks 0..12 dirty
dirty.push(0x3F, 0x00, 0x00, 0x00, 0x00, 0x00);            // block 13 partly, 14 and 15 clean
const payload = [1, 0, 2, 3, 12, 42, 59, 9, 1, 0, 0, 0, 49, 109, ...dirty, 68];
assert.equal(payload.length, 47, "the reply the firmware sends");

// HEADER, the echoed command, the payload, F7 (as Web MIDI delivers it)
const reply = [0xF0, 0x7D, 0x46, 0x4C, 92, ...payload, 0xF7];
assert.equal(decodeStatus(reply), 2540340739, "the unified firmware's renderer identity");

// the older generation leaves the echoed command out: the same id, one byte earlier
const older = [0xF0, 0x7D, 0x46, 0x4C, ...payload.slice(0, 46), 0xF7];
assert.equal(older.slice(4, -1).length, 46, "the older layout's payload");
assert.equal(decodeStatus(older), 2540340739, "an older companion firmware's reply");

// nothing to act on: absent, truncated, refused, or an impossible identity
assert.equal(decodeStatus(null), 0, "no reply");
assert.equal(decodeStatus([0xF0, 0x7D, 0x46, 0x4C, 92, 1, 0xF7]), 0, "a truncated reply");
assert.equal(decodeStatus([0xF0, 0x7D, 0x46, 0x4C, 92, ...payload.map((v, i) => (i === 1 ? 1 : v)), 0xF7]), 0,
    "a refused capture (ok != 0)");

const pages = {pages: [
    {rendererId: 534039429, path: "screen/", label: "the older generation"},
    {rendererId: 2540340739, path: "screen-unified/", label: "the unified generation"},
]};
const here = "https://example.test/dinobox-fm1/webapp/screen/";
assert.equal(choosePage(pages, 2540340739, here), "https://example.test/dinobox-fm1/webapp/screen-unified/",
    "a newer instrument is forwarded to its page");
assert.equal(choosePage(pages, 534039429, here), "", "this page's own generation stays");
assert.equal(choosePage(pages, 12345, here), "", "an unknown build stays (the page reports it)");
assert.equal(choosePage({}, 2540340739, here), "", "no registry shipped");

console.log("screen_router: the page forwards to the build its firmware needs, and only then");
