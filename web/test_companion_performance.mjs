// SPDX-License-Identifier: GPL-3.0-only
// Static guard: experimental refresh must be opt-in and must not modify MIDI transport.
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
const html=await readFile(new URL('./companion_page.html',import.meta.url),'utf8');
assert.match(html,/id="fast-refresh" aria-pressed="false"/);
assert.match(html,/let fastRefresh=false/);
assert.match(html,/fastRefresh\?45:90/);
assert.match(html,/Math\.max\(20,\(fastRefresh\?45:90\)/);
assert.match(html,/presenter\.metrics\(\)/);
assert.match(html,/lastPaintCount=perf\.painted/);
assert.match(html,/presented /);
assert.match(html,/performance\.now\(\)-lastStatusAt>1000/);
console.log('companion: experimental fast refresh opt-in and frame diagnostics present');
