/* SPDX-License-Identifier: GPL-3.0-only */
/* T-WRECKS: Paleotone's sounds as voices for the layers (pt.c). Each layer that has one is a Paleotone part: its
 * sound one of its list (PT_N[layer]), its notes the layer's, its output read as floats (mono, about +-1 at a full
 * voice). Everything here runs where amb.c's render runs (the audio interrupt), after pt_init. */
#pragma once
#include <stdint.h>

enum { PT_DRONE, PT_SEQ, PT_LEAD, PT_SEQ2, PT_NLAYER };
enum { PT_CALLS = PT_NLAYER, PT_NPART };
#define PT_NCALL 8
/* the sounds each layer offers (pt.c PT_LIST's); a layer's voice knob has them after its own voices */
#define PT_NDRONE 30
#define PT_NSEQ 26
#define PT_NLEAD 22
#define PT_NSEQ2 38
extern const uint8_t PT_N[PT_NPART];

void pt_init(void);
void pt_clean_organ(int on); /* clean Temple support in the approved taiko scene */
const char *pt_name(int layer, int k);          /* its name as the screen shows it (9 characters at most) */
int pt_mono(int layer, int k);                  /* 1: a mono sound (one note at a time) */
void pt_sound(int layer, int k);                /* the layer plays sound k of its list; k < 0: none (its own voice) */
int pt_is(int layer);                           /* the sound the layer plays + 1, 0: none */
/* a note (MIDI 0..127, velocity 1..127); gate: samples until it lets go by itself (0: until pt_note_off) */
void pt_note_on(int layer, int note, int vel, uint32_t gate);
void pt_note_off(int layer, int note);
void pt_notes_off(int layer);                   /* everything it holds let go (its tails ring on) */
void pt_quiet(int layer);                       /* everything it plays, now (a voice change, a panic) */
/* the next n samples of the layer into out (written, not added); 0: nothing sounds (out left as it was) */
int pt_render(int layer, float *out, uint32_t n);
int pt_voices(void);                            /* Paleotone's voices sounding now, every layer's (of 8) */

/* Original TRIBE kit's BOOM, SKIN and LOG models, independent of melodic voices. */
void pt_skin_hit(int lane, int velocity, int tone, int decay);
int pt_skin_render(float *out, uint32_t n);
void pt_skin_quiet(void);
