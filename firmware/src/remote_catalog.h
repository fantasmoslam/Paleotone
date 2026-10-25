/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Browser-only project summaries. No raw pointers cross the MIDI connection. */
typedef struct {
    uint8_t used;
    char name[13];
    uint16_t len[4];
    uint32_t density[4];
    uint8_t cells[4];
} remote_project_t;
#if FELUCCA_REMOTE
static uint8_t remote_display;
static remote_project_t remote_projects[4];
#endif
