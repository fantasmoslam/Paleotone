/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS: Paleotone's GRAIN engine reads the sample sets (eng_sample.c: not built here). Engine 8 keeps its number
 * and plays nothing; no sound in pt.c's lists uses it. */
typedef struct { uint8_t unused; } gr_part_t;
static const engine_t ENG_GRAIN = {
    "GRAIN", {"-", "-"}, {{"-", F_INT, 0, 0, 0, 0, 0}}, SILENT_PRESETS, 1, -1, silent_note_on, silent_render,
    0x87F0, {P_E0, P_E1, P_ATK, P_REL},
};
