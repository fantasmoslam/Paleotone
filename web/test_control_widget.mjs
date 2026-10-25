// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
const html=await readFile(new URL('./screen.html',import.meta.url),'utf8');
assert.match(html,/HOME/);assert.match(html,/PhysicalFocus/);
assert.doesNotMatch(html,/ControlTarget|inputCommand|\.turn\(|\.tap\(|shiftTarget|id="target-(left|right|shift)"/,
  'page swapping must not synthesize instrument input from browser or keyboard');
assert.match(html,/await focus.poll\(enabled\)/);
assert.match(html,/focus.snapshot\(\)/);
assert.match(html,/contextSnapshot\(bytes,second.layout,state\[otherIndex/);
assert.match(html,/panes\[activeIndex\].update\(active\)/);
assert.match(html,/focusDue=performance.now\(\)\+750/);
assert.match(html,/focus.configure\(view/);
const script=html.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
new Function(script.replace(/^import .*?;\s*$/mg,'')); // parse the complete inline module body
console.log('device screen page: physical focus, two retained panes, optional manual setup and no synthesized controls passed');
