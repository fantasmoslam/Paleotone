#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/nightclaw-pc
x86_64-w64-mingw32-gcc -O2 -ffp-contract=off -std=c99 -w -DEN_HOST -DEN_DESKTOP -c firmware/src/pt/pt.c -o build/nightclaw-pc/pt.o
x86_64-w64-mingw32-gcc -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation -DEN_HOST -DEN_DESKTOP -shared -static-libgcc -o build/nightclaw-pc/nightclaw-engine.dll desktop/pc_engine.c desktop/pc_audio.c firmware/src/dsp/amb.c build/nightclaw-pc/pt.o -lole32 -luuid -lm
