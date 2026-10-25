// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {join,resolve} from 'node:path';
import {createHash} from 'node:crypto';
const dir=resolve(process.argv[2]),metadata=JSON.parse(await readFile(join(dir,'build.json'),'utf8'));
assert.equal(metadata.presentation,'device-owned-focus');
assert.match(metadata.source,/^[0-9a-f]{40}$/);
assert.ok(['paleotone-device-controls.fwsc','paleotone-unified.fwsc'].includes(metadata.firmware.file));
const bytes=await readFile(join(dir,metadata.firmware.file));
assert.equal(bytes.length,metadata.firmware.size);
assert.equal(createHash('sha256').update(bytes).digest('hex'),metadata.firmware.sha256);
const html=await readFile(join(dir,'index.html'),'utf8');
assert.doesNotMatch(html,/ControlTarget|inputCommand|shiftTarget/);
assert.match(html,/PhysicalFocus/);
console.log('device release: complete package, exact firmware hash, device-owned focus and guide passed');
