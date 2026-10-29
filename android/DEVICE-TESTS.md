# SM-T510 hardware record — October 28, 2026 (America/Phoenix)

Tester: Dylan Spaniel / Fantasmoslam. These are user-reported observations, not
instrumented latency, power or complete device-acceptance measurements.

## Android 0.1.0-alpha, installed development-signed APK

Device: Samsung SM-T510, Android 15, unofficial LineageOS 22.2-20260910.
Native source: `ac60499e651ac93889bdbe1e67e960e1297a73af`.

| Behavior | Observed result |
| --- | --- |
| App launches / native sound | Keys play and sound good |
| Multitouch | Five fingers dragged rapidly across keys; no noticed stutter |
| Key highlighting | Working |
| Firmware screen | Present and working |
| Sequencer tracking | Reported spot on |
| Save session, close, reopen | Changed sound returns |
| Speakers and wired headphones at maximum Android/app volume | Output subjectively too quiet |
| Plugging in headphone cable | Playback stops; manual start restores it |

The sustained 10–15 minute soak was suggested but no completed soak result was
reported. External MIDI controllers, physical latency and numeric underrun data
have not been reported as tested. Do not convert subjective clean playing into
a claim of measured zero underruns or certified low latency.

## 0.1.1-alpha changes awaiting device acceptance

The native adapter retains the portable renderer's normalization of the FM-1's
24-bit codec output. That firmware path deliberately leaves 6 dB of reserve
(`OUT_SHIFT=7`), so the Android output now defaults to 2x gain (+6.02 dB). AUDIO
offers original 1x or boost 4x as alternatives. Gain transitions are smoothed;
an instantaneous-attack stereo-linked limiter with 50 ms recovery bounds digital
output at 0.98. This is a source-level correction and bounded-output test, not
a measurement of the tablet's analog loudness.

AAudio errors are consumed by the foreground Activity's polling loop. The Activity
closes the old stream on its lifecycle thread, waits a short route-settling interval,
and retries up to five opens with backoff. An output-device event can rearm a pending recovery;
after five failed opens the user can tap START to retry. Manual Stop, backgrounding and lost audio focus
cancel retries. Synth/project state is not rebooted. Save and buffer changes
preserve explicit paused intent.

Verify the signed update preserves the saved project, is louder on the same patch
at the same master/media settings, remains clean with dense playing, and resumes
after both plugging and unplugging wired headphones without a manual start.
Also verify pressing STOP during recovery prevents later automatic playback.

## 0.1.1-alpha hardware follow-up

Dylan reports a slight jitter on unplugging headphones, followed by immediate
automatic recovery. Speaker volume is reasonable for the tablet; headphones are
great and he would not want them louder. Output gain is accepted; do not increase
it further by default. The brief route-transition jitter remains a polish item.

## 0.1.2-alpha kiosk boundary

Adds optional Home/device-owner lock task and maintenance/ADB escape paths. None of
the kiosk provisioning, actual lock-task exit, emergency broadcast or boot behavior
has yet been reported as device-tested. See KIOSK.md. Audio DSP and output gain are
unchanged from the accepted 0.1.1 build.
