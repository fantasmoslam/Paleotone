# Installer, browser instrument and display setup

The site hub links to `/webapp/installer/`, `/webapp/emu/` and `/webapp/app-setup/`.
The current PALEOTONE package is the hardware-tested compressed-display build:
source `1f68c416481860b568e41a85685edcf0cfefa1a7`, renderer `2540340739`, schema 2,
SHA-256 `26eafca0b34d109d79026d8bffa6a64188fc6c28ce11fd290097662d04d7d07e`.

`web/publish_release.py` assembles a complete emulator artifact, the tested package,
installer and read-only setup page. `release.json` records the exact package and
renderer identities. The installer verifies the package hash before enabling its
button; reload/resume keys are scoped to that package. An old main-branch website
build preserves this explicitly promoted release hub rather than rolling it back.

The setup page sends only the identity handshake and display-status query. It
does not capture snapshots, send panel input, read raw flash or start updates.
An FM-1_900 identity alone cannot distinguish custom firmware forks. Exact renderer
identities determine compatibility; known old PALEOTONE builds get an upgrade
link; unsupported builds receive an explanation of full firmware replacement.

## Adding display support to GHOULBOX

The first external integration targets official GHOULBOX 2.4, upstream source
`9f2b9a22e456d7fe9552b40f7a1d0c13200913bb`, official package SHA-256
`92630ae278d4a0b4233ff2512b62ad9b5dbe976a4c9083046222d9293a8df749`.
The source integration is isolated on `feature/ghoulbox-display`. It retains
GHOULBOX sound engines, presets, scenes, DREAM, theme and storage formats while
adding the state exporter, compression, physical focus lease and matched renderer.
SCENES selection state is included in the snapshot, and every firmware page is
checked against its native renderer. The original 16 KiB application reserve
and audio target budgets remain required.

`web/prepare_ghoulbox_release.mjs` uses the official package and an independently
built display-enabled application to create a pinned patch recipe. The browser
patcher accepts only the exact original package hash. It verifies the downloaded
app and exact output hash, reparses the package CRCs and retains the original
boot region and non-application package entries. An unknown file is rejected
before patching. Files are processed locally, with separate download and install
actions. Successful installation also checks the expected renderer after reboot.

The updater cannot read back arbitrary installed firmware. This tool patches a
known firmware **package** and reinstalls its app-compatible build; it does not
infer arbitrary code locations or inject into unknown installed firmware.
Further firmware support needs a separate source integration and exact recipe.
The GHOULBOX integration must remain labelled experimental until it has a real
device playing/listening trial.

## Validation

Run `node web/test_firmware_support.mjs`, `node web/test_firmware_patch.mjs` and
`node web/test_site_release.mjs SITE_DIR`. The patch test accepts optional original
package, app binary and recipe paths for full positive/corruption checks. The
companion CI additionally checks the emulator against the native audio build,
saved projects, all exact display pixels, stale replies, retries, USB resets and
queue backpressure. The GHOULBOX branch runs upstream host/golden/health tests and
its own target image/OTA/loader/CPU checks before generating a releasable package.

## SCAPE redesign candidate

The isolated preview is `/webapp/scape-preview/`, with its own `installer/`, `screen/`, `editor/` and `manual/`. It does not replace the release manifest or the existing companion. Browser saves use the `paleotone-scape-preview-` prefix.

Firmware/renderer source: `2ecc0a3827e90e45638ad74bf7690b2e82543a1b`. Renderer `3439484896`, schema 2; package SHA-256 `ed82093de042239665b4bc1ef4da4fc0359bd2951ee684cd0708b9d53b773332`. Target image 565692 bytes, data+BSS 90340/98304, pool 332900/344064. The configured 15872-byte application reserve is unchanged by the UI work. This UI candidate has not been flashed to the physical FM-1.

Firmware run `40263805878`, companion/publish run `40263805917` and full host run `40263805879` passed. Both UI variants have zero layout findings; all 213 audio goldens are unchanged. The candidate workflow verifies package hash, native/WASM identity and complete local assets before publishing only the isolated preview folder. GitHub Pages deployment `40264771557` succeeded.

Rebuild staging with `python web/prepare_scape_preview.py EMULATOR PACKAGE OUT SOURCE`, then run `node web/test_scape_preview.mjs OUT` and `node web/test_published_companion.mjs OUT/screen`. Only publish that output inside the dedicated preview folder. Use its paired display after installing this candidate.
