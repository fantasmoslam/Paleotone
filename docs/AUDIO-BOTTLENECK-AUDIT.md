# Audio bottleneck checklist: current firmware findings

Applies to diagnostics source c564f667fe54d051eb0158e54b86ba319e814c60 and exact-arithmetic candidate 8fba4c6036f5c201d8eeb81a5d90730e5ee7a6c8. This is a source audit plus measured hardware evidence, not a claim that all contention sources have been excluded.

| Proposed check | What applies here | Evidence / next action |
| --- | --- | --- |
| RTOS task priority/starvation | This application uses a bare-metal foreground loop and hardware interrupt handlers; there is no RTOS task scheduler in the inspected firmware. Foreground UI/editor work can still be delayed. | `firmware/src/main.c`: ALNK0 audio priority 3; TIMER5 input/USB service priority 4. USB MIDI/UART polling is deferred while audio runs; USB audio consumer service can run nested. Measure service latency if MIDI responsiveness remains a problem after render headroom improves. |
| Audio interrupt execution time | Confirmed problem under the loaded scene. Rendering itself can exceed the available deadline. | `firmware/src/audio.c`: measured render 3065 us and wall 3295 us versus 2902 us deadline. Nested maximum 247 us is separately maintained and must not be added to the other maxima. The master/effects costs are substantial even idle. Candidate PR #13 targets master helper overhead without changing its arithmetic. |
| SPI/display blocking | Real foreground waiting exists. No LCD timeout observed; that does not prove display cost or DMA contention is harmless. | `firmware/hal/fm1_lcd_hw.h` uses SPI DMA and polls completion, counting a bounded wait timeout. `firmware/src/lcd.c` documents default BAUD 4 as 12 MHz. Audio interrupts can preempt foreground waiting, but shared memory/bus/cache effects remain candidates. A display-workload A/B test is still needed to quantify them. |
| BLE MIDI overhead | BLE is not part of the current application build inspected here. | No BLE stack or BLE MIDI traffic path was found in these firmware sources. USB MIDI and optional UART MIDI are the relevant paths. Do not treat the hardware's wireless capability as an enabled background load. |
| Heap fragmentation/compaction | No dynamic allocator calls were found in the firmware source; audio uses fixed storage/pools. Dynamic heap compaction is not a supported explanation for these measurements. | Static RAM is tight (96136/98304), with a separate pool of 332900/344064, so capacity and stack safety still matter. Preserve the second-core reserve and existing guards. Memory-race and persistence checks remain part of the software gate. |
| External-flash/cache latency | Plausible contributor; not isolated by the current counters. | `firmware/hal/fm1_xip.h` explicitly describes the SFC/cache XIP window. Target disassembly places master code and the clipping table in that window. Removing small repeated calls can reduce overhead, but only hardware timing establishes benefit. Moving code/tables into RAM needs explicit size, ownership and concurrency analysis; there is no blanket RAM relocation in this candidate. |

## USB audio observation

The producer ring reached zero raw frames and producer publication gaps reached roughly 4 ms during verified loaded playback. Repeated eight-second captures produced different underrun totals (16 then 293), so one short capture is not a stable performance benchmark. Compare repeated phase costs, deadline counters, and capture intervals together. Increasing buffers alone would not repair the independently observed I2S render deadline violations.

## Scope

No clock, voltage, interrupt priority, buffer, overload threshold, voice count, reserve or synthesis behavior was changed for this audit. The independent SCAPE redesign remains later work; the current goal is measured headroom with preserved sound and saved projects.
