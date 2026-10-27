# Audio diagnostics installation and interrupted hardware session
Recorded 2026-10-26 America/Phoenix (2026-10-27 UTC).

## Installed candidate
Source: c564f667fe54d051eb0158e54b86ba319e814c60
Branch: feature/audio-diagnostics; draft PR #12.
Package SHA256: 6ef842944829f6e3a7b38e5fb4616369eaffbe4456bbdfdfe13b0f359dd59962
Package size: 609905 bytes.
Verified device: FM-1_900, renderer 3998626645, schema 2, globals 82.
Fresh 13-object backup was captured by the guarded installer before transfer.
Install and reboot identity verified at 2026-10-27T00:12:38.116Z.

Exact-source gates passed:
- Target/image/updater/loader/CPU budgets: run 40422710355.
- Full host, profiling counter tests, real threaded mixer, worker-active sound regression, ThreadSanitizer and existing single-core suite: run 40422710443.
- Pinned matching companion: run 40422790870.

Target image 548420 B; static RAM 96136/98304 B; pool 332900/344064 B.
The matching companion was prepared and validated locally, but NOT published.
The currently published screen-dualcore page has the PREVIOUS renderer 503813915 and must not be presented as matching this newly installed firmware. Use the exact new artifact or publish its prepared screen directory before browser testing.

## Hardware status and invalid playback phases
The first profiling harness completed but every serial status, including phases named playingProfile and usbProfile, reported playing=0. Its PLAY request did not establish running transport. It failed to assert playing=1 after the tap. Those phases must NOT be used as loaded-song evidence. Cause of failure to start was not established; the attempt to read UI context and crash status was blocked when the user's connected computer environment became unavailable.

Last verified final state:
- playing 0.
- cpu2_set 0 and cpu2 0.
- profile off returned profile_on 0.
- No new project/note edits were sent by this profiling harness.
- No automated listening assessment was made.
- boots=2; audio_late=54 remained unchanged through the entire stopped run. This cumulative value is not evidence of 54 new overruns during the test.

Valid stopped-transport measurements:
- Idle CPU about 36–37%.
- 4324 idle profiling calls: aggregate parts 112292 us; events 49289 us; ambient/SCAPE 11560 us; effects 599994 us; master/scope 385620 us; USB copy 247 us.
- Individual track rows overlap the aggregate parts row and are not additive.
- Effects/master dominate the measured stopped-load groups, but this does not identify loaded-song costs.
- Stopped USB capture completed, zero underruns/overruns/missed packets/stalls; raw ring fill minimum 98 frames; maximum 44.1-kHz producer-publication gap 2084 us.
- LCD timeouts 0 in sampled statuses.
- Max audio wall time 2067 us and nested interrupt maximum 124 us are separate cumulative maxima, not necessarily the same block.

## Local recovery/session locations
Workspace root: C:\Users\Dylan\Documents\Codex\2026-10-25\new-chat-9
Diagnostics checkout: work\paleotone-audio-diagnostics
Diagnostics session: hardware\trial-1793058919816
Key files: installation-verified.json, before-ui-backup.json, hardware-profile-results.json, install-diagnostics.mjs, run-profile.mjs, read-diagnostics.ps1, artifact\paleotone.fwsc, screen\build.json, screen\felucca.wasm.
Previous installed candidate and earlier backups: work\paleotone-next\hardware\trial-1793054758596
Previous rollback package SHA256: 8d569b4cd471ae97a77f655cb4d927ae412cd88a0063b509b675b304f752f737 (source 21404aa, renderer 503813915).
Do not run an older identity-pinned installer against this new device by mistake.

## Resume
1. Restore access to the same connected home computer; enumerate MIDI and serial again, then verify new renderer and settings.
2. Read actual UI context/crash status and determine why PLAY did not start.
3. Fix the test harness to assert playing=1 immediately after start and before/after every loaded phase. Mark/abort any phase with wrong transport state.
4. Preserve the latest backup. Rerun profile OFF/ON overhead comparison, then actual playing profiles with and without USB capture.
5. Publish the prepared matching companion and preserve its directory in site deployment; do not overwrite the old matching page.
6. Compare measured track/events/SCAPE/effects/master work, total wall/nested time and USB raw fill/feed gaps. Do not optimize from the mislabeled stopped phases.
7. Keep clocks, overload thresholds and voice limits unchanged until the actual bottleneck is established.
