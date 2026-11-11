# NightClaw identity

NightClaw is the new public name of the firmware formerly called T-WRECKS. The splash title, main-page branding, screen-reader name, normal USB manufacturer/product strings, crash/safe-mode text, README and quick-start guide use the new name. The technical update identity remains FM-1_942 and the saved-preset format is unchanged, preserving settings and installer compatibility. Existing branch and historical document paths remain stable.

## Artwork

The new original generated artwork emphasizes a moonlit prehistoric jungle: canopy, ferns, ravine, mist and distant ridges, with a small raptor in the landscape. The full artwork is [nightclaw-jungle.png](assets/nightclaw-jungle.png).

The LCD uses an 88 x 88 monochrome version scaled to its 240 x 240 screen, with NIGHTCLAW above and the instrument subtitle below. Its packed pixel data occupies 760 bytes. The checked-in include is the canonical build input; regenerate it explicitly with python3 tools/gen_nightclaw_splash.py when changing the source artwork. The encoder asserts a lossless packed-data round trip. No extra audio RAM or steady-state drawing is used.

## Title duration

The complete title screen remains visible for at least 3000 ms. Timing starts after lcd_sync confirms the full image transfer. The firmware then services the watchdog while audio and TIMER5/USB interrupts continue. The title is static and cannot be skipped by ordinary UI input during the hold. Controls open afterward.

## Validation and installation

Native image: 581,532 / 581,564 bytes. RAM and pool remain within their enforced limits. The actual simulator splash has been visually checked; the twenty approved production audio hashes remain unchanged, and USB audio, updater/loader and installer checks pass. Reader and production control scenarios pass.

This candidate is prepared but not installed. Dylan chose to save current RED MESA edits later; the working device firmware is being left running. Save those edits before the later firmware restart. Existing sounds, twenty scene presets and two-core audio behavior have not been altered by the rename.
