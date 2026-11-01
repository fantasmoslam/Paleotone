# USB queue margin experiment

Based on engine hotpath source 3da5576. Change only USB priming and fill-control targets; retain the 512-frame ring, packet descriptors, sample equations, resampling, I2S buffer length, worker policy and rate controller.

44.1 kHz: prime 104 -> 232, low 80 -> 208, high 128 -> 256 frames. Added target wait: 128/44100 = 2.9025 ms. 48 kHz targets are the new values scaled by 160/147 and rounded up: 253/227/279. Their added wait is approximately 2.9 ms. No ring allocation or stack size change. The existing capacity assertions still require room for a render and a maximum packet at both rates.

This is USB queue latency, not measured total key-to-DAW latency. Host capture scheduling, driver buffering, analog output and musical note onset are separate. The host test timestamps each source frame at publication and measures its actual queue wait using the real producer/consumer code, both normally and through deterministic 4 ms producer pauses. It compares the previous source under the same schedule. Existing UAC coverage additionally verifies both descriptor configurations, 44.1/48 kHz stream order and clock drift, underrun/overrun handling, stop/start and mid-stream rate switching.

Hardware gate: exact-source target, host/concurrency/race and matching companion checks pass before installation. Fresh backup, verify core ON, repeat same 84 BPM loaded scene with asserted playback, compare interval counters and 30-second USB capture. Preserve private backup objects, restore prior package if the tradeoff fails. Queue improvement cannot establish an I2S synthesis deadline fix. Keep as a draft until hardware results exist.
