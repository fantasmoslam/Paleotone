# NightClaw for the M-VAVE FM-1

![NightClaw — a raptor within a moonlit prehistoric jungle](docs/assets/nightclaw-jungle.png)

A generative dinosaur synth with resonant taiko drums, western/tribal scenes, selectable instruments and playable creature calls. RAPTOR NIGHT and six western scenes are the approved musical reference. The guarded two-core audio candidate plays the previously overloading Lead test cleanly on the tested FM-1.

**[Quick start](docs/TWRECKS-QUICKSTART.md)** · **[Current features](TWRECKS.md)** · **[Completion checks](docs/TWRECKS-COMPLETION-CHECKLIST.md)** · **[Development PR](https://github.com/fantasmoslam/Paleotone/pull/41)**

## Play

Turn PRESETS to choose a scene. On Home, SELECT chooses a track; ALGORITHM chooses its instrument, or the beat when Skin is selected. The four right encoders edit that row. Play the Lead on the white keys, or select Calls for manual creatures.

The twenty-scene bank includes BONE RIDER, DUST TRAIL, RED MESA, FOSSIL PASS, HIGH FRONTIER and NIGHT RIDER. Fourteen drum patterns and the melodic voices are independently selectable and saved per scene. Cello, Cello Low, Sub Pad and Fat Pad are the last four instruments on each melodic track.

**Saving:** hold GLO for about half a second and release. A short GLO tap opens Setup; holding about 1.5 seconds resets the current scene. The physical SAVE button opens sound pages.

## Candidate installation and build

The installed NightClaw candidate is source 3cf8a1a, product identity FM-1_942, including the [jungle title screen with a three-second minimum hold](docs/NIGHTCLAW-BRANDING.md). It retains the twenty approved audio baselines and guarded two-core worker. This is a development candidate; [the audio headroom record](docs/TWRECKS-AUDIO-HEADROOM.md) distinguishes measurements from wider release coverage.

The [NightClaw package](https://github.com/fantasmoslam/Paleotone/blob/77f1b86224ea55fd076eb4df496e70d455b234c9/nightclaw-FM-1_942.fwsc) passed [full CI](https://github.com/fantasmoslam/Paleotone/actions/runs/43092611149). The local installed package SHA-256 is beb6ce14e4b50ef44270442c312e750abb8d6aa49f02ed1c6a72cdb59671586e.

CI publishes this branch's package and logs to [twrecks-completion-output](https://github.com/fantasmoslam/Paleotone/tree/twrecks-completion-output). Use its RESULT file to identify the built source and successful check status. Install its nightclaw-FM-1_942.fwsc with the project's trusted fm1_install.py USB-MIDI installer. The checksum is published beside the package. [BUILDING.md](BUILDING.md) describes the compiler and SDK setup.

## Source and credits

NightClaw builds on NACHTFAHRT/Felucca and Paleotone's synthesis work. The Paleotone/X0X/Melodee-derived guarded worker and original copyright notices remain in their source files. Firmware code follows the repository's GPL notices; the recorded taiko assets have separate [CC BY-SA 4.0 attribution](tools/research/taiko-bank-05/ATTRIBUTION.md).

The [upstream README](UPSTREAM-NACHTFAHRT-README.md) is retained as a historical reference; its installer, website and musical description describe NACHTFAHRT.
