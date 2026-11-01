# Exact-arithmetic Hall helper candidate

Based on installed master optimization source 8fba4c6036f5c201d8eeb81a5d90730e5ee7a6c8. Force inline only hl_mul, retaining its exact multiply, sign-dependent rounding correction and shift. Hall state, line order, clamping, modulation, coefficients and routing remain unchanged. No clocks, buffers, reserve or overload settings change.

Target assembly previously calls hl_mul directly eight times per sample plus twice inside every allpass helper. Check the unchanged target budgets, sound goldens, concurrent/race/persistence/UI tests and exact-source companion before installing. Compare hardware effects stage against approximately 139 us per 32-frame block on the installed master candidate. A runtime speedup is not established by source edits or static counts alone.
