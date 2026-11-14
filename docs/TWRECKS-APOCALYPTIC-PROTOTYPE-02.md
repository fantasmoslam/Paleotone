# Apocalyptic prototype 02: clearer accompaniment

[Play prototype 02](https://chatgpt.com/library?file_id=libfile_108c161448e4819187acd346aacb2620&account_id=personal).

Dylan accepts prototype 01's overall vibe as a start, but hears excessive distortion, blended voices, mud/sludge, too much melody and insufficient beat. The revised target retains the ominous mood while making a backing track a player can lead over.

Changes:

- A recurring three-strike pulse replaces the sparse impacts. This is three quarter-note strikes followed by a rest, not a musical triplet. The quieter middle retains a slower pulse so the player can keep time. There are 50 drum events over the score.
- Existing LOG/tom-model drums are tuned down to nominal 110 Hz and approximately 147 Hz, with clear attacks and shorter decays. They are a synthesized, taiko-inspired direction, not sampled acoustic taiko.
- The lead phrase is omitted completely. Two-note supporting fifths and six sustained low-bass entries leave the melodic lead role to the player.
- Clean Sub Bass replaces Fat Bass. Organ drive, click and rotary are disabled, sub drawbars removed and the supporting voice softened.
- Separate bass/body/drum frequency ranges and a much shorter, quieter room reduce overlap. Bass remains dry; the backing pad briefly yields at drum attacks. No mix saturation or limiter is used.

The 77.111-second score stays at 54 BPM for a useful comparison. The backing reduces around 0:36 and returns around 0:44. Peak is -2.00 dBFS and stereo RMS -17.52 dBFS, close to version 01's -17.45 dBFS RMS. These checks do not establish perceived loudness equality or subjective musical success. WAV and decoded MP3 remain finite and unclipped. Peak melodic occupancy is three of eight voices.

This uses the existing Paleotone engines and compact drum DSP, with desktop-only direct patch edits, tuned drum configuration and offline mixing. It demonstrates a candidate sound/arrangement, not a completed FM-1 port or hardware CPU result. The installed firmware is unchanged. Version 01 is retained for comparison.

Reproduce by compiling tools/research/apocalyptic_prototype_02.c as a standalone host translation unit with -O2 -ffp-contract=off -std=c99 -w -I. -lm. It includes pt.c to configure the existing patch and drum internals; do not also link pt.o. Pass the stems.wav output path and an events.csv path. Run tools/research/mix_apocalyptic_prototype_02.py OUTPUT_DIRECTORY to create the MP3 and WAV; dependencies are the same as version 01.
