# NightClaw browser player and visual tutorial

The player runs NightClaw's actual application, UI, project storage and DSP in WebAssembly. It does not substitute recordings or a JavaScript sound-alike engine. This is a separate browser build; the installed FM-1 firmware does not change.

Public project page: https://fantasmoslam.github.io/Paleotone/nightclaw/

## Use

Press **Start audio** once. Choose a scene, choose a track, select its instrument or beat, and use the four colored controls. The live 240 × 240 firmware screen is enlarged alongside these controls. Home, Key, Space, Performance and Chords are directly accessible; the remaining sound pages are in More sound pages.

The sixteen white and eleven black keys support mouse, touch and keyboard focus. Enable Computer keyboard for Q–I and A–K white-key shortcuts, and 1–0 / minus black-key shortcuts. Shortcuts do not intercept form controls or tutorial navigation. Losing focus, hiding the page, changing scene/track/page or pressing Release all keys releases held inputs.

Calls and Performance preserve the firmware's context-dependent keyboard roles. Their explanations change with the page. Manual creature calls remain the boot default.

**Save browser session** stores the firmware's saved objects in this browser. It does not read or write a connected FM-1. Unavailable browser storage is reported. Listening volume and Mute are browser output controls.

## Visual tutorial

Eight chapters cover scenes, Home's four columns, independent voice selection, Skin beats, manual creature calls, Performance, Key/Chords and saving. Each chapter has a labeled FM-1 diagram, highlighted controls, a real firmware screen and numbered actions.

The tutorial does not change the music as you navigate. Show this on the player explicitly selects the illustrated track/page when audio is running. It does not select a different instrument or beat for you.

Hardware instructions distinguish the GLO half-second save gesture from the printed SAVE sound-page button, and warn that holding GLO to 1.5 seconds resets the current scene. Octave instructions identify the Lead page on hardware; browser octave buttons also support the selected Home melodic track.

## Build

```sh
sh web/nightclaw/build.sh
node tests/host/nightclaw_web_test.mjs build/nightclaw
python3 -m http.server 8000 --bind 127.0.0.1 --directory build/nightclaw
```

Requires Emscripten, Python/Pillow/fonttools and Node. Serve over localhost or HTTPS; opening an HTML file directly is insufficient for AudioWorklet. The generated build/nightclaw directory is a self-contained static site with local fonts and images.

Source adapter: web/nightclaw/nightclaw_web.c. It includes the existing host-based emulator and adds bounded browser commands and read-only metadata. No native firmware source, musical recipe or DSP code is changed. The browser runs the renderer serially; it does not reproduce the FM-1's two-core scheduling or hardware CPU measurement.

## Validation

The shipped WASM passes checks for all twenty scenes producing bounded finite audio; voice/beat independence; parameter steps; calls; page contexts; octave control; transport; and save/reboot restoration. AudioWorklet tests execute the real processor with the shipped WASM at 44.1 and 48 kHz, including timing-preserving resampling, screen transfer, persisted bytes and background message suppression.

JavaScript syntax and static asset/element references are checked locally. A connected browser was unavailable during development, so visual layout, real browser interaction and subjective listening remain unverified. Node engine/worklet tests do not establish those results.

The NightClaw browser GitHub workflow rebuilds and tests the player when its sources or firmware dependencies change, then publishes only the nightclaw/ subtree of gh-pages. The existing Paleotone site is preserved. The firmware download is pinned to the verified installed f9b11d6 package. TOTAL_STACK retains compatibility with Ubuntu 24.04’s Emscripten while newer versions also accept it. Code, font licenses and taiko attribution ship with the page.
