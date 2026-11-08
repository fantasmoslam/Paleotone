#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# PALEOTONE in the browser: the firmware's sources as the host UI tests build them (web/emu/paleo_web.c), compiled to
# WebAssembly, plus the page.
#   sh web/emu/build.sh   ->  build/emu/{index.html, paleotone.wasm, worklet.js, ..., art/, fonts/}
# Needs clang and wasm-ld with wasi-libc (Debian / Ubuntu: apt install clang lld wasi-libc libclang-rt-dev-wasm32,
# or the versioned libclang-rt-18-dev-wasm32), and the generated headers (build/gen: ./build.sh, or
# python3 -c 'import sys; sys.path.insert(0, "tools"); import build; build.generate()').
# -ffp-contract=off: no fused multiply-adds, as the host build.
set -e
cd "$(dirname "$0")/../.."
[ -f build/gen/felucca_tables.h ] || { echo "emu: no build/gen (run ./build.sh, or tools/build.py's generate(), once)"; exit 1; }
CC="${EMU_CC:-clang}"
VERSION=$(sed -n 's/^#define FELUCCA_VERSION "PALEOTONE \([^"]*\)".*/\1/p' firmware/src/ui.c)
mkdir -p build/emu/art build/emu/fonts build/emu/vendor/libflac build/emu/vendor/mp3 build/emu/licenses
# shellcheck disable=SC2086
"$CC" --target=wasm32-wasi -mexec-model=reactor ${EMU_OPT:--O2} -ffp-contract=off -std=gnu99 -w ${EMU_CFLAGS:-} \
    -Iweb/emu/wasi -Ibuild/gen -Ifirmware/src -Itests \
    -Wl,--export-dynamic -Wl,-z,stack-size=1048576 -Wl,--initial-memory=33554432 \
    -o build/emu/paleotone.wasm web/emu/paleo_web.c
sed -e "s/@VERSION@/$VERSION/g" web/emu/index.html > build/emu/index.html
cp web/emu/worklet.js web/emu/wasi_stubs.js web/emu/emu_io.js web/emu/audio_export.js web/emu/audio_encode.js \
   web/emu/audio_encode_worker.js web/emu/audio_session.js build/emu/
cp web/emu/vendor/libflac/libflac.min.wasm.js web/emu/vendor/libflac/libflac.min.wasm.wasm build/emu/vendor/libflac/
cp web/emu/vendor/mp3/WasmMediaEncoder.min.js web/emu/vendor/mp3/mp3.wasm build/emu/vendor/mp3/
cp web/emu/vendor/README.md build/emu/vendor/
for f in MIT-libflac.js.txt BSD-3-Clause-libFLAC.txt MIT-wasm-media-encoders.txt LGPL-2.0-LAME.txt LAME-NOTICE.txt; do
    [ -f "LICENSES/$f" ] && cp "LICENSES/$f" build/emu/licenses/
done
cp web/site/art/skull.png web/site/art/wordmark.png build/emu/art/
cp assets/fonts/Clayborn.ttf assets/fonts/Clayborn-LICENSE.txt build/emu/fonts/
echo "emu: build/emu ($(wc -c < build/emu/paleotone.wasm | tr -d ' ') B wasm, PALEOTONE $VERSION)"
