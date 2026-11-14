# Apocalyptic prototype 01

[Play the 77-second MP3](https://chatgpt.com/library?file_id=libfile_a65ed02322f08191950d1a7044a1dd49&account_id=personal).

An original fixed-score desktop direction check using T-WRECKS's actual Paleotone voice DSP. Bass holds C1/Db1 (32.7/34.6 Hz); Temple Organ provides the body and Titan Brass carries the restrained phrase. TRIBE BOOM/LOG provide spaced impacts. The music reduces at 0:36 and returns more fully at 0:44. No creature effects are needed to supply its identity.

The score runs at 54 BPM and reaches six simultaneous melodic voices out of the existing eight-voice budget. Offline stem gains and a damped desktop room are used for this audition; this is not a render of the current generative composer, an FM-1 CPU measurement, or a claim that the complete mix is already ported. Listening acceptance is still required. The connected device is unchanged.

The player's supplied reference compilation was examined privately as additional guidance. Its varied tonal balances reinforce that a single spectral percentage must not become a genre rule. Neither that audio nor its per-window measurements are published in this repository. No reference audio is sampled into the prototype.

Reproduce the voice render by compiling tools/research/apocalyptic_prototype.c with build/host/pt.o, -O2 -ffp-contract=off -DEN_HOST -I. and -lm. Pass a four-channel stems WAV path and events CSV path. Run tools/research/mix_apocalyptic_prototype.py OUTPUT_DIRECTORY with those stems named stems.wav; it uses NumPy, SciPy, soundfile and imageio-ffmpeg, optionally installed under OUTPUT_DIRECTORY/../genre-analysis/packages.

Measured output: 77.111 seconds, WAV peak -2.00 dBFS, stereo RMS -17.45 dBFS. WAV and decoded MP3 are finite and unclipped; the closing tail fades below -60 dBFS RMS in its last tenth-second. Numeric checks establish file integrity and headroom, not musical success.
