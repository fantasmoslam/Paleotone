/* SPDX-License-Identifier: GPL-3.0-only */
/* Lossless storage codec for 128 seven-bit FM patch bytes. Not the MIDI SysEx
 * encoding. Distinct input/output buffers; caller validates the enclosing
 * project's size, version and checksum before decoding. */
#ifndef DINOBOX_PATCH7_H
#define DINOBOX_PATCH7_H
#define PATCH7_RAW 128u
#define PATCH7_SIZE 112u
static int patch7_pack(uint8_t *out, const uint8_t *in)
{
    uint32_t i, acc = 0, bits = 0, pos = 0;
    for (i = 0; i < PATCH7_RAW; i++)
        if (in[i] > 127u)
            return 0; /* invalid patch: leave destination untouched */
    for (i = 0; i < PATCH7_RAW; i++) {
        acc |= (uint32_t)in[i] << bits;
        bits += 7u;
        if (bits >= 8u) {
            out[pos++] = (uint8_t)acc;
            acc >>= 8;
            bits -= 8u;
        }
    }
    return 1;
}
static void patch7_unpack(uint8_t *out, const uint8_t *in)
{
    uint32_t i, acc = 0, bits = 0, pos = 0;
    for (i = 0; i < PATCH7_RAW; i++) {
        if (bits < 7u) {
            acc |= (uint32_t)in[pos++] << bits;
            bits += 8u;
        }
        out[i] = (uint8_t)(acc & 127u);
        acc >>= 7;
        bits -= 7u;
    }
}
#endif
