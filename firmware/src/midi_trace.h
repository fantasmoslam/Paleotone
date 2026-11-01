/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef PALEOTONE_MIDI_TRACE_H
#define PALEOTONE_MIDI_TRACE_H
#include <stdint.h>
/* Diagnostic counters, no resets during operation. Each field has one writer:
 * RX fields in USB ISR; handling/TX fields in foreground. Serial reads are
 * individually atomic 32-bit observations, not one coherent multi-field snapshot. */
static struct {
 volatile uint32_t rx_start, rx_busy, rx_done, rx_cmd, rx_track;
 volatile uint32_t handle29, handle96, tx29, done29, tx96, done96;
 volatile uint32_t tx_timeout, tx_fail_cmd;
} midi_trace;
#endif
