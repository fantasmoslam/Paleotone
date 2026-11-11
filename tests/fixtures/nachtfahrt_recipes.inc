/* SPDX-License-Identifier: GPL-3.0-only */
/* Legacy regression fixtures only: never compiled into FM-1 firmware. */
static const int16_t PC_TIMEWIND[] = {        /* Schulze, Bayreuth Return: an organ through the phaser; Sequence 2
                                                * at its default 60 and the reverb at 50, the lengths 16/15 (the user's
                                                * ear) */
    P_TEMPO, 96, P_ROOT, 2, P_MODE, MODE_AEOLIAN,
    P_SVOICE, 0, P_SEQLEN, 16, P_SEQ2LEN, 15, P_SEQGRID, 1, P_DENSITY, 65, P_REVERB, 50,
    P_TAPB, 50, P_TAPBT, 2,
    P_CVOICE, 0, P_PHASER, 60, P_DRIFT, 3, P_PMODE, EN_WALK_HOME,
    P_LVOICE, 0, P_LDIN, 32,
    P_NCHAR, 1, P_CRACKLE, 7,
    P_MW0, 30, P_MR0, 25, P_MW2, 15, P_MW4, 30, P_BRIDGE, 45,
    P_MI1, 32, P_ARC, 25, P_MOTION, 45, P_LMEM, 55, -1};
static const int16_t PC_WAHNFRIED[] = {       /* Schulze, Wahnfried 1883: roots walking with no home, strings, the
                                                * Sequence a faint heartbeat under them (the user's ear) */
    P_TEMPO, 80, P_ROOT, 10, P_MODE, MODE_DORIAN,
    P_SEQ, 30, P_MI1, 16, P_SEQ2, 50, P_SEQ2OCT, -1,
    P_CHORD, 95, P_CVOICE, 1, P_PEDAL, 85, P_PHASER, 50, P_CRES, -5, P_DRIFT, 2, P_PMODE, EN_WALK_FREE,
    P_LEAD, 54, P_LVOICE, 3, P_LTONE, 75, P_VIB, 55, P_LDENS, 60, P_LDIN, 8,
    P_FEEDBACK, 75, P_REVERB, 45,
    P_MW0, 45, P_MT0, 60, P_MR0, 45, P_MW2, 15, P_BRIDGE, 30,
    P_ARC, 20, P_MOTION, 55, P_LMEM, 40, -1};
static const int16_t PC_SAKURA[] = {         /* (2026-10-30) the ninth piece, the user's: Wahnfried's Lead melody, tempo,
                                                * key and Strings, the Lead a Theremin two octaves down, gliding ("almost
                                                * like singing"); Jean-Michel Jarre's arps (the Sequence on 2600 an octave
                                                * down, dark; Sequence 2 on Sub two down, open); the Drone darker (Colour
                                                * -30) and quieter (75), on the royal road. First Umeda, then Ryokuu (緑雨,
                                                * "green rain") with Rain outside; the FM-1's CPU went over 75% and stopped the
                                                * rain, so, the user (2026-10-31): "it's not point chasing effect if it takes
                                                * too much cpu": Sakura Rain, no Atmosphere */
    P_TEMPO, 80, P_ROOT, 10, P_MODE, MODE_DORIAN,
    P_SEQ, 30, P_MI1, 16, P_SVOICE, 0, P_SEQOCT, -1, P_CUTOFF, 30,
    P_SEQ2, 50, P_S2VOICE, 5, P_SEQ2OCT, -2, P_S2CUT, 90,
    P_CHORD, 75, P_CVOICE, 1, P_CCOL, -30, P_PEDAL, 85, P_PHASER, 50, P_CRES, -5, P_DRIFT, 2,
    /* (the user: "royal road progression, keep Bb Dorian"; research_notes/japanese-chords, progression A) A flat's
     * royal road in B flat Dorian's notes, IV maj7 - V7 - iii7 - vi7, again, then D flat over E flat and home to B flat
     * minor 11: the eight slots written, looping, a chord every Drift (8 bars) */
    P_PMODE, EN_WALK_FREE | EN_PLOOP,
    P_CR0, 1, P_CQ0, EN_CQ_MAJOR, P_CX0, EN_CX_MAJ7, P_CR1, 3, P_CQ1, EN_CQ_MAJOR, P_CX1, EN_CX_7,
    P_CR2, 0, P_CQ2, EN_CQ_MINOR, P_CX2, EN_CX_7, P_CR3, 5, P_CQ3, EN_CQ_MINOR, P_CX3, EN_CX_7,
    P_CR4, 1, P_CQ4, EN_CQ_MAJOR, P_CX4, EN_CX_MAJ7, P_CR5, 3, P_CQ5, EN_CQ_SUS4, P_CX5, EN_CX_9,
    P_CR6, 0, P_CQ6, EN_CQ_MINOR, P_CX6, EN_CX_11, P_CR7, 10, P_CQ7, EN_CQ_MINOR, P_CX7, EN_CX_11,
    P_LEAD, 60, P_LVOICE, 3, P_LOCT, -2, P_GLIDE, 70, P_LTONE, 75, P_VIB, 55, P_LDENS, 60, P_LDIN, 2,   /* (the user: the
                                                  * Lead too quiet for the speaker, 60; in sooner, 2 bars) */
    P_FEEDBACK, 75, P_REVERB, 45,
    P_MW0, 45, P_MT0, 60, P_MR0, 45, P_MW2, 15, P_BRIDGE, 30,
    P_ARC, 20, P_MOTION, 55, P_LMEM, 40, -1};
