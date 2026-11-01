# Exact-arithmetic master hot-path candidate

Based on diagnostics source c564f667fe54d051eb0158e54b86ba319e814c60, currently installed on the test FM-1. The measured master/scope stage averaged about 90 us per 32-frame block in the loaded scene.

Changes: force-inline dc_block and lowcut1, and expose an always-inline softclip implementation to the master while retaining the existing compact softclip wrapper for other callers. Equations, state update order, rounding, clipping table, limiter, tape, effects, clocks, voices and overload thresholds are unchanged. Target assembly previously had two dc_block, up to four lowcut1 and two softclip calls per output frame. The candidate aims to remove that call/argument/spill overhead; a hardware speedup is not yet established.

Gates: unchanged golden/health/CPU budgets, target/image/updater/loader checks, full host/race tests and matching companion. Compare master phase timings and audio/USB deadline behavior on the same loaded scene before claiming benefit. Recheck profiling OFF/ON overhead. Keep the current diagnostics package and pre-install backup for rollback.
