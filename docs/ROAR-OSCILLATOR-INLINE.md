# ROAR oscillator call experiment

Base81bae2a retains verified MIDI dispatch fix, USB queue margin and optional second core. Local always-inline saw helper copies the existing osc_saw/blep arithmetic, including unsigned phase wrap, division and branch precedence. Only four ROAR call sites change; SAURUS and all other shared helpers remain untouched. No cache, allocation, table, state, RNG, voice limit or approximation change.

Actual production functions are extracted and compiled with their attributes intact at -Os and -O2 for boundary/phase and1000000 randomized comparisons. Existing sound, ROAR corner, concurrent/race and full host checks must pass. Target disassembly must confirm calls removed without exceeding unchanged budgets; hardware must show a real gain before adoption. More code can increase spills or instruction-cache pressure, so reject if it does not help.
