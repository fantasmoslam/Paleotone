# SM-T510 kiosk setup — Android alpha 0.1.2

The APK update preserves the 0.1.1 audio engine, output level and session format.
Installing it alone does not enable a lock or change the Home app.

## Install and prove the exit menu before enabling

Save your current session, download the APK to Windows Downloads, then:

```powershell
.\adb.exe -s 192.168.0.46:33862 install -r "$env:USERPROFILE\Downloads\Paleotone-Android-0.1.2-alpha.apk"
.\adb.exe -s 192.168.0.46:33862 shell am start -n org.paleotone.synth/.MainActivity
```

Hold the PALEOTONE title for **three seconds**, without moving your finger, to
open the kiosk/maintenance menu. HELP also has a **Kiosk / maintenance** button.
Test that you can open this menu before provisioning.

## Provision device owner

This gives Paleotone Android device-policy authority for its kiosk. The app uses
that authority only after you choose Enable kiosk. It installs no management
server and does not reset the tablet. ADB development provisioning requires a
device without accounts or an existing device owner; if Android refuses, keep
ordinary app mode and inspect the exact error instead of wiping or forcing setup.

```powershell
.\adb.exe -s 192.168.0.46:33862 shell dpm set-device-owner org.paleotone.synth/.KioskAdminReceiver
```

Wait for `Success`. If it reports an existing owner, account/setup limitation or
another error, do not proceed as though setup succeeded. Leave wireless debugging
available during kiosk testing. Its connection port may change after a reboot.

## Enter kiosk

Return to the app, hold PALEOTONE for three seconds, then select **Enable kiosk /
return to instrument**. This explicitly:

1. Allowlists only Paleotone for lock-task mode.
2. Leaves global power actions available for reboot/power off.
3. Sets Paleotone as the persistent preferred Home app.
4. Saves the enabled state and enters lock-task mode while foreground and unlocked.

Home launches Paleotone on subsequent normal device boots; a keyguard, if later
configured, still requires unlocking. The app does not bypass Android's lock screen.
The app never falls back silently to ordinary screen pinning when not device owner.
Wi-Fi, ADB, media-volume controls, thermal management and other system services are
not disabled or stripped. This is a personal instrument mode, not a secure public kiosk.

## Exit for maintenance

Hold PALEOTONE for three seconds or open HELP → Kiosk / maintenance. Choose
**Exit kiosk and open Android settings**. The app first persists kiosk disabled,
then stops lock task, clears the allowlist and clears its persistent Home policy.
Maintenance remains unlocked across app resumes/restarts until explicitly enabled
again. Returning to Paleotone does not automatically leave maintenance mode.

**Keep kiosk disabled** does the same policy release without opening Settings.
**Remove device management** additionally clears Paleotone's device-owner role
after a confirmation; it does not delete sessions or uninstall the app.

If the app was separately chosen as the default Home app through Android Settings,
that user choice is distinct from the DPC's persistent Home policy. Use Settings →
Apps → Default apps → Home app to choose the Lineage launcher if necessary.

## ADB emergency escape

The explicit receiver requires Android's `DUMP` permission, held by the ADB shell.
It is not a normal exported no-permission bypass. While ADB remains connected:

```powershell
.\adb.exe -s 192.168.0.46:33862 shell am broadcast -n org.paleotone.synth/.MaintenanceReceiver
.\adb.exe -s 192.168.0.46:33862 shell am start -a android.settings.SETTINGS
```

Expected receiver result: `Kiosk disabled; Home policy cleared`. It does not need
the MainActivity to be working or an audio stream to open. If an OEM denies shell
permission or an operation fails, report the exact output; the physical-device
escape has not been certified merely by host tests.

## Acceptance still needed on the SM-T510

- Before owner setup, title hold and HELP both open maintenance; no spontaneous pinning.
- Device owner provisions successfully without a reset.
- Explicit enable locks Home/Recents/notification navigation and leaves volume usable.
- The title-hold exit works in actual lock-task mode, and Android Settings opens.
- Returning to the synth while in maintenance does not relock.
- Emergency ADB broadcast clears lock/Home policy while the app is locked.
- Enable again, reboot, and verify the Home app starts and locks after unlock.
- Audio, session persistence and headphone recovery retain the accepted behavior.

Automated coverage includes actual KioskSession policy ordering and failure injection
at allowlist, Home, enabled-state and lock operations, plus cleanup if unlock or state
persistence fails. Android-specific DPM permissions, lock-task UI and boot behavior
still require the above physical acceptance; no emulator/device-owner test was run.

Primary platform references:
- https://developer.android.com/work/dpc/dedicated-devices/lock-task-mode
- https://developer.android.com/work/dpc/dedicated-devices/cookbook
