# Prototype 05: recorded taiko ensemble

[Play prototype 05](https://chatgpt.com/library?file_id=libfile_5b7d08364a44819198bd49833be8c453&account_id=personal). The opening 3.2 seconds expose the drum sound before the sustained backing enters.

Dylan rejects the small log/tom drum character and sparse literal three-hit rhythm. The target is grandiose, resonant percussion with fuller, more interesting beats that carry under a live lead. Earlier comments that the overall vibe was improving do not constitute approval of those drum sounds or of a mandatory three-hit pattern.

This pass replaces all previous percussion with recorded taiko samples from SCC/Floe/Subaqueous. Four samples are trimmed, faded, converted to mono 16 kHz IMA ADPCM and decoded from that compact bank for this audition. Encoded total is 40,312 bytes, including WAV headers. This is not a claim that the recordings are synthesized by the current firmware.

The score uses four related rhythmic figures, alternating low and responding drums, layered ensemble accents and phrase-end fills. It has 137 note events including ensemble layers, compared with 62 total drum events in version 04. Dynamics and round-robin samples vary the hits. Playback is capped at six concurrent drum voices; this render peaks at five with no tail steals. The melody lead role remains empty. Native bass and organ stems supply the sustained bed.

The old log/hide textures are removed. Natural attack detail is retained so the percussion can carry through a lead; no cymbals, hats or bright pitched percussion are added. A damped room supports scale while bass remains dry. Desktop peak control uses 5 ms lookahead, 100 ms release and a -1.5 dBFS ceiling. The final RMS is -17.40 dBFS, close to version 04’s -17.48. These measurements do not establish perceived loudness equality or musical acceptance. WAV/decoded MP3 are finite and unclipped, and duration is 77.111 seconds.

## Source and credits

Taiko samples: Subaqueous; SCC Taiko Drums by S. Christian Collins; Floe-compatible port. The source library additionally credits whatsanickname4u on Freesound. See [the asset attribution](../tools/research/taiko-bank-05/ATTRIBUTION.md) for exact links, adaptation details and CC BY-SA 4.0 notice. The audio attribution and license are embedded in the MP3 metadata. These credits do not indicate endorsement.

## Reproduction and hardware boundary

Run tools/research/mix_apocalyptic_prototype_05.py OUTPUT_DIRECTORY tools/research/taiko-bank-05 PATH_TO_VERSION_03_STEMS.wav with the same Python dependencies as the earlier audition tools. It decodes the checked-in compact bank, not the larger original FLAC recordings. Source hashes and transformations are recorded with the bank.

This remains a desktop sound/arrangement audition. The installed firmware has not changed. The 40,312-byte audio bank alone is smaller than the previously measured roughly 50.5 KB of free application image space. A linked build must still prove decoder/mixer/code alignment space; memory, real-time flash access and device CPU with bass, drums, effects and live lead remain unmeasured. No external sample storage has been overwritten. Acceptance of the recorded drum sound should precede the firmware playback port.

## Acceptance and native integration

Dylan explicitly approved this sound and direction, then requested all compatible Paleotone/Gurdy beats. The native candidate now has all eleven rhythm families and a six-voice ADPCM taiko player. Desktop approval does not establish FM-1 CPU stability or acceptance of the native mix; those remain device listening checks.
