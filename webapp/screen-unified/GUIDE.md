# PALEOTONE unified test build

This package combines the six-page SCAPE feature, SAURUS 2 and physical dual-screen switching.
Firmware and graphics renderer must be used together.

Source: 1f68c416481860b568e41a85685edcf0cfefa1a7
Firmware SHA256: 26eafca0b34d109d79026d8bffa6a64188fc6c28ce11fd290097662d04d7d07e

Download paleotone-unified.fwsc from this folder. Install using the existing FM-1 installer,
then open this screen page in Chrome/Edge or compatible Android Chrome, connect USB MIDI/SysEx
and select Expanded.

HOME + OCT+ selects the right page. HOME + OCT− selects the left. Release the combo, then edit.
Choices remain for the session; both pages share transport and sound settings.

HOME pages: MIXER, CLOCK, KEYS, SCAPE MUSIC, SCAPE, SCAPE DRONE, SCAPE LIFE,
SCAPE LEVELS, SCAPE LEVELS 2.

SCAPE MUSIC: EVOL / DENS / DRFT / MOOD; a key composes after confirmation over existing patterns.
SCAPE: LEVEL / BIOME / DIST / TIME.
SCAPE DRONE: DRONE / KEY / MOVE / PTCH (PTCH affects creatures).
SCAPE LIFE: TRK / RATE / DANGR / SIZE.
The final two pages set all eight environmental layer levels.

SAURUS 2 includes 14 call types and 22 presets; four throat sets are shared across tracks.

This is an FM-1 package. Do not install it on an SMK-37.
Automated checks do not replace a real-device playing/listening trial.
The previously installed firmware and its matching /webapp/screen-device/ page remain separate.

GPL-3.0-only. Exact source:
https://github.com/fantasmoslam/Paleotone/tree/3770e810ef5fea05900a12e42ab81cc4fc14921e


## Compressed USB display update

The latest paired firmware and companion negotiate lossless compression per state block, alongside four-block USB bursts. Each block falls back to raw bytes when compression would not save space. Refresh this page after installing its matching firmware. Rendering remains the real firmware C renderer; audio and physical HOME + OCT− / OCT+ controls stay on the instrument.