static const int16_t PC_PHAEDRA[] = {         /* Tangerine Dream, Phaedra: a Moog bass line, Mellotron flutes and choir */
    P_TEMPO, 149, P_ROOT, 2, P_MODE, MODE_AEOLIAN,
    P_SVOICE, 1, P_SEQOCT, -1, P_SEQLEN, 16, P_DENSITY, 70, P_SWEEP, 85,
    P_SEQ2, 40, P_S2VOICE, 1, P_SEQ2OCT, 0, P_SEQ2LEN, 15, P_MI3, 0,
    P_CHORD, 70, P_CVOICE, 5, P_DRIFT, 4,         /* (the user's ear: the Drone a formant Choir, at 70) */
    P_LVOICE, 2, P_LDIN, 24,
    P_NCHAR, 5, P_CRACKLE, 10,
    P_MW0, 25, P_MW3, 30, P_MR3, 40, P_MW4, 25,
    P_MI1, 16, P_ARC, 30, P_MOTION, 55, P_LMEM, 75, -1};
static const int16_t PC_RUBYCON[] = {         /* Tangerine Dream, Rubycon: dark, two sequences out of the drone (the
                                                * octaves and voices the user's ear: the Sequence -2 on the 2600, Sequence 2 0,
                                                * the Lead the Synthi) */
    P_TEMPO, 144, P_ROOT, 5, P_MODE, MODE_AEOLIAN,
    P_SVOICE, 0, P_SEQOCT, -2, P_SEQLEN, 8, P_CUTOFF, 32, P_SWEEP, 60,
    P_SEQ2, 55, P_S2VOICE, 5, P_SEQ2OCT, 0, P_SEQ2LEN, 7, P_MI3, 32,
    P_TAPB, 40, P_TAPBT, 4,
    P_CHORD, 65, P_CVOICE, 0, P_CCOL, -20, P_PEDAL, 85, P_DRIFT, 4,
    P_LVOICE, 4, P_LDIN, 32,
    P_MW0, 35, P_MR0, 25, P_MW3, 30, P_BRIDGE, 50,
    P_MI1, 16, P_ARC, 40, P_MOTION, 65, P_LMEM, 45, -1};
static const int16_t PC_STRATOSFEAR[] = {     /* Tangerine Dream, Stratosfear: brighter, driving, the Lead forward */
    P_TEMPO, 138, P_ROOT, 4, P_MODE, MODE_AEOLIAN,
    P_SVOICE, 3, P_SEQLEN, 16, P_DENSITY, 70, P_CUTOFF, 55,
    P_SEQ2, 60, P_S2VOICE, 0, P_SEQ2LEN, 15, P_MI3, 0,
    P_CVOICE, 1, P_DRIFT, 3,
    P_LEAD, 52, P_LVOICE, 1, P_LDENS, 65, P_LDIN, 12,
    P_MR1, 40, P_MW3, 30, P_MR3, 40, P_BRIDGE, 35,
    P_ARC, 20, P_MOTION, 50, P_LMEM, 85, -1};
static const int16_t PC_SUNRAIN[] = {         /* Ashra, New Age of Earth: major, the strongest repeats, bright strings;
                                                * the lengths 8/7 (the user: "works best") */
    P_TEMPO, 130, P_ROOT, 2, P_MODE, MODE_IONIAN,
    P_SVOICE, 3, P_SEQLEN, 8, P_CUTOFF, 55,
    P_SEQ2, 50, P_S2VOICE, 4, P_SEQ2OCT, 0, P_SEQ2LEN, 7, P_MI3, 0,
    P_TAPA, 100, P_TAPB, 100, P_TAPBT, 3, P_ECHO, 65, P_FEEDBACK, 65,
    P_CVOICE, 1, P_DRIFT, 3,
    P_LVOICE, 0, P_LDIN, 16,
    P_NCHAR, 2, P_CRACKLE, 5,
    P_MW3, 25, P_MW4, 25, P_BRIDGE, 35,
    P_ARC, 25, P_MOTION, 45, P_LMEM, 60, -1};
