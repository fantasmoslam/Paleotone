> The published build described below is the earlier verified installation. For this integration, use its matching companion artifact; see [UNIFIED-SCAPE.md](UNIFIED-SCAPE.md).

# FM-1 device-controlled dual screens

This build swaps the instrument's control focus directly in firmware. Both pages stay visible; only the selected side responds to the FM-1's navigation and knobs.

## Install the matching build

Automated firmware, image, loader, UI and renderer tests passed. A physical-device trial is still pending. Keep your known-good firmware and the recovery method used previously before installing this experimental build.

1. Download [paleotone-device-controls.fwsc](https://fantasmoslam.github.io/Paleotone/webapp/screen-device/paleotone-device-controls.fwsc).
2. Install it from your computer using the existing FM-1 installer.
3. Open [the matching device-control page](https://fantasmoslam.github.io/Paleotone/webapp/screen-device/), connect the FM-1, and select **Expanded**.

## Play

- Hold **HOME**, tap **OCT+**, then release: control the right side.
- Hold **HOME**, tap **OCT−**, then release: control the left side.
- Navigate and edit with the FM-1's buttons and knobs. Each side remembers its own page, track, cursor, bank and drum lane.
- The right side starts on Mixer. Its page and track can also be assigned with the webpage selectors.
- The swap is latched: you can release the combo and keep playing. No computer keyboard or mouse action is needed for subsequent swaps.
- Tempo, transport and sound settings stay shared. Held musical notes are permitted during a swap; finish a held step edit, naming operation, dialog or layer before swapping.
- Original mode shows whichever page is currently active on the instrument. Switching the webpage to Original does not navigate the instrument to another page.
- Page assignments are retained during the session, not saved automatically to flash.

The original [screen companion](../screen/) remains available for the previous firmware. The two companion builds reject mismatched firmware instead of displaying an approximation.

## Exact build

Source: [5233e71702f81150f8a0d54566ff9f87c44c7b37](https://github.com/fantasmoslam/Paleotone/tree/5233e71702f81150f8a0d54566ff9f87c44c7b37)

Firmware SHA256:
`52020d278e3023b1dcf6ef840412679d595d1728a1fe267779208d560a015862`

[Firmware checks](https://github.com/fantasmoslam/Paleotone/actions/runs/40172598769) · [Companion checks](https://github.com/fantasmoslam/Paleotone/actions/runs/40172598766)

Licences: [GPL source](https://github.com/fantasmoslam/Paleotone/blob/5233e71702f81150f8a0d54566ff9f87c44c7b37/LICENSE), [licensing notes](https://github.com/fantasmoslam/Paleotone/blob/5233e71702f81150f8a0d54566ff9f87c44c7b37/LICENSING.md), [component licence texts](../../firmware/LICENSES/).
