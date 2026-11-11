# T-WRECKS: why the current sound misses the brief

Research date: 10 November 2026 (Arizona). Candidate examined: 91dcc88. This is an audio-analysis and source-audit report, following Dylan's rejection of the hollow bass, upbeat/noir character and unsuitable instruments.

## The actual target

Dylan's words **“apocalyptic” and “consequential”** govern the next prototype. The music needs looming scale, weight, gravity and meaningful changes. A heavy scene should feel dangerous and inevitable, without requiring a literal march. Dinosaur calls remain playable accents; the composition needs to convey its identity with them silent.

The existing candidate has not met that target. Passing level checks, using a minor scale and naming patches after prehistoric things did not establish the right musical character. The last pass increased percussion weight and bass activity while retaining the old composition grammar. That was insufficient.

## What was actually examined

Eleven complete public Bandcamp streams were decoded and measured: **48 minutes 10 seconds** across Diplodocus, Thagomizer and Synthosaurus. Five two-minute candidate diagnostics were also rendered: STAMPEDE full mix, isolated bass, isolated drums, isolated drone, and EXTINCTION full mix. Eight controlled bass tones tested four existing voices at MIDI 24 and 36. A separate MIDI trace recorded the actual bass notes from the candidate.

This is computational analysis of audio plus inspection of the composing/synthesis code. **It is not a claim of human critical listening, an ear-verified transcription, or certain identification of instruments from a mixed recording.** Instrumentation statements below come from the artist interview. Musical direction comes from the user's listening feedback, interpreted alongside these measurements. Exact reference chord charts, time signatures and melodies have not been established.

No reference audio is redistributed, copied into the firmware or used to train a model. The data file contains measurements, public source links and content hashes. Raw streams and working renders remain temporary analysis inputs.

## References and selection

The core comparison is Diplodocus's *Slow And Heavy* and B-side, Thagomizer's *Mesozoic Mood Music* and *Time Of The Titans*. Synthosaurus's *Metalosauric Crush* and *Dino War* provide a heavier, sometimes faster comparison. This is a deliberately selected corpus, not a census of dinosynth.

