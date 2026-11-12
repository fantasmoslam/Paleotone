# NightClaw for the M-VAVE FM-1

![NightClaw — a raptor within a moonlit prehistoric jungle](docs/assets/nightclaw-jungle.png)

A generative dinosaur synth with resonant taiko drums, western/tribal scenes, selectable instruments and playable creature calls. RAPTOR NIGHT and six western scenes are the approved musical reference. The guarded two-core audio candidate plays the previously overloading Lead test cleanly on the tested FM-1.

**[Quick start](docs/TWRECKS-QUICKSTART.md)** · **[Current features](TWRECKS.md)** · **[Completion checks](docs/TWRECKS-COMPLETION-CHECKLIST.md)** · **[Development PR](https://github.com/fantasmoslam/Paleotone/pull/41)**

## Play in your browser

The [NightClaw browser player](https://fantasmoslam.github.io/Paleotone/nightclaw/) is a clickable physical FM-1 panel, following Paleotone’s emulator: knobs, buttons, touch keys and the live firmware screen. Its [separate visual guide](https://fantasmoslam.github.io/Paleotone/nightclaw/guide.html) covers the instrument in eight illustrated chapters. See [browser build and validation](docs/NIGHTCLAW-BROWSER.md). The project page is published on GitHub Pages.

## Play

Turn PRESETS to choose a scene. On Home, SELECT chooses a track; ALGORITHM chooses its instrument, or the beat when Skin is selected. The four right encoders edit that row. Play the Lead on the white keys, or select Calls for manual creatures.

The twenty-scene bank includes BONE RIDER, DUST TRAIL, RED MESA, FOSSIL PASS, HIGH FRONTIER and NIGHT RIDER. Fourteen drum patterns and the melodic voices are independently selectable and saved per scene. Cello, Cello Low, Sub Pad and Fat Pad are the last four instruments on each melodic track.

**Saving:** hold GLO for about half a second and release. A short GLO tap opens Setup; holding about 1.5 seconds resets the current scene. The physical SAVE button opens sound pages.

## Candidate installation and build

Installed source **f9b11d6** passed [full CI](https://github.com/fantasmoslam/Paleotone/actions/runs/43139217653). The [published candidate](https://github.com/fantasmoslam/Paleotone/blob/1086a04d6dbda6ca95f6b8cc07ec67a75ca310f2/nightclaw-FM-1_942.fwsc) matches the installed package by SHA-256; its fixed output commit and checksum are recorded in the [completion checklist](docs/TWRECKS-COMPLETION-CHECKLIST.md). Product identity remains FM-1_942.

The [physical-detent correction](docs/NIGHTCLAW-DETENT-FIX.md) is installed and user-approved: one click is one item or parameter step, with no velocity acceleration. Home names use 24–32 px cuts and retain 36 px gauges. Every generated font role uses the NightClaw family, without the original-font fallback. The approved palette is bone white, saturated crimson, sulfur yellow and electric violet on near-black.

The static jungle title is visible for at least three seconds. Twenty approved audio baselines and the guarded two-core worker are retained. The [completion plan](docs/TWRECKS-NEXT-PLAN.md) separates completed checks from longer physical-device sessions and broader readability feedback still needed.

Install nightclaw-FM-1_942.fwsc using tools/fm1_install.py, the project's USB-MIDI installer. A reinstall is unnecessary if the current candidate is already installed. [BUILDING.md](BUILDING.md) describes the compiler and SDK setup.

## Source and credits

NightClaw builds on NACHTFAHRT/Felucca and Paleotone's synthesis work. The Paleotone/X0X/Melodee-derived guarded worker and original copyright notices remain in their source files. Firmware code follows the repository's GPL notices; the recorded taiko assets have separate [CC BY-SA 4.0 attribution](tools/research/taiko-bank-05/ATTRIBUTION.md).

The [upstream README](UPSTREAM-NACHTFAHRT-README.md) is retained as a historical reference; its installer, website and musical description describe NACHTFAHRT.
