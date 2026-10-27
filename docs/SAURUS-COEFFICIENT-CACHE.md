# SAURUS exact-input coefficient cache

Based on hardware-tested engine optimization 3da5576a9971191eef44d027f2cc1727203eb9f6 (renderer 2162762987). The current loaded BRACHIO CALL still averages about 110 us per 32-frame block.

The three resonators per throat retain their coefficients and the exact clamped frequency/width inputs used to compute them. An exact key match skips only the pure saur_res calculation; filter samples, state, rounding, random draws/order, calls, modulations and transitions retain their current behavior. A zero frequency invalidates the key because valid inputs are at least 30*16. Existing whole-slot initialization clears keys and coefficients on new ownership; retriggers may reuse matching inputs.

Memory plan: 60 extra bytes per throat, three throats in four slots = 720 bytes. This is fixed storage, not a heap. The target RAM/stack/pool gates must confirm the final layout. Do not consume the second-core reserve or change clocks, buffers, voice limits, overload thresholds, or core exclusions.

The new differential lifecycle test compares mixed audio and shared RNG evolution with caching versus invalidating all frequency keys before every block. It covers every call type, live size/mouth/call changes, sustained notes, same-key retriggers, four-track slot pressure/stealing, note release and slot reuse. Existing golden, health, UI/persistence, threaded/race and target budgets remain unchanged.

After all exact-source software gates and the matching companion pass: fresh consistent backup and guarded install, same scene at 84 BPM with core OFF, actual-playing assertions, phase timings and at least 30 seconds of USB capture; compare all 13 saved objects, then stop playback and profiling. Keep the current engine package (0899f068b59cf46783e0a0fe96b6636aa82abe37de94f65114213df1568ec607) for rollback.

No coefficient hit rate is a speedup percentage. The previous held-note research (98-99% consecutive matching inputs) is not the live workload or a measured performance gain. Hardware timing is required.
