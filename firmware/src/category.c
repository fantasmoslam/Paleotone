/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Sound categories (1.2, Discussion #90): every sound in the PRESETS list belongs to one, so KNOB 4 (LIST) can show
 * only the basses, the pads, .. (ui.c list_mode). Factory presets: a static table here (a letter per preset, in the
 * engine's preset order); user presets: a byte of their record (upreset.c up_cat, set on the NAME screen when saving
 * or renaming, ui_name.c). Included by ui.c and upreset.c (the host build of the user presets needs the numbers only).
 * The numbers are stored (a user preset's record, the editor's UP_LIST / UP_GET / UP_PUT): append-only. CAT_NONE is a
 * record of before (or the editor's without one): its category follows from its engine (cat_of_engine) */
#ifndef FELUCCA_CATEGORY
#define FELUCCA_CATEGORY 1
enum { CAT_NONE, CAT_BASS, CAT_LEAD, CAT_PAD, CAT_PLUCK, CAT_KEYS, CAT_DRUM, CAT_FX, CAT_OTHER, CAT_N };
static const char *const CAT_NAME[CAT_N] = {"-", "BASS", "LEAD", "PAD", "PLUCK", "KEYS", "DRUM", "FX", "OTHER"};
/* the category a sound without one is listed under: a kit (DRUM, SLICE: engines 10, 13) DRUM, any other OTHER */
static uint32_t cat_of_engine(uint32_t e) { return e == 10u || e == 13u ? CAT_DRUM : CAT_OTHER; }

#ifndef UP_HOST
/* the factory presets' categories: per engine number (engines.c ENGINES[]), a letter per preset in its order (B BASS,
 * L LEAD, P PAD, U PLUCK, K KEYS, D DRUM, F FX, O OTHER); a preset past its string (DIGITAL's, FELUCCA_FM4 builds
 * only; SAMPLE's retired PERC, past SMP_NPRESETS): its engine's (cat_of_engine). tests/ui_test.c checks every string
 * against its engine's preset count. A preset's category is a hint for browsing: nothing else reads it */
static const char *const PRESET_CAT[16] = {   /* Felucca's sounds, then DINOBOX's (tools/ghoulbox_presets.py), ROAR at 14 */
    "LPBPBKLBULLP" "PPLBLBPPP",  /* 0 ANALOG: SAW LEAD, SOFT PAD, SQR BASS, PWM STR, ACID, SINE KEY, RAVE, SUB BASS,
                                  * PLUCK, BRASS, WIND, STRINGS; JUNGLE PAD, EPIC STRINGS, CANYON HORN, STOMP BASS,
                                  * AMBER LEAD, SWAMP BASS, FERN SWELL, STAR SKY, ICE AGE PAD */
    "",                          /* 1 DIGITAL (retired) */
    "LKPLUU" "LUPB",             /* 2 PHASE: BRASS, ORGAN, STRING, RESO, BELL, WIRE; TITAN BRASS, ICE CRYSTAL, RESIN PAD,
                                  * DEEP PULSE */
    "LBLLL" "LPLFUL",            /* 3 LOFI: PULSE LD, WAVE BASS, ARP 8BIT, WAVE LEAD, STEP LEAD; FOSSIL LEAD, DINO CHOIR,
                                  * BONE WHISTLE, PIXEL RAPTOR, EGG ARP, CAVE ECHO */
    "KKLL" "K" "FDUF",           /* 4 SAMPLE: PIANO, (PIANO: the retired TRANH's alias), FLUTE, SAX, (PIANO: the retired
                                  * PERC's alias); JUNGLE NIGHT, ENCOUNTER (a kit), LITHOPHONE, TROPIC STORM */
    "PLBF" "PPFFPPLP",           /* 5 VOICE: CHOIR AAH, VOX LEAD, WOW BASS, WHISPER; TRIBE CHANT, EPIC CHOIR, RAPTOR CALL,
                                  * REX ROAR, DEEP CHANT, SPIRIT VOX, SHAMAN, NIGHT WAIL */
    "BLLUP" "PLPFLPKP",          /* 6 TRIO: FAT BASS, ARP LEAD, SYNC LEAD, RING BELL, CHIP CHOIR; PANGAEA PAD, BRONTO HORN,
                                  * SKY STRINGS, PTERO CRY, LAVA BRASS, SPORE CLOUD, PRIMAL STAB, AURORA PAD */
    "KKKKK" "KKKKKKK",           /* 7 WHEEL: the organs, DINOBOX's too */
    "PFPP" "FPFF",               /* 8 GRAIN: CLOUD PAD, GLITCH, FROZEN, SHIMMER; NIGHT SWARM, STONE MIST, STORM CLOUD,
                                  * AMBER DUST */
    "UUUPUDDPU" "UUUPDUPUU",     /* 9 PHYS: BELL TREE, MARIMBA, PLUCK, BOWED METAL, KALIMBA, HAND DRUM, TOMS, DRONE
                                  * STRING, HARP; LOG MARIMBA, GEODE BELLS, VALLEY HARP, TARPIT DRONE, HIDE DRUM, WATER
                                  * DROPS, BOWED BONE, SITAR VINE, TUSK TINES */
    "D" "DDDD",                  /* 10 DRUM: DRUM KIT; TRIBAL KIT, WAR DRUMS, BONE RATTLE, RITUAL */
    "FFFF" "FFFFFF",             /* 11 NOISE: WIND, RAIN, ARCADE, METAL; VOLCANO .. WIND RIDGE */
    "KUBLPUKU" "BULPFFUBLP",     /* 12 FM6: TINE EP, BELL, FM BASS, BRASS, PAD, MARIMBA, ORGAN, PLUCK; TAR PIT BASS, SLIT
                                  * DRUM, MAMMOTH, GLACIER PAD, METEOR, CICADA BUZZ, AMBER BELLS, TAR GROWL, MAMMOTH
                                  * CALL, GLACIER WIDE */
    "DD",                        /* 13 SLICE: CHOP, STUTTER */
    "LPLLLLLP" "DDDDDDDDDDDD",   /* 14 ROAR: PRIMEVAL, EARTHQUAKE, STAMPEDE, REX GROWL, SWAMP FIDDLE, TRIBE PIPES, WAR
                                  * CHANT, TUNDRA; the rhythms WAR PARADE .. CHAOS DRUMS */
    "FFFFLFFFFPFFFFPB" "FFLF" "FF",   /* 15 SAURUS (DINOBOX): the calls FX, CREST HOOT a lead, DEEP RUMBLE and DISTANT
                                  * HERD pads, LOW GROWL a bass; BABY REX, SEA MONSTER, HADRO HORN (a lead), COMPY PACK;
                                  * PALEOTONE 2's RAPTOR CRY, TITAN BOOM */
};
static uint32_t preset_cat(uint32_t e, uint32_t k)    /* factory preset k of engine e */
{
    static const char L[] = "?BLPUKDFO";               /* (a letter's index is its CAT_*) */
    const char *s = e < NELEM(PRESET_CAT) && PRESET_CAT[e] ? PRESET_CAT[e] : "";
    uint32_t i, c;
    for (i = 0; i < k && s[i]; i++)
        ;
    for (c = 1; s[i] && L[c] && L[c] != s[i]; c++)
        ;
    return s[i] && L[c] ? c : cat_of_engine(e);
}
#endif
#endif
