# NightClaw FM-1 browser emulator and visual guide

Project page: https://fantasmoslam.github.io/Paleotone/nightclaw/

Visual guide: https://fantasmoslam.github.io/Paleotone/nightclaw/guide.html

## Tablet / fullscreen player

https://fantasmoslam.github.io/Paleotone/nightclaw/?view=tablet

Tablet view removes the hardware casing, site header and margins, using the viewport for the same running instrument. Screen, knobs, buttons and keys reflow for portrait/landscape; the four sound controls show their current parameter names. Narrow screens use two key banks to retain at least 44 px key targets. Short windows can scroll the instrument rather than shrink its controls.

Fullscreen requests browser fullscreen from a tap. Unsupported or denied fullscreen leaves the viewport layout usable and reports the limitation; Add to Home Screen can hide browser bars on devices without the API. FM-1 view returns to the physical panel without reloading audio. Window/size/fullscreen changes release held inputs before rebuilding. Existing browser saves are shared between both views.

Automated layout checks cover eleven sizes, including 768/1024-class tablets, checking bounds, screen separation, button spacing and at least 44 px touch targets. This is geometry validation, not a physical-tablet visual or multitouch test.

## Physical-panel player

The player follows Paleotone's FM-1 emulator: the eight physical knobs, fourteen buttons, twenty-seven touch keys and live firmware screen are arranged like the instrument. It uses Paleotone main 6f027f0's wide and portrait panel geometry and input approach, adapted to NightClaw's firmware and saturated palette. This replaces the earlier dashboard with scene menus, sliders and a piano keyboard.

Switch on, then drag or scroll the physical knobs. Arrow keys turn a focused knob one detent. Buttons and keys support mouse, touch and keyboard. Computer shortcuts are printed on the wide panel; the number row operates page buttons, with Z-row and upper-row shortcuts playing the keybed. Shift-click or right-click latches a button until pressed again. Losing window focus, hiding the page, or Release all controls releases every held and latched input.

The browser sends the same encoder roles and button/key masks that the host platform uses. MASTER controls the firmware render gain. GLO's half-second hold/release saves through the firmware's own gesture, persisting the resulting saved objects in this browser. The physical SAVE button opens sound pages. Holding GLO to about 1.5 seconds resets the current scene.

Click the live screen to enlarge it; click again or press Escape to return. Listening mute and keyboard/display utilities stay below the instrument. Browser storage remains separate from the physical FM-1.

## Visual tutorial

The separate guide has eight illustrated chapters for scenes, Home's track table, independent voice/beat selection, Skin drums, manual creature calls, Performance, Key/Chords and saving. Show this on the player opens the physical emulator with the illustrated track/page selected after Switch on. Navigating tutorial chapters does not change music.

The guide follows the physical panel and its keyboard shortcuts. Legacy #guide links redirect to guide.html.

## Engine and build

The player runs NightClaw's actual application, UI, project storage and DSP in WebAssembly, using the existing host-based emulator. It does not change native firmware source, DSP or scene recipes. The browser renders serially, rather than reproducing the hardware's two-core scheduling.

```sh
sh web/nightclaw/build.sh
node tests/host/nightclaw_web_test.mjs build/nightclaw
python3 -m http.server 8000 --bind 127.0.0.1 --directory build/nightclaw
```

Requires Emscripten, Python/Pillow/fonttools and Node. Serve over localhost or HTTPS for AudioWorklet. TOTAL_STACK supports Ubuntu 24.04's older Emscripten and newer compilers.

The GitHub browser workflow tests and publishes only nightclaw/ on gh-pages, preserving the rest of the existing Paleotone site. The download link is pinned to the verified f9b11d6 FM-1 package.

## Validation

The shipped WASM passes twenty audible-scene checks, independent voices/beats, calls, page contexts, octave control, transport and save/reboot restoration. Worklet tests run the shipped processor and WASM at 44.1 and 48 kHz, including correct timing, stereo output, screen transfer, persistence and background message suppression.

Physical-panel protocol tests additionally check all seven encoder roles using the panel's detent payload, page-button masks, GLO's hold/release save timing and MASTER silence at both sample rates. Main player and guide JavaScript syntax and static references are checked.

No connected browser is available for live visual/interaction/listening QA. The panel's geometry and drawing come from the reference implementation; protocol tests do not establish a rendered-browser result.

## Credits

Physical FM-1 panel adapted from Paleotone main 6f027f0, after charlesvestal's X0X and Felucca. Original copyright and GPL notices are retained in index.html and player.js. Clayborn's font license, code license and separate taiko attribution ship with the page.
