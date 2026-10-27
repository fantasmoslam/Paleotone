# Next synthesis hot-path audit

Installed build remains USB queue source 0765942, renderer 696410948. No further synthesis changes were installed.

Target disassembly, not host compiler assumptions: gurdy_render still calls osc_saw at four sample-loop sites, osc_sine at one, soft_knee and voice_amp at output. saurus_render still calls osc_saw, noise32 in breath/period paths, soft_knee and voice_amp. Its 476-byte stack frame also suggests register pressure worth tracking. Static call sites do not establish runtime cost; conditional paths and source settings matter.

The prior coefficient cache was slower and remains rejected. A useful isolated next candidate would expose an exact always-inline saw/blep helper only to ROAR, leaving compact wrappers for other engines. Preserve unsigned phase wrapping, integer division and sample rounding exactly. Avoid shared global helper changes or approximate reciprocal division until differential equivalence and target code size are understood.

Before device installation: target disassembly must confirm the intended calls disappear without budget, RAM or stack guard regressions; exact-source sound/worker/race and companion checks must pass. Then compare the same scene using core OFF for per-track profiles (track profiling is disabled with worker ON), and core ON for end-to-end deadlines. Keep USB queue targets fixed. Extra flash and increased register spills can erase inlining gains, so reject if hardware does not improve.
