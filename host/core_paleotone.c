/* SPDX-License-Identifier: GPL-3.0-only */
/* Run the unchanged Paleotone firmware through FM1VST's existing family adapter.
 * FM1CORE_FW_WEB_C is an absolute source path supplied by CMake, so this glue
 * does not depend on where the external FM1VST checkout lives. */
#define FM1CORE_FW_P pt
#define FM1CORE_FW_ANIM_IDLE 1
/* Paleotone has a different recovery guard from upstream Felucca. Hardware
 * recovery is not emulated by this application module. */
#define FM1CORE_FW_BOOTGUARD 0
#include "fw_core.h"
