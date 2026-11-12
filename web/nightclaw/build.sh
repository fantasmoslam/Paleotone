#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Full NightClaw firmware in WebAssembly; no device HAL or browser imitation DSP.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/nightclaw/assets
python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
python3 tools/gen_words.py firmware/words.tsv web/words build/gen/ann_words.h >/dev/null
emcc -O2 -ffp-contract=off -std=gnu99 -Wall -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
    -DEN_HOST -DEN_WEB '-DEN_VERSION="WEB"' -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    --no-entry -sSTANDALONE_WASM -sTOTAL_STACK=1048576 -sINITIAL_MEMORY=33554432 -sFILESYSTEM=0 \
    -o build/nightclaw/nightclaw.wasm web/nightclaw/nightclaw_web.c firmware/src/dsp/amb.c firmware/src/pt/pt.c
cp web/nightclaw/index.html web/nightclaw/panel.css web/nightclaw/style.css web/nightclaw/player.js web/nightclaw/worklet.js web/nightclaw/guide.html web/nightclaw/guide.js build/nightclaw/
cp docs/assets/nightclaw-jungle.png build/nightclaw/assets/jungle.png
cp docs/assets/nightclaw-detents/home.png build/nightclaw/assets/home.png
cp docs/assets/nightclaw-typeface/edit.png build/nightclaw/assets/edit.png
cp docs/assets/nightclaw-typeface/calls.png build/nightclaw/assets/calls.png
cp assets/fonts/Clayborn.ttf assets/fonts/Clayborn-LICENSE.txt assets/fonts/BarlowSemiCondensed-Medium.ttf assets/fonts/BarlowSemiCondensed-Bold.ttf assets/fonts/Barlow-OFL.txt build/nightclaw/assets/
cp LICENSE build/nightclaw/LICENSE.txt
cp tools/research/taiko-bank-05/ATTRIBUTION.md build/nightclaw/TAIKO-ATTRIBUTION.txt
echo "NightClaw browser: build/nightclaw"
