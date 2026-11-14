# Felucca / FM-1 / SMK-37 firmware survey

**Survey date:** November 6, 2026, America/Phoenix.  
**Project:** DIY more powerful FM-1 from available parts.  
**Purpose:** establish what public firmware, engines, composition systems and device layers can be reused before starting tablet integration.

## Result and scope

The public-source screening snapshot is complete for the discovery inputs recorded below. It contains **1,215 repository candidates**, plus **13 explicitly inspected SMK/reference branches**. These counts include forks, libraries, tools, documentation, games, aliases and unavailable repositories. They are not counts of distinct musical instruments.

**11,619 unique requested files** were downloaded or recovered from the prior work and checked against their Git blob SHA-1. They represent **37,598 repository/path/commit references**. Representative production implementation was collected for **1,048 candidates**. The evidence package retains source locations, commit pins, observed licence information, source-tree equivalence groups and unsuccessful lookups.

This is a source and architecture survey, with automated screening across the inventory and focused semantic review of the principal reusable families. It is not a line-by-line correctness audit of every fork. No firmware was compiled, installed, listened to, benchmarked or physically qualified during this survey. A source download, an author's test claim and a successful device test are different evidence levels.

The main finding is that this project has substantial existing material to work from. The useful components fall into four groups: compositional logic, synthesis/effects, performance/sequencing, and host/controller infrastructure. The largest practical discovery is that RemoteFM1 already implements much of the hardware-panel/host relationship, including Android and an SMK build marked as testing. It should be assessed before designing another protocol.

## What was corrected in the previous survey

The earlier 512-candidate status was stale: the recovered working inventory already contained 765 candidates. Its broad GitHub searches had not reached their final pages. Eighteen repositories in a saved direct Felucca fork listing had also not been merged into that inventory. Both omissions were corrected.

The first source selector sometimes chose `tests/hostsim.c`, a UI test or a thin wrapper as its only implementation reference. That is useful test evidence but insufficient to characterize the engine. This pass expanded collection to real application implementations, engine bodies, DSP directories, generator/sequencer code and selected interface/inline-DSP headers. Capped earlier source reads were replaced with complete, hash-verified bytes. Branch-specific SMK code was collected separately from default branches.

Newer catalogue references led to AQUORA, DEEP FIELD, MODFM, MODFM1 and ARTIHIVE, among others. Those entries were added rather than assuming the older catalogue was exhaustive. A GitHub topic URL was retained as an explicit exclusion, not counted as a repository that passed screening.

## Discovery coverage and stopping rule

