# NightClaw PC 0.1.0 — Windows development build

Extract the ZIP into a folder, then open **NightClaw.exe**. Keep the DLL and configuration file beside it. Windows x64 and .NET Framework 4.8 are required. No browser, FM-1, MIDI device or network connection is needed.

The application opens stopped. Choose a scene and press Play. Rest stops the generative transport while tails fade; Silence stops notes and effects. In Scene & effects choose the Windows output device and adjust Master.

## Instrument

Select a track on the left; choose its instrument/beat and use the large sliders. Attack, Decay, Sustain and Release accept **Voice** at their far-left position, meaning the instrument's original setting. Skin retains its percussion decay model rather than a held-note envelope. The bottom eight numbered pads play Lead: press and hold. The eight creature buttons trigger calls.

All twenty scenes, fourteen beats, original voices and the appended cello/bass-pad instruments come from the existing NightClaw engine. This build retains the approved scenes and serial desktop synthesis. It does not update or alter FM-1 firmware.

## First PC composition expansion

Open Composer and enable **Expanded composer**. The app plans 32 bars ahead as four eight-bar phrases: Call, Answer, Develop and Resolve. The plot shows the actual planned notes. The last phrase closes on the scene tonic. Notes follow the current chord within the scene scale; mouse-played Lead takes priority. Enabling during playback waits for a bar boundary.

- **Variation:** how extensively the development phrase transforms the motif, including modest syncopation above 50.
- **Space:** how often optional notes are omitted. Higher leaves more room.
- **Theme reuse:** chance of drawing from the remembered bank (up to eight motifs) for the next plan.
- **Lead register:** shifts planned notes by up to two octaves.
- **Seed:** reproducible starting material. Changing it clears the theme bank.
- **Develop next plan:** prepares and starts a new passage immediately.

Variation, Space and Theme reuse affect newly prepared plans; register changes affect subsequent notes. Disable Expanded composer to return to the existing NightClaw generator. The original Lead In/Bridge/Arc/Memory settings remain in Scene & effects; Lead In and Memory govern the original generator, not the PC planner.

This is the first PC planner, with one automatic Lead. Separate responsive instrumental composers, advanced harmony/phrase evaluation, recording/export and richer samples are subsequent work. MIDI and remote control remain deferred as requested.

## Projects

Save or Save as writes a **.nightclaw** file containing all twenty scenes, beat/voice/mix/effects/ADSR settings, master level, seed, planner settings and theme bank. Writes use a temporary file and atomic replacement. Open validates the snapshot before applying it and opens stopped.

The saved format is versioned for this development build. Projects preserve musical state and planned material, not a sample-exact playback cursor. Scene edits persist in memory while switching scenes; save before closing. Generated themes evolve while playing, so save again to retain the latest bank.

## Build

Native engine: WSL with gcc-mingw-w64-x86-64-posix, then run sh desktop/build-engine.sh. Windows interface: run desktop/build.ps1 from PowerShell. The compiler ships with the Windows .NET Framework. Only Windows system DLLs are runtime dependencies.

Run NightClaw.exe --self-test OUTPUT_DIRECTORY for native scene/audio, planner, project and muted WASAPI lifecycle checks. NightClaw.exe --smoke OUTPUT.png renders actual instrument and composer forms for layout inspection.

GPL-3.0 source and original synthesis credits remain in the repository. Taiko recordings carry separate CC BY-SA 4.0 attribution included in the ZIP.
