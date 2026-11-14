/* SPDX-License-Identifier: GPL-3.0-only */
/* Beat-only extraction from Paleotone/GHOULBOX eng_gurdy.c, source blob 8955075588e9494aeb26569e1ccef63dae3ef7be.
 * Original GHOULBOX/DINOBOX work on Felucca by Leo Kuroshita, Hugelton Instruments.
 * No oscillator, drone, rosin or Gurdy audio is included. Step phase belongs to the transport.
 * For the first eleven entries, seed zero reproduces the original event logic.
 * Three T-WRECKS variations are appended. Other seeds vary ghosts/CHAOS, not the core grid. */
#pragma once
#include <stdint.h>
#define TAIKO_RHYTHM 8u
#define TAIKO_CHAOS 18u
#define TAIKO_N_IMPORTED 11u
#define TAIKO_N_RHYTHMS 14u
enum { TAIKO_GALOP, TAIKO_TRES, TAIKO_WAR, TAIKO_RAIN, TAIKO_QUAKE, TAIKO_WAR2,
       TAIKO_TALK, TAIKO_SEVEN, TAIKO_PENTA, TAIKO_SPIRAL, TAIKO_CHAOTIC,
       TAIKO_TRAIL, TAIKO_OUTRIDER, TAIKO_HUNT };
static const struct { uint8_t len, v[32]; } TAIKO_RHY[] = {
    {16, {2, 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1}},                   /* GALOP: a herd running, da-da-DUM */
    {16, {2, 0, 0, 1, 0, 0, 1, 0, 2, 0, 0, 1, 0, 0, 1, 0}},                   /* TRES: 3-3-2 */
    {16, {2, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 0, 1, 1}},                   /* WAR: a march, a boom on the halves */
    {16, {2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1}},                   /* RAIN: hands never stopping */
    {16, {2, 0, 1, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0}},                   /* QUAKE: a slow heartbeat */
    {32, {2, 0, 0, 2, 0, 0, 3, 0, 2, 0, 1, 1, 3, 0, 0, 0,                     /* WAR2: war drums, two bars, low and */
          2, 0, 0, 2, 0, 0, 3, 0, 2, 0, 3, 0, 2, 3, 2, 3}},                   /* high answering */
    {16, {4, 0, 1, 4, 0, 3, 0, 1, 4, 0, 0, 4, 1, 0, 3, 1}},                   /* TALK: talking drums */
    {14, {2, 0, 1, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0}},                         /* SEVEN: 7/8, 2+2+3 */
    {20, {2, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0}},       /* PENTA: five hits over four beats .. */
    {32, {2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 3,   /* SPIRL: hits closing in, a roll into */
          0, 1, 0, 3, 1, 3, 1, 3, 4, 4}},                                     /* the downbeat */
    {16, {2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0}},                   /* CHAOS: the downbeats, the rest by chance */
    /* T-WRECKS originals: independent western/tribal beat variations.
     * The original eleven entries and their event logic remain unchanged. */
    {32, {2, 0, 1, 1, 2, 0, 1, 0, 2, 0, 1, 1, 3, 0, 1, 0,
          2, 0, 1, 1, 2, 0, 1, 0, 2, 0, 1, 1, 3, 1, 2, 0}},               /* TRAIL: gallop, answering low/mid accents */
    {32, {2, 0, 1, 0, 0, 1, 2, 0, 3, 0, 1, 1, 2, 0, 1, 0,
          2, 0, 1, 0, 0, 1, 2, 1, 3, 0, 1, 0, 2, 1, 3, 1}},               /* OUTRIDER: displaced accents over a continuing pulse */
    {32, {2, 0, 1, 1, 3, 0, 2, 0, 2, 1, 0, 1, 3, 0, 1, 1,
          2, 0, 1, 1, 3, 1, 2, 0, 2, 0, 1, 2, 3, 1, 2, 1}},               /* HUNT: denser low ensemble and a turnaround */
};
static inline uint32_t taiko_rhythm_hash(uint32_t x)
{
    x = (x + 0x9E3779B9u) * 0x85EBCA6Bu;
    x ^= x >> 13;
    x *= 0xC2B2AE35u;
    return x ^ (x >> 16);
}
/* rhythm r's hit on step st (from the note-on): 0 none, else its kind (1 .. 4) */
static inline uint32_t taiko_rhythm_hit(uint32_t r, uint32_t st, int32_t wobl, uint32_t seed)
{
    if (r >= TAIKO_N_RHYTHMS) return 0;
    wobl = wobl < 0 ? 0 : wobl > 127 ? 127 : wobl;
    uint32_t len = TAIKO_RHY[r].len, pos = st % len, v = TAIKO_RHY[r].v[pos], h = taiko_rhythm_hash(st ^ seed);
    if (r + TAIKO_RHYTHM == TAIKO_CHAOS && !v && (h >> 28) < 7u)
        v = (h >> 28) < 3u ? 1u : (h >> 28) < 5u ? 3u : (h >> 28) < 6u ? 4u : 2u;
    if (((st / len) & 3u) == 3u && pos + 4u >= len)     /* the fill: the bar's last four steps a roll */
        v = pos + 1u == len ? 2u : (pos & 1u) ? 3u : 1u;
    if (!v && (int32_t)(h >> 25) < wobl / 3)            /* a ghost note */
        v = 1;
    return v;
}
