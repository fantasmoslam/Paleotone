# ORK recorded-voice FM-1 prototype

This is the implementation prototype following Dylan's approval of the recorded-voice chant preview. The voice bank is generated from owner-supplied recordings and remains a separate private input. No recordings or extracted sample data are committed here.

## Implemented

- Portable integer C player: IMA ADPCM, prepared exact-note assets, three voices, sample-rate interpolation, held vowel loops with restored decoder state, release tails, click-reduced stealing, group-owned note-offs, and immediate panic.
- Single notes, power chords, minor and major triads. Chords are admitted atomically; an unavailable note rejects the request without disrupting the existing chord.
- C2–B3 bank range (MIDI 36–59). Power-chord roots are C2–B2 so the octave stays in range.
- Offline packer with filtered resampling, vowel-loop search, codec verification, CRC, exact source-rate metadata, and three Felucca-compatible user-slot wrappers.
- A Paleotone `engine_t` adapter, plus an integration script restricted to a clean isolated checkout. It appends engine 16; it does not reorder existing engine IDs.
- Synthetic test fixtures, malformed-bank checks, control lifecycle checks, block-size invariance, a host renderer using the same C core, and adapter type-contract compilation.

## First private bank

The prototype prepares GRAH, BOM, DOOM, RUM at 24 pitches each, using the same provisional syllable assembly as the approved desktop preview. The resulting 96 assets are 225,728 bytes at 11,025 Hz/4-bit IMA, leaving 18,496 bytes in three 81,408-byte payload regions. These are measured generated-file sizes, not proof of target CPU performance, sound equivalence after compression, or successful hardware integration. Labels remain order-based pending listening confirmation.

Preparation of those private PCM files currently uses the working C# audition/export pipeline, not a complete production recording editor. `tools/pack_bank.py PCM_DIR OUTPUT_DIR` accepts the exported files and reproduces the packed bank. The full recording set is larger; this prototype deliberately uses four syllables to establish the device path.

## Build and test on a computer

```
make test
build/ork-render /path/to/private/ork-bank.bin audition.wav
```

The core performs no allocation, floating-point arithmetic, mutex acquisition, file access, or flash writes. Bank parsing/CRC happens while audio is stopped. Host tests do not establish the FM-1's real-time headroom.

## Device integration and present boundary

The adapter is written for the Felucca/Paleotone API in the inspected source snapshot. `tools/integrate_paleotone.py /path/to/clean/isolated/checkout` stages the code for a firmware build. It refuses tracked changes and unfamiliar source anchors. The adapter uses the existing 0xA0000 user-sample area, not a new invented flash address. Preparing the source does not install it.

The bank wrappers use all three user sample slots. Existing data in those slots would be replaced by a future upload; no upload is performed by this prototype or its tests. The wrappers' payload is an ORKB container, not a musical SAMPLE instrument. Select the ORK engine, not the generic SAMPLE engine, for this data. Missing or invalid bank data produces silence. Bank installation, slot backup, actual hardware mapping, CPU/load measurements, and full target qualification remain separate work. Reboot is required after a future bank upload because CRC parsing is deliberately kept out of the audio callback.

The adapter caps a part at three voices; the existing four-part platform can instantiate more than one ORK part. The first intended qualification configuration is one active ORK part. This is not a claim of a universal three-voice global cap in the inherited workstation.

## Provenance

GPL-3.0-only, using the existing Felucca IMA codec convention and Paleotone engine API. The host voice preparation derives from the approved recorded-syllable PSOLA/splice experiments; the real-time player uses prepared audio rather than running that desktop processing on the MCU. Preserve the containing repository's licences/notices. User voice assets are not assigned a licence or distributed by this source commit.

No installable, hardware-qualified release is claimed by this checkpoint.
