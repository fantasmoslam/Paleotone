# Dinosynth study tools

These diagnostic tools do not modify firmware or install to a device. See docs/TWRECKS-APOCALYPTIC-AUDIO-STUDY.md for conclusions and limitations.

- dinosynth_audio_analysis.py WORKDIR reads WORKDIR/references.json (copy docs/research/dinosynth-reference-streams.json), public stream audio named ID.mp3, and optional twrecks-stampede/bass/extinction/drums/drone.wav files. It writes analysis.json; it does not download audio. Install librosa, NumPy, SciPy and imageio-ffmpeg in the environment, or into WORKDIR/packages.
- render_dinosynth_diagnostics.c links amb.c and the host pt.o. Arguments: preset-index, mode (0 full / 1 dry bass / 2 drums / 3 drone), seconds, WAV-path, MIDI-CSV-path. It enables MIDI logging, records block-resolution note-on events and writes stereo PCM16. Example compiler flags: -O2 -ffp-contract=off -DEN_HOST -Ifirmware/src/dsp. Each run should start in a fresh process.
- render_dinosynth_tones.c links host pt.o with -DEN_HOST -I. and takes an output directory. It writes six-second float WAVs for four supported bass voices at MIDI 24 and 36.
- dinosynth_tone_analysis.py WORKDIR measures those tones and summarizes bass-events.csv.

Do not treat the MIDI chord logger as an exact audio-stem transcription: the mono Paleotone drone path can synthesize fewer notes than the legacy logger emits. Do not treat independently rendered diagnostics as additive stems. Stream duration checks reject truncated files. Pulse and chroma results are automated candidates, not verified BPMs, chords, instrumentation, motifs or section transcriptions.

The published JSON contains the final measurements and content hashes. Copyrighted stream audio, dependencies and local renders are excluded from git. Reproduction needs the corresponding artist-provided public streams or lawfully obtained copies; no album purchase or access-control bypass is automated here.
