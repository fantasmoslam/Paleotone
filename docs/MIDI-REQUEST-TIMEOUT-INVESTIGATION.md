# MIDI request timeout investigation

Installed firmware unchanged: USB queue source 0765942f34a1b2818b5f9af2ce1d8242c32bf5ca, renderer 696410948. Core ON; same 84 BPM scene. No synthesis or transport code changed.

Two additional 60-second unprofiled USB recordings used command-level request logging. Only one request was outstanding. Both commands were screen-context command 96 operation 0 and track-dump command 29 cycling four tracks. Note: command 96 operation 0 also disables companion focus; it is not a strictly read-only operation, although no music data is edited.

| Run | Completed pairs | Timeout exceptions | USB underruns |
| --- | ---: | ---: | ---: |
| Original unlogged stress | 483 | 7 | 0 |
| Logged reproduction | 521 | 4 | 0 |
| 25 ms delay before each request | 492 | 3 | 0 |

Logged reproduction: three track-dump timeouts, one command-96 timeout; successful per-command maxima 52 / 56 ms. Paced run: one track-dump timeout, two command-96 timeouts; successful maxima 87 / 85 ms including the artificial 25 ms wait. Exceptions are 1200 ms request timeouts, not connection-busy or closed errors. Longer timeout has not been tested; response loss versus delayed/partial delivery is not yet distinguished.

Pacing did NOT eliminate the failures. These short live windows do not establish a statistically meaningful improvement from 4 to 3. Do not ship pacing as a fix or attribute the problem solely to track-dump size.

Code audit: track dump replies are 235-byte SysEx frames (observed), larger than the 64 USB-MIDI-event output ring's 192 payload-byte capacity. ota_wire_send can wait for ring drain, with a 200 ms send timeout. Standard commands >=11 are handled in place before ota_frame_done releases the single input frame, whereas remote commands 92..96 use a separate queue. Incoming SysEx starts set sx_collect=!sx_ready. This is a plausible request-loss window, not a demonstrated cause. Native WinMM SysEx fragmentation, LONGERROR events and firmware send completion also require instrumentation before changing firmware.

Next discriminating tests: log native fragment lengths/LONGERROR and discarded partial frames; compare stopped/playback/USB-capture cases with identical request counts; separately exercise each command and correlate successful replies with requests. Add firmware send/drop counters only if native evidence is insufficient. Do not hide misses through retries or raise deadlines and call it fixed.

Evidence in work/paleotone-usb-buffer/hardware: midi-diagnostic-1793089395443 and midi-paced-1793089531343. JSON results preserve exact command/track/error/timing; JSONL logs preserve send/receive timing. Captures complete successfully. Paced surrounding interval ~65.9 seconds: five late blocks, 46 voices shed, zero USB underruns. No worker faults/fallbacks.

Fresh final backup matches all 13 objects from pre-diagnostic state in size, CRC and encoded data. Playback stopped, profiling OFF, second core ON; same firmware retained. Root cause remains unresolved.