| Input | Use and evidence |
|---|---|
| [FM-1 resource hub](https://github.com/aroum/fm1-custom-fw) | Current README fetched; repository links compared with the recovered inventory. This is the GitHub resource hub requested by the user. |
| [2bbb collection](https://github.com/2bbb/fm1-firmware-collection) | Prior and current 314-entry catalogues, build recipes, commit pins, licence metadata and overrides retained. |
| [The Firmware Hub](https://thefirmwarehub.com/) | Previously saved November 2 catalogue retained and its public page checked through web retrieval. A fresh direct catalogue request returned HTTP 403; the old catalogue is not represented as a newly downloaded live feed. |
| [FM-1 editor directory](https://fm1-editor.com/firmware/) | November 6 public directory checked for recent additions and the watch list. It shows 274 cards with overlapping categories. Its direct HTML download returned HTTP 403; targeted primary-source follow-up was used. This was a supplemental check, not a complete machine-readable import of all 274 cards. |
| [SMK-37 hub](https://genkigenki.github.io/smk37/) | Current HTML downloaded; repository and explicit branch links extracted. |
| GitHub discovery | Felucca search continued through page 4, which returned 64 results; the FM-1 firmware search continued through page 10, which returned 54. Relevant candidates were merged case-insensitively. A saved 100-entry direct Felucca fork list was reconciled. |
| Other catalogues | Recovered emulator, Atlas and awesome-list inputs retained as discovery evidence. |
| Primary references | READMEs and selected source from RemoteFM1, its Android/iOS hosts and its firmware catalogue added. |

Discovery closes at this dated snapshot after these finite inputs have been reconciled. This does not establish that every public repository on the internet has been found. Search ranking, deleted/private projects, changed names, undiscovered forks and updates after collection remain limits. The source paths are pinned to the inspected commits rather than presented as universally current releases.

## Compositional systems worth carrying forward

These are engineering assessments of the inspected designs. They are not listening scores or endorsements of musical quality.

| Source | What is actually useful | Boundary |
|---|---|---|

| [Rastapopolix/tessera](https://github.com/Rastapopolix/tessera/blob/5ce3580fd749b198b8688229d87cdaa4110923d0/README.md) | A compositional system with multiple generators, separate random streams, phrase evolution, section recall, project-wide form and slow energy/rest behavior. `gen.c`, `form_chain.c` and `energy.c` expose these as concrete mechanisms. | Most suitable reference for structured generative behavior; experimental status and GPL remain relevant. |

| [delirioux/tuna](https://github.com/delirioux/tuna/blob/3f1567b9f33cd7896cdb22e23bc288c4ada447cf/README.md) | Marbles-derived randomness is shaped by harmonic, melodic, rhythmic and formal rules. `tuna_gen.c` separates generative decisions from the firmware/render adapter. `tuna_app.c` manages seeds, selection and evolution. | Its large style library and measured-score claims need their own data/provenance audit before treating them as validated style models. |

| [charlesvestal/fm1-amb](https://github.com/charlesvestal/fm1-amb/blob/e642bf1efb618768ba2f3bb5ff716f465db94457/README.md) | Unequal bell loops, chord-aware pitches, controlled growth/pruning and Euclidean percussion. The DSP source has separate chord-change, loop-pass, drum-pattern and evolution routines. | A strong small ambient system; changing a seed or adding presets does not by itself provide broad musical form. |

| [FartyPants/fm1-nachtfahrt](https://github.com/FartyPants/fm1-nachtfahrt/blob/ba0728378f9a9aa5ef1f69027c42d03319f7ef8e/README.md) | Two shifting sequences, held harmony, evolving timbres and lead phrases/call-and-response. The inspected DSP contains separate sequence growth, note choice, motif and lead-tick logic. | A distinctive Berlin School vocabulary rather than a universal composer. |

| [Reytian/fm1-min1](https://github.com/Reytian/fm1-min1/blob/85c26d3055c22e43c88c6605144e4cd9f1d03b9b/README.md) | Audible compositional processes: additive arpeggios, phase drift/locking, independently advancing pattern chains and a bell/melody relationship. | Useful for coherent variation; the four models intentionally constrain the musical language. |

| [aceyprime/zen-1](https://github.com/aceyprime/zen-1/blob/ec9295233ea64ddac224b58b4d07216706c1b8ee/README.md) | Scene events trigger notes; Sakura, Koi and Hotaru have separate implementations. Musical scale, held harmony and a slow conductor constrain the event stream. | Especially relevant to environment-driven music. Graphics, nature assets and composition logic should be treated as separate dependencies. |

| [davidpryan/haunted-fm1](https://github.com/davidpryan/haunted-fm1/blob/bd031d3722c5e3432392b6e60979379ad2bead1c/README.md) | Five separate applications: route-shaped arrangement, vocal fragments, off-grid percussion, voice-led harmony and progressively worn loops. Each has its own app and DSP engine. | One repository contains five firmware concepts; repository counts conceal that distinction. |

| [jasonpersinger/ghoulbox-fm1-dungeon-synth](https://github.com/jasonpersinger/ghoulbox-fm1-dungeon-synth/blob/9f2b9a22e456d7fe9552b40f7a1d0c13200913bb/README.md) | The inspected tree includes both `dream.c` and `scenes.c`, in addition to dungeon timbres, ambience and effects. It is a composition/performance source as well as a sound source. | README release labels and newer changelog sections coexist; use the pinned source, not an inferred release number. |

| [BunnyOrchard/jam-playground](https://github.com/BunnyOrchard/jam-playground/blob/c400d548f0ae100e8e524a704d404cf2b2710fdd/README.md) | Boot-time song creation, vibe changes, pattern mutation and bar-boundary changes, through the HYP-1/AMB-1 lineage. | Treat its documented variations as a branch of a shared family rather than an entirely independent engine. |

| [RustyPawz/fm1-HYP1](https://github.com/RustyPawz/fm1-HYP1/blob/6908dc1897841463c462202a92aa3aff72c3addb/README.md) | An AMB-derived generative techno direction; useful comparison for how the ambient architecture is repurposed toward groove. | Needs evaluation on its own terms; inherited framework code is not an extra synthesis engine. |

| [zeptosecond-dsp/Arcturus-S6](https://github.com/zeptosecond-dsp/Arcturus-S6/blob/1ad7f6016e75a6f27fe3d69ebb4c606cce45d8cc/README.md) | A different generative direction: evolving wavetable voices driven by function generators, sample/hold and slow modulation. | A sound/behavior source; do not equate absence of a conventional sequencer with absence of structure. |

| [tensor-programming/fm1-t1](https://github.com/tensor-programming/fm1-t1/blob/bea0fd38fbebe48758059828a7ba107c6248304c/README.md) | A Torso-inspired exploration with source available for comparison. | The README is extremely sparse and calls it work in progress. It is not evidence of a finished T-1 equivalent. |

The useful common pattern is constrained variation: independent seeds or streams, a harmonic frame, phrases with memory, bounded mutation, sections and purposeful silence. A large preset list is a different asset. The survey records both without treating them as interchangeable.

For a larger shared composition dataset, retain recipe tables, scales/chords, phrase templates, generator state machines, transition rules and their original attribution separately. The code collection is a research corpus; it is not automatically a curated set of musical examples suitable for model training.

## Synthesis and effects inventory

| Family/source | Material available | Integration implication |
|---|---|---|

| [hugelton/Felucca-Engines](https://github.com/hugelton/Felucca-Engines/blob/d0260074ce100a47675814b7f38d61f11176c926/README.md) | Six MIT-licensed fixed-point C99 engines in the inspected README: ANALOG, LOFI, TRIO, WHEEL, NOISE and PHASE, plus shared DSP, tables and a host contract. | The clearest small reusable DSP library. Default rate is 44.1 kHz; its documented supported rate range is 32–96 kHz with generated tables. Inspect the pinned API rather than copying rate constants. |

| [hugelton/Felucca](https://github.com/hugelton/Felucca/blob/a4f007c9211ea5b37298c01ff6d73430fe2cb96a/README.md) | The full multi-engine instrument, including voices, presets, sequencing, effects, storage and editor behavior. Additional DSP has its own dependencies. | An application/platform source, not just the six-engine MIT library. Whole-firmware licence and imported-component notices remain distinct. |

| [keremimo/melodee](https://github.com/keremimo/melodee/blob/165d177a425944f3d30b0b669255a6ed0050c3a5/README.md) | CZ/phase-distortion, FM6, Prophet-style synthesis, SID and physical/formant sources, with expanded sequencing and MIDI infrastructure. | Strong source of richer synthesis and performance work. Hardware-core scheduling and firmware-specific data formats require an adapter. |

| [jasonpersinger/ghoulbox-fm1-dungeon-synth](https://github.com/jasonpersinger/ghoulbox-fm1-dungeon-synth/blob/9f2b9a22e456d7fe9552b40f7a1d0c13200913bb/README.md) | Hurdy-gurdy, dungeon-oriented patches, ambience, hall/tape treatment and generative/performance layers. | A good thematic library for dark/prehistoric/fantasy instruments. Separate patch curation, DSP, effects and sequencing in the inventory. |

| [ruppert/fmini-1](https://github.com/ruppert/fmini-1/blob/7619d903fa732286f3912a378f29bf71b8057a0c/README.md) | Dedicated Minimoog-inspired synthesis and its performance application. | Already has a project-owned SMK branch, inspected independently. That port is the useful board/control reference. |

| [ruppert/organvave-1](https://github.com/ruppert/organvave-1/blob/9e49485ddcfd6a08205b16110dd334ddec1b1bd7/README.md) | Integer setBfree-derived tonewheel work, drawbars and organ/rotary-related source. | The README contains old milestone text alongside a later manual and explicitly says physical testing is absent. Do not call all listed milestones qualified. |

| [R-Jeb/setBfree-fm1](https://github.com/R-Jeb/setBfree-fm1/blob/bbc446d7dca81fd6122d5f1a2329947481c65835/README.md) | Another setBfree/Dinghy organ route. | Compare code provenance and actual implementation before importing a second organ stack with overlapping ancestry. |

| [charlesvestal/fm1-x0x](https://github.com/charlesvestal/fm1-x0x/blob/ab4aa06c82450900e0fb6c2c8c3b279af9bf9fae/README.md) | 303, 808/909, break manipulation, groove generation, sequencing and shared effects. | Useful rhythmic voices and arrangements; some models have licences different from the platform. |

| [DeadActive/hortator](https://github.com/DeadActive/hortator/blob/44c9ad4aa57bebf63f412bd552e57143c30b4065/README.md) | Percussion-oriented synthesis/application source, including varied drum models. | A focused drum reference rather than another generic four-track synth. |

| [JoshCowley/MODFM](https://github.com/JoshCowley/MODFM/blob/0ef8310bcb9239b97102eb8044b2ba987b6ae3a1/README.md) | A Felucca-based Mutable-inspired rack direction retaining much of the instrument shell. | Distinct from MODFM1; preserve that distinction when comparing build and device status. |

| [JoshCowley/MODFM1](https://github.com/JoshCowley/MODFM1/blob/98e49fd161af795694089afecbb7d4bce686061f/README.md) | A separate rack implementation with `engine/rack.cc` and module/firmware adapters. | One author has multiple architectures. A similar name is not proof of the same code or readiness. |

| [WisestWizard/deepfield](https://github.com/WisestWizard/deepfield/blob/e4fa27b5144e9fba3dfb8fc2c331f64fee092874/README.md) | A repository containing Pocket Poly and a separate Deep Field application with its own drone engine, effects and scenes. | Subdirectory applications need separate profiles; the root README alone does not describe every instrument. |

| [CodingSlapfu/AQUORA](https://github.com/CodingSlapfu/AQUORA/blob/b8743a39420e9f4c9fa2bd426d8ccf54876a0fba/README.md) | A generative multi-role instrument with a dedicated `device/dsp/engine.c` and validation material. | Source is a useful comparator. Catalogue claims about tones or sound effects are not established health effects or measured audio quality. |

| [KHAVld/ARTIHIVE](https://github.com/KHAVld/ARTIHIVE/blob/ed53f4b0ee3b33884c8026a64f6c9ef278a0ba6b/README.md) | FM-pair drone/poly behavior with inline DSP in `firmware/src/artihive_dsp.h`. | An example of why collecting only `.c` engine names misses real implementations. |

| [fantasmoslam/Paleotone](https://github.com/fantasmoslam/Paleotone/blob/99a39c19f81f85d1df3d999e6a578dc4af9f1b81/README.md) | The existing dinosaur instrument, its native host work, custom sound/scape code and performance history. | Use as the project baseline. This survey did not requalify the device or Android builds. |

Other branches in the register broaden the palette with samplers, trackers, wavetable, bytebeat, chord instruments, modular builders, vocal engines and specialized synth models. The machine-readable register retains their names, descriptions, source paths and documented feature evidence. They are not silently dropped because they are outside the ambient shortlist.

## Sequencing and performance sources

| Source | Reusable ideas |
|---|---|

| [isod89/sloop-fm1](https://github.com/isod89/sloop-fm1/blob/bac9c05ea7627449405aa10e48da21d75b6c8c56/README.md) | Live capture, editing, pattern/section organization, synthesized drums and performance effects. Many forks share large parts of this code. |

| [charlesvestal/fm1-loom](https://github.com/charlesvestal/fm1-loom/blob/faf459d1f9cea3e634b6e2622a1c3e91d58b3452/README.md) | Track organization, pattern weaving, a conductor, locks, throws, scenes and song-level structure on fm1-base. |

| [Quixotic7/ChoralRootFM1](https://github.com/Quixotic7/ChoralRootFM1/blob/d7b509f3c4cb811d8251743f6620ee41805a4731/README.md) | Chord selection/voicing as a playable instrument, with bass/performance relationships. |

| [charlesvestal/fm1-fomni](https://github.com/charlesvestal/fm1-fomni/blob/9581e1b57ea0ac8a972c46f41caa898f5d232328/README.md) | The chord-harp interaction model and approachable root/strum separation. |

| [zednaked/jangada](https://github.com/zednaked/jangada/blob/b546675f965b15704b0d9f56c38401c777d98048/README.md) | Latched/evolving drones, modulation and performance-oriented controls; source includes dedicated drone behavior. |

| [Mr-Roboto/fm1-x0x-rmx](https://github.com/Mr-Roboto/fm1-x0x-rmx/blob/c7a633d01dca5faf714b436b92a1bf9a87729cee/README.md) | A remix-oriented extension of X0X; compare changed systems against upstream before counting duplicate features. |

| [genkigenki/fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37/blob/54252ffa2219e3c9cd7eb70f4e99b361eaa048e1/README.md) | Concrete board-port patches and model-specific documentation, useful across several musical applications. |

## Host and controller findings

### Existing approaches

| Approach | Evidence | What it means here |
|---|---|---|
| RemoteFM1 controller + host | The firmware README describes panel events sent to a host and LCD/LED output returned. It links an Android host, portable `.rfm1` builds and SMK support marked as testing. | This is an existing implementation of the project's plug-in-panel concept. Assess its protocol, device coverage and maintenance before inventing another. |
| RemoteFM1Android | Its pinned README requires Android 10+ and arm64, describes native built-ins and imported WebAssembly cores, and records both headless checks and real-device issues. | It is useful source, not proof that the S6 Lite or this exact two-device setup is qualified. CPU capability alone does not establish the installed Android ABI. |
| FM1 DECK | Native firmware modules with an API/ABI boundary, plus a host that parks the audio slot before swapping or accessing a core. | A concrete native-module design to compare against the existing Paleotone host. Its README labels it preview and arm64. |
| fm1-studio | Host firmware/session interfaces and per-firmware adapters are present. | Particularly useful for understanding the adapter work required when source layouts and state formats differ. |
| fm1-emu and emulator-based hosts | Execute device firmware through a simulated chip/platform. | Broader binary compatibility can be traded against emulation cost. This is a different route from compiling an engine natively. |
| CTL-1 and fm1-ctrl | Controller-oriented firmware projects are indexed. | Compare which inputs they expose and whether they support screen feedback or a full legacy panel contract. |

Primary references:

[Quixotic7/RemoteFM1](https://github.com/Quixotic7/RemoteFM1/blob/ed06aa502c7b75a3414c9c1976d0a5abd4350f57/README.md), [Quixotic7/RemoteFM1Android](https://github.com/Quixotic7/RemoteFM1Android/blob/3e3ea79b729da53c11196513e0ebb9a56ef42ab5/README.md), [Quixotic7/RemoteFM1FW](https://github.com/Quixotic7/RemoteFM1FW/blob/e782ff0dcfb3232b228beebfa56d5f2be690de28/README.md), [TonalPAK/FM1-DECK](https://github.com/TonalPAK/FM1-DECK/blob/ffb17001512b461920ec2bae53bbd46f284f4aee/README.md), [thingsapart/fm1-studio](https://github.com/thingsapart/fm1-studio/blob/76372746ca8579bb04eeaf67748adbb7c0b76d6d/README.md), [joakimeriksson/fm1-emu](https://github.com/joakimeriksson/fm1-emu/blob/1c6e80a816f07f350aa8abbbce5f040992b219f3/README.md), [charlesvestal/fm1-ctl](https://github.com/charlesvestal/fm1-ctl/blob/9bb5abcc6ef6e0af803a868ecda166e76e90d8d6/README.md), [johnskiller/fm1-ctrl](https://github.com/johnskiller/fm1-ctrl/blob/6c4d4d45d73019e6af6bc379d0b7a4b18ed70680/README.md).

### Compatibility evidence

| Layer | FM-1 | SMK-37 | Survey conclusion |
|---|---|---|---|
| Upstream Felucca application | Its README explicitly supports FM-1 only. | Third-party ports are separate projects. | Do not describe upstream as supporting both. |
| felucca-bsp | FM-1 board support is documented. | The inspected README calls Elite planned/not started and Pro planned/no board support. | It is not the ready dual-device hardware layer, despite sharing chip ancestry. |
| genkigenki port family | Starts from FM-1 applications. | Hub branches/packages provide model-specific ports. | The concrete port code is more relevant to SMK than a promise in upstream BSP documentation. |
| Project FMini-1 branch | The project's earlier FM-1 preservation work is historical context. | `fantasmoslam/fmini-1@smk37` was pinned and inspected. | Survey evidence covers source; previous hardware reports were not repeated here. |
| RemoteFM1 | FM-1 panel/host operation is documented. | Pro, Elite and MKE-P37 are listed, explicitly in testing. | A particularly relevant dual-device candidate, with testing status preserved. |

The expanded keyboard, pads, faders, wheels and encoders need explicit mapping. Translating every SMK action into an old 27-key FM-1 interface may hide its extra controls. Absolute faders also need appropriate pickup/feedback behavior when a host changes instruments. No hardware verification was inferred from a compile target or a catalogue card.

## Platform and performance lessons

Native source, device firmware and browser previews are three different portability assets. A browser preview demonstrates an existing host abstraction; it does not automatically yield an Android build. Device code may still rely on memory-mapped peripherals, interrupt timing, fixed addresses, linker sections, compiler extensions and global state.

The smallest reusable boundary is an engine API. The next level is an application behind an emulator HAL. A complete device image is useful to an emulator even where a clean native adapter is absent. Those routes should be evaluated separately rather than promising one universal binary loader.

Several practical points recur in the source:

- Keep note/clock scheduling independent of screen redraw work.
- Preserve a firmware's virtual sample rate or adapt it explicitly. Android output rate is not permission to replace all 44.1-kHz constants.
- Treat core switching, reset, storage restoration and renderer lifetime as part of the host contract.
- Make old notes, pedal state, held buttons and modulation state explicit when switching instruments.
- Separate the selected engine's persistent storage from the physical device's flash/bootloader.
- Do not transfer embedded dual-core claims into Android speedup estimates. This survey measured neither.

These are implications of the inspected designs, not new implementation completed in this task. Wi-Fi development remains outside this work.

## Provenance and licence findings

The catalogue licence field is not a substitute for the inspected source. The register therefore keeps catalogue declarations, collected licence files and observed SPDX identifiers separately. For example, current AMB-1 material identifies MPL-2.0 while earlier forks preserve GPL-derived versions. The same project name can refer to materially different historical snapshots.

Felucca as a whole, the standalone MIT engine library and the MPL hardware layer must not be collapsed into one licence. Imported DSP, samples, fonts and other assets may carry further terms. A project using a permissive hardware layer does not thereby change the licence of an inherited GPL application. Refer to the original notices at the pinned commits when choosing actual code.

The corpus preserves source bytes and repository/commit/path attribution. Source-tree equivalence is a deduplication aid, not an authorship judgement. A fork can add documentation, presets or build targets while retaining identical indexed native code. Conversely, a changed version of common infrastructure can produce a new fingerprint without creating a new instrument.

## Coverage accounting

| Measure | Count |
|---|---:|
| Repository candidates, including exclusions and unavailable entries | 1,215 |
| Explicit branch variants in addition to those candidates | 13 |
| Candidates with an available inspected tree | 1,197 |
| Candidates with collected representative production implementation | 1,048 |
| Repository/path/commit source references | 37,598 |
| Unique requested blobs verified | 11,619 |
| Requested blobs unavailable after collection | 0 |
| Exact indexed native-source equivalence groups with multiple repositories | 111 |
| Repositories in those multi-repository groups | 402 |

The screening dispositions below are automated working classifications, with primary-source review concentrated on the families discussed above. They make omissions visible; they are not a certified count of released, distinct or playable firmwares.

| Screening disposition | Candidates |
|---|---:|

| host, tool, documentation or data | 157 |

| resource or source-light candidate | 108 |

| unavailable or empty repository | 17 |

| platform, engine library or porting foundation | 62 |

| firmware or musical application candidate | 825 |

| game or non-music application | 14 |

| adjacent or unresolved relevance | 31 |

| excluded: GitHub topic URL, not a repository | 1 |

### Unavailable, empty and unresolved entries

These remain in the register with their original discovery provenance and errors rather than being silently dropped. An HTTP 404 does not prove a project never existed. Renames, aliases and inaccessible repositories require care.

| Candidate | Recorded problem |
|---|---|

| [28spawn/m-vave-pocket-amp-onyx](https://github.com/28spawn/m-vave-pocket-amp-onyx) | not found/inaccessible |

| [Felucca1027/felucca](https://github.com/Felucca1027/felucca) | empty Git repository |

| [friendsmakenoise-prog/fm1-pocket-sampler](https://github.com/friendsmakenoise-prog/fm1-pocket-sampler) | not found/inaccessible |

| [hdavid/fm1-emulator](https://github.com/hdavid/fm1-emulator) | tree unavailable or oversized response |

| [hfchfc1991/fm1-microtonic-inspired-fmtonic-](https://github.com/hfchfc1991/fm1-microtonic-inspired-fmtonic-) | empty Git repository |

| [JalopyMusic/fm1-milkboyg-ambii](https://github.com/JalopyMusic/fm1-milkboyg-ambii) | not found/inaccessible |

| [jeymadcat-ops/SLICE64-FM-web](https://github.com/jeymadcat-ops/SLICE64-FM-web) | not found/inaccessible |

| [jvitkauskas/fm1-bootloader](https://github.com/jvitkauskas/fm1-bootloader) | not found/inaccessible |

| [jvitkauskas/pi32v2-llvm](https://github.com/jvitkauskas/pi32v2-llvm) | tree unavailable or oversized response |

| [lzxnike-commits/firstattemptsFM-1-firmwares](https://github.com/lzxnike-commits/firstattemptsFM-1-firmwares) | empty Git repository |

| [mangogrape/Felucca](https://github.com/mangogrape/Felucca) | not found/inaccessible |

| [michaeljsmalley/fm1-linear](https://github.com/michaeljsmalley/fm1-linear) | empty Git repository |

| [pfkellogg/mac-usb-midi-bridge](https://github.com/pfkellogg/mac-usb-midi-bridge) | not found/inaccessible |

| [PierreArnaudGUYOT/FM1-Oktowave-Firmware](https://github.com/PierreArnaudGUYOT/FM1-Oktowave-Firmware) | not found/inaccessible |

| [stanleygurvich/fm1-firmware-database](https://github.com/stanleygurvich/fm1-firmware-database) | not found/inaccessible |

| [stanleygurvich/UplinkFeluccaEditor-WebApp](https://github.com/stanleygurvich/UplinkFeluccaEditor-WebApp) | tree unavailable or oversized response |

| [topics/m-vave](https://github.com/topics/m-vave) | not found/inaccessible |

| [xMagElonx/fm1-guitar](https://github.com/xMagElonx/fm1-guitar) | not found/inaccessible |

A small number of shallow/source-light or adjacent projects remain deliberately unresolved rather than being called finished instruments. The register is the complete candidate disposition list, including these cases. The survey is sufficient to choose what to inspect or adapt next; it does not establish universal compatibility or justify flashing every listed package.

## Files in the package

| File or directory | Contents |
|---|---|
| `Felucca-Firmware-Survey-2026-11-06.md` | This report. |
| `deliverables/Repository-Register.md` | Human-readable row for every candidate. |
| `deliverables/repository-inventory.json` / `.jsonl` | Repository records, provenance, commit pins, source paths, licences and evidence for documentation tags. |
| `deliverables/identical-source-groups.json` | Exact equivalence groups for indexed production native source paths/blobs. |
| `deliverables/ancestry-evidence.json` | Per-candidate exact source-sharing matches against Felucca, its engine library, BSP and fm1-base, plus contextual README evidence. Equality demonstrates sharing; it does not determine direct parentage. |
| `deliverables/survey-summary.json` | Reproducible coverage counts. |
| `deliverables/verification.json`, `verify_survey.py` | Offline source-integrity and inventory-accounting verification. |
| `audit/final-source-references.json` | Repository/ref/path to content-hash mapping, including branch variants. |
| `audit/blob-status.json` | Per-blob integrity/retrieval outcome. |
| `audit/trees/` | Inspected tree snapshots, additional branches and lookup errors. |
| `corpus/blobs/` | Complete collected text files addressed by Git blob hash. |
| `sources/` | Discovery catalogues and saved source evidence. |
| `extract_repo.py` | Reconstruct a selected surveyed subset from the corpus without network access. |
| `finish_survey.py`, `compile_survey.py`, `audit_ancestry.py`, `write_report.py` | Collection, screening, source-sharing audit and report-generation scripts. |

Example reconstruction, run from the unpacked package:

```bash
python extract_repo.py Rastapopolix/tessera --out extracted/tessera
python extract_repo.py fantasmoslam/fmini-1@smk37 --out extracted/fmini-smk37
```

The reconstructed files are the collected survey subset, not a complete build checkout. Submodules, assets, generated files and toolchains may still be needed to build an instrument. Original source licences apply.

## Decision supported by this survey

There is enough public material to avoid treating this as a new synthesis platform built from nothing. The best-evidenced reuse paths are the standalone Felucca engine library for compact DSP, Tessera/Tuna and the AMB family for composition, specialized instruments such as GHOULBOX/Melodee/FMIni for character, SLOOP/LOOM for sequencing, and the existing RemoteFM1/native-host approaches for instrument selection and hardware control.

That is the survey conclusion. Selecting a host, integrating engines and qualifying the S6 Lite, FM-1 and SMK-37 are subsequent work; this task did not start that integration.
