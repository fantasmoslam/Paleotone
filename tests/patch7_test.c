/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/patch7.h"
static uint32_t seed = 72913;
static uint32_t next(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }
static int verify(const uint8_t *raw)
{
    uint8_t dst[PATCH7_SIZE + 2], decoded[PATCH7_RAW + 2], oracle[PATCH7_SIZE] = {0};
    uint32_t i, bit;
    memset(dst, 0xA5, sizeof dst); memset(decoded, 0x5A, sizeof decoded);
    /* Independent bit-position oracle fixes the on-disk layout, rather than
     * trusting an encoder and decoder that might share the same mistake. */
    for (i = 0; i < PATCH7_RAW; i++)
        for (bit = 0; bit < 7u; bit++)
            oracle[(i * 7u + bit) / 8u] |= ((raw[i] >> bit) & 1u) << ((i * 7u + bit) % 8u);
    if (!patch7_pack(dst + 1, raw) || memcmp(dst + 1, oracle, PATCH7_SIZE) ||
        dst[0] != 0xA5 || dst[PATCH7_SIZE + 1] != 0xA5) return 0;
    patch7_unpack(decoded + 1, dst + 1);
    return !memcmp(raw, decoded + 1, PATCH7_RAW) && decoded[0] == 0x5A && decoded[PATCH7_RAW + 1] == 0x5A;
}
int main(void)
{
    uint8_t raw[PATCH7_RAW] = {0}, dst[PATCH7_SIZE], before[PATCH7_SIZE];
    uint32_t i, v, cases = 0;
    for (v = 0; v < 128u; v++) {
        memset(raw, v, sizeof raw);
        if (!verify(raw)) return 1;
        cases++;
    }
    for (i = 0; i < PATCH7_RAW; i++)
        for (v = 0; v < 128u; v++) {
            memset(raw, 0, sizeof raw); raw[i] = v;
            if (!verify(raw)) return 2;
            cases++;
        }
    for (v = 0; v < 4096u; v++) {
        for (i = 0; i < PATCH7_RAW; i++) raw[i] = next() & 127u;
        if (!verify(raw)) return 3;
        cases++;
    }
    memset(dst, 0xC3, sizeof dst); memcpy(before, dst, sizeof dst);
    for (i = 0; i < PATCH7_RAW; i++)
        for (v = 128u; v < 256u; v++) {
            memset(raw, 0, sizeof raw); raw[i] = v;
            if (patch7_pack(dst, raw) || memcmp(dst, before, sizeof dst)) return 4;
        }
    /* Every possible packed byte pattern must decode and re-encode exactly. */
    for (v = 0; v < 4096u; v++) {
        for (i = 0; i < PATCH7_SIZE; i++) before[i] = next();
        patch7_unpack(raw, before);
        if (!patch7_pack(dst, raw) || memcmp(dst, before, sizeof dst)) return 5;
    }
    printf("patch7: %u oracle/roundtrip cases; 16384 invalid inputs rejected atomically; 4096 packed-data roundtrips PASS\n", cases);
    return 0;
}
