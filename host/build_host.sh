#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the AMB-1 host simulator (the whole app on this machine) into build/host/amb_host.
# Same sources as the firmware, -ffp-contract=off like the device (no fused multiply-add).
set -e
cd "$(dirname "$0")/.."
CC="${CC:-cc} -D_GNU_SOURCE"          # (Linux hides clock_gettime and friends under -std=c99)
HOSTFLAGS=""
HOSTOUT=build/host/amb_host
if [ "${EN_LEGACY_TEST_BANK:-0}" = 1 ]; then
    HOSTFLAGS=-DEN_LEGACY_TEST_BANK
    HOSTOUT=build/host/amb_host_legacy
fi
mkdir -p build/gen build/host
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
python3 tools/gen_words.py firmware/words.tsv web/words build/gen/ann_words.h >/dev/null
# (T-WRECKS) Paleotone's sounds (firmware/src/pt/pt.c): Paleotone's code, its own unit, its own warnings
$CC -O2 -ffp-contract=off -std=c99 -w -DEN_HOST -c -o build/host/pt.o firmware/src/pt/pt.c
$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DEN_HOST $HOSTFLAGS -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o "$HOSTOUT" host/amb_host.c \
    firmware/src/dsp/amb.c build/host/pt.o -lm
echo "host: $HOSTOUT"