[Diplodocus describes the original release](https://diplodocus.bandcamp.com/album/slow-and-heavy) as drawing on classic dungeon synth and older depictions of dinosaurs. In the [Thagomizer artist interview](https://hamiltonundergroundpress.com/features/outside-my-usual-wheelhouse-a-qa-with-caspar-kneale-of-thagomizer/), Caspar Kneale identifies Diplodocus as his main influence and distinguishes Synthosaurus's more power-metal-like approach. He describes building tracks from tuba, French horn, trombone and trumpet, with timpani and 808 percussion. That gives us a concrete orchestration lead: a substantial musical body supported by impacts. It does not establish that every dinosynth release shares that palette.

### Full-track measurements

Pulse figures are onset-autocorrelation candidates, not verified BPM markings. Half/double-time ambiguity remains; unrelated subdivisions can also score highly. Percentages are spectral power relative to 20 Hz–10 kHz after a mono fold, not perceived loudness or stem proportions.

| Reference | Duration | Strong pulse candidates | 20–60 Hz | 250–1,000 Hz |
|---|---:|---:|---:|---:|
| Prowl of the Concavenator | 4:20 | 64.6 / 32.3 | 0.0% | 34.9% |
| Tanystropheus' Reign | 2:52 | 64.6 / 32.3 | 0.0% | 57.8% |
| Return of the Thunder Lizard | 3:43 | 76.0 / 37.4 | 4.2% | 55.5% |
| Primal Rage | 3:59 | 60.1 / 123.0 | 2.1% | 52.1% |
| Largest To Live | 3:38 | 60.1 / 39.8 | 22.5% | 47.6% |
| Migration Of The Camarasaurus | 4:32 | 55.0 | 17.6% | 47.7% |
| Impact Winter | 7:01 | 37.4 / 76.0 | 16.9% | 43.8% |
| Tears of Flasmosaurus | 5:24 | 123.0 / 39.8 | 10.3% | 28.4% |
| Million Years of Terror (Alternative Version) | 2:52 | 60.1 / 117.5 | 8.6% | 31.0% |
| Dinopocalypse | 4:36 | 172.3 / 117.5 | 26.6% | 30.6% |
| Thunder Lizard | 5:14 | 45.3 | 4.8% | 60.9% |

Sources: [Slow And Heavy](https://diplodocus.bandcamp.com/album/slow-and-heavy), [B-side](https://diplodocus.bandcamp.com/album/slow-and-heavy-b-side), [Mesozoic Mood Music](https://thagomizer.bandcamp.com/album/mesozoic-mood-music), [Time Of The Titans](https://thagomizer.bandcamp.com/album/time-of-the-titans), [Metalosauric Crush](https://synthosaurus.bandcamp.com/album/metalosauric-crush), [Dino War](https://synthosaurus.bandcamp.com/album/dino-war).

Several core references have strong candidates around 45–65 pulses per minute. Others have faster candidates, and *Dinopocalypse* is ambiguous under this method. A single genre BPM rule would be unjustified. For our next prototype, slower perceptual movement is a reasonable design choice because Dylan rejects the current upbeat result; it is not a discovered universal rule.

Low-end weight also varies markedly. The two original *Slow And Heavy* tracks put almost no measured power below 60 Hz while maintaining a much greater 250–1,000 Hz proportion than STAMPEDE. That directly argues against treating “more sub” as a sufficient substitute for a musically substantial sound.

## Diagnosis 1: the supposed deep bass lives too high

The 120-second STAMPEDE bass trace contains **246 note-on events**. The median time between onsets is **0.313 seconds**, consistent with frequent eighth-grid activity at 96 BPM. There are no commanded bass notes below MIDI 36 / 65.4 Hz. **80.9% of its bass note events are at 98 Hz or above; 50.4% are at 130.8 Hz or above.** This counts events, not time spent sustaining each pitch.

| MIDI note | Nominal fundamental | Onsets | Share |
|---|---:|---:|---:|
| 36 | 65.4 Hz | 33 | 13.4% |
| 41 | 87.3 Hz | 14 | 5.7% |
| 43 | 98.0 Hz | 75 | 30.5% |
| 48 | 130.8 Hz | 82 | 33.3% |
| 53 | 174.6 Hz | 23 | 9.3% |
| 55 | 196.0 Hz | 19 | 7.7% |

The source explains this: Sequence 2 starts from MIDI 48 plus the octave setting; STAMPEDE sets that octave to -1, giving a starting root of 36. The shared sequence generator then favors root/fifth/octave cells, enables additional octave jumps, and transposes through chord changes. It is explicitly a Berlin-school generator in *seq.c*. Lowering its starting register by one octave did not turn it into an anchored low-register part.

**Design consequence:** the bass needs its own role. Establish a low tonic/pedal, sustain it across multiple beats, and make departures purposeful. Start auditioning roughly 33–73 Hz roots with audible harmonics above them. Those are prototype test bounds, not a genre prescription. Remove automatic high octave decoration from the foundational part. An upper motif can move independently.

## Diagnosis 2: the mix's sub-bass is mostly impact, while musical body is weak

| Candidate diagnostic | 20–60 Hz | 60–120 Hz | 120–250 Hz | 250–1,000 Hz | 1–10 kHz |
|---|---:|---:|---:|---:|---:|
| T-WRECKS stampede | 56.6% | 3.5% | 33.8% | 5.7% | 0.4% |
| T-WRECKS bass | 3.5% | 23.8% | 44.3% | 28.0% | 0.4% |
| T-WRECKS extinction | 0.0% | 3.1% | 45.0% | 25.1% | 26.8% |
| T-WRECKS drums | 67.0% | 0.3% | 31.9% | 0.6% | 0.3% |
| T-WRECKS drone | 0.0% | 5.6% | 65.8% | 25.7% | 2.8% |

STAMPEDE's full mix already concentrates 56.6% of measured power below 60 Hz. The drums-only diagnostic concentrates 67.0% there, while the bass-only diagnostic has 3.6% and the drone-only diagnostic approximately zero. This supports a diagnosis of impact-heavy sub-bass with a comparatively small harmonic body. The diagnostics are separate runs; muting layers can affect generation and voice allocation, and the bass test removes reverb/echo. They are not additive stems and should not be interpreted as an exact mix decomposition.

Across the first 30 seconds, the drum diagnostic measures -24.2 dBFS RMS, bass -34.0, and drone -43.1. They are roughly 10 and 19 dB below the drum diagnostic respectively. The disparity is substantial even though isolated-run RMS cannot establish perceived mix balance by itself.

The full mix has only 5.7% power at 250–1,000 Hz, versus 28.4–60.9% across the eleven complete references. A first-120-seconds comparison preserves the mismatch: those references span 26.5–61.3% in the same band. This comparison controls duration, not arrangement phase, mastering or instrumentation.

**Design consequence:** rebalance the musical foundation before adding more BOOM. Build a broad, low brass/organ/string body, then place impacts around it. Keep a stable bass fundamental and enough low-mid harmonic information for substance. Use saturation/filtering deliberately; a low-pass applied to already sparse partials can leave a soft, hollow result. Do not EQ the whole mix to match these percentages: they are diagnostic evidence, not a mastering target.

## Diagnosis 3: a patch's name does not guarantee the needed timbre

Controlled sustained tones compared Tar Growl, Sub Bass, Pulse and Fat Bass at MIDI 24 / 32.7 Hz and 36 / 65.4 Hz. The measurement window was 0.25–3.25 seconds, after the initial attack. Tar Growl placed about 99% of its measured 20 Hz–10 kHz power near the commanded fundamental in both tests. Pulse and Fat Bass retained substantially more energy outside that narrow fundamental band.

This does not prove a synth bug or describe the attack's complete character. It establishes that Tar Growl's sustained output in this harness is nearly sinusoidal, despite its name. A sine-rich foundation can be useful, but by itself it provides little harmonic body. Increasing its level or picking a prehistoric label does not solve orchestration.

**Design consequence:** audition the actual sustained sound and attack at the intended low pitches. Separate depth, body and aggression as sound-design jobs. First try one supported voice with a stable fundamental and controlled upper partials; if that fails, assess a light sub/body combination against the shared eight-voice limit. Avoid committing to another named preset without a low-register render.

## Diagnosis 4: inherited harmony plausibly explains the noir association

The harmony engine normally stacks four scale tones for seven-note pools. *en_voicing* supplies open seventh chords or a ninth-type voicing; *en_walk_chord* carries sevenths and major sevenths into generated slots. This happens even when a recipe's mode is Aeolian or Phrygian. The selected timbres and articulation can make such harmony read very differently from the intended scene.

A source trace at STAMPEDE's opening includes C, G, B-flat and E-flat in the chord system. The MIDI logger also emits a legacy pedal note that the mono Paleotone drone path does not necessarily synthesize; MIDI output must not be treated as an exact audio-stem transcription. The code paths, rather than that extra logged note, establish the automatic extensions.

Extended harmony is available throughout much music, including dark music. The engineering error was leaving that inherited preference in charge without checking it against Dylan's target. His “cheap detective daytime noir” report is credible listening evidence; source inspection identifies a plausible mechanism, not a proof that any single interval causes that association.

**Design consequence:** the apocalyptic profile should begin with pedals, bare fifths, octave weight and selected minor triads. Use suspended notes and semitone/tritone tension as deliberate events. Let harmonic changes have consequences. Default seventh/ninth wandering should be removed from this profile, while explicit user-selected harmony remains possible.

## Diagnosis 5: event density and scene development need different rules

The current bass shares the eighth-note sequencer machinery, root/fifth/octave selection and random gating. Many recipes also prominently use struck/bell/plucked colors. Combined with moving extended harmony, these mechanisms can keep suggesting activity and incidental accompaniment even when the scale and preset names promise menace.

The audio analysis finds recurring pitch-class patterns at several timescales in the references: roughly 15 seconds in *Tanystropheus' Reign*, 8/16 seconds in *Primal Rage* and *Largest To Live*, and around 17/18 seconds in *Migration Of The Camarasaurus*. These are lag-correlation observations, not transcribed motif lengths or bar counts. The exact phrase boundaries need ear verification. They support investigating retained ideas and deliberate transformations, rather than assuming that continuous random movement provides adequate form.

**Design consequence:** choose one clearly recognizable phrase, keep its identity, and vary orchestration, register, density and intensity at phrase boundaries. Make a reduction and return audible. Quiet passages should maintain tension. Percussion can imply great mass with deliberate spacing and long resonances; constant conventional backbeats are not required.

## Next prototype: apocalyptic and consequential

This is an original design proposal, not a transcription of any reference.

| Role | Starting brief |
|---|---|
| Foundation | A sustained low root/pedal, restrained movement, harmonically supported depth. |
| Main body | Low brass/organ/strings with a substantial middle register and measured attacks. |
| Motif | A short grave idea with rests, limited register and deliberate resolution or denial. |
| Harmony | Pedal/fifths/minor triads first; extensions and tension chosen for a purpose. |
| Percussion | Monumental impacts, variable reinforcement, room for decay; no literal march requirement. |
| Pacing | Initially audition a perceived 48–64 pulse range; density and durations matter as much as the tempo number. |
| Form | Establish weight, introduce the threat, remove support, then return with greater consequence. |
| Calls | Optional and manually placed; music must work with them off. |

The next work should produce **one short desktop audio prototype**, before a bank-wide rewrite or another hardware flash. Compare at matched playback loudness, on headphones first, then the FM-1. Evaluate whether the bass is genuinely foundational, the voices carry scale, and the changes feel consequential. Only then turn the useful rules into the generative profile and revise the weaker scenes. Preserve the approved controls and manual-call feature.

## Method, reproducibility and limits

The analyzer decodes to 22,050 Hz mono, uses a 4,096-point Hann-window STFT with a 512-sample hop, and measures spectral-power bands. Onset-flux autocorrelation supplies pulse candidates. One-second chroma summaries supply lag recurrence and six-second before/after novelty candidates. The JSON retains track-by-track results and tentative change timestamps; those timestamps are **places to inspect**, not asserted composition boundaries.

Streams were verified against their published durations within 0.2 seconds; native renders were checked for the complete 120 seconds. A truncated early bass diagnostic was rejected and replaced before this report's final measurements. Controlled tones use an independent, longer FFT window for narrow-band fundamental measurements. MIDI event timestamps are accurate only to the render-block interval.

The mono fold may change spectra of wide or phase-shifted material. Mastering, reverb, arrangements, codec compression and different sections all affect the comparisons. Power percentages do not measure emotional gravity. The corpus is too small and selectively chosen to establish genre-wide laws. Source inspection establishes how this firmware behaves; it cannot prove what a reference artist intended beyond their own statements.

Tested analysis environment: Python 3.12, NumPy 2.5.4, SciPy 1.18.1, librosa 1.0.0 and imageio-ffmpeg 0.8.1 / FFmpeg 9.0.2. [Measured data](research/dinosynth-audio-analysis.json) includes stream SHA-256 hashes. The scripts and diagnostic C harnesses are retained under tools/research. The firmware, preset bank and connected device were not changed during this research pass.