static const int16_t PC_MIRAGE[] = {          /* Schulze, Mirage: icy, a glass sequence, sparse, a long reverb; the
                                                * lengths 12/11 (the user's) */
    P_TEMPO, 95, P_ROOT, 6, P_MODE, MODE_AEOLIAN,
    P_SVOICE, 4, P_SEQLEN, 12, P_SEQGRID, 1, P_DENSITY, 40, P_SWEEP, 30,
    P_SEQ2, 45, P_S2VOICE, 4, P_SEQ2OCT, 0, P_SEQ2LEN, 11, P_MI3, 24,
    P_TAPB, 60, P_TAPBT, 4,
    P_CVOICE, 4, P_PHASER, 85, P_DRIFT, 4,
    P_LVOICE, 3, P_LDIN, 24,
    P_REVERB, 55, P_SIZE, 90,
    P_NCHAR, 3, P_CRACKLE, 10,
    P_MW0, 25, P_MR0, 20, P_MW4, 35, P_MR4, 20,
    P_MI1, 16, P_ARC, 20, P_MOTION, 40, P_LMEM, 50, -1};
static const int16_t PC_CYBORG[] = {          /* Schulze, Cyborg: early, raw, deep organ drones beating; a quiet
                                                * Sequence (20, the user's: at its defaults), no Sequence 2; the Lead
                                                * up front (70, the user's) */
    P_TEMPO, 70, P_ROOT, 0, P_MODE, MODE_PHRYGIAN,
    P_SEQ, 20, P_SEQ2, 0,
    P_CHORD, 70, P_CVOICE, 0, P_CCOL, 25, P_PEDAL, 90, P_PHASER, 20, P_DRIFT, 4, P_PMODE, EN_WALK_WANDER,
    P_LEAD, 70, P_LVOICE, 3, P_LDENS, 30, P_LOCT, -1, P_LDIN, 8,
    P_MW0, 50, P_MT0, 70, P_MR0, 45, P_MW2, 10, P_BRIDGE, 30,
    P_ARC, 15, P_MOTION, 35, P_LMEM, 35, -1};
static const int16_t PC_UMEDA[] = {           /* (2026-10-31) the tenth piece, the user's: Kankyo Ongaku, the piano in the
                                                * plazas of Umeda; F sharp major pentatonic at 50 (a sixteenth 0.3 s: the
                                                * motive's own grid), a quiet Organ pad on the home chord, the Piano on
                                                * both sequences (sparse, low) and the Lead, the Lead's theme the user's
                                                * second motive (ld_seed); no Atmosphere (the user: "I prefer the Umeda
                                                * with None atmosphere": the Rain, then, took about 8 points of
                                                * the FM-1's load, 72 to 80) */
    P_TEMPO, 50, P_ROOT, 6, P_MODE, MODE_PENT_MAJ,
    P_SEQ, 25, P_SVOICE, 16, P_DENSITY, 20, P_SEQOCT, -1, P_MI1, 8,   /* (the user, in the browser: the Piano on both
                                                  * sequences; Sequence: Volume 25, Density 20, octave -1, Intro 8) */
    P_SEQ2, 30, P_S2VOICE, 17, P_SCH1, 65, P_SEQ2OCT, -2, P_S2DEC, 70,   /* (Sequence 2: Volume 30, Chance 65, octave -2,
                                                  * Decay 70) */
    P_CHORD, 45, P_CVOICE, 0, P_PEDAL, 85, P_DRIFT, 1, P_PMODE, EN_WALK_HOME,   /* (the user: "the drone sounds better with
                                                  * Organ"; Drift 4 bars; by ear in the browser, "the current good volumes":
                                                  * Drone 45) */
    P_LEAD, 70, P_LVOICE, 9, P_GLIDE, 0, P_LDENS, 50, P_LDIN, 2,   /* (the user: no glide, a piano's notes struck) */
    P_REVERB, 60,
    P_ARC, 15, P_MOTION, 30, P_LMEM, 60, -1};
static const struct {
    const char *name;
    const int16_t *set;
} PIECES[EN_NPIECES] = {
    {"Timewind", PC_TIMEWIND}, {"Wahnfried", PC_WAHNFRIED}, {"Phaedra", PC_PHAEDRA},   {"Rubycon", PC_RUBYCON},
    {"Stratosfear", PC_STRATOSFEAR}, {"Sunrain", PC_SUNRAIN}, {"Mirage", PC_MIRAGE}, {"Cyborg", PC_CYBORG},
    {"Sakura Rain", PC_SAKURA}, {"Umeda", PC_UMEDA},
};
