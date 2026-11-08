/* SPDX-License-Identifier: GPL-3.0-only */
/* The FM-1's flash allow-list (hal/fm1_flash_map.h FL_JEDEC_OK), shared by the app and the update loader:
 * the Puya P25Q80 and the UC25HQ80 of newer units, nothing else. */
#include <stdint.h>
#include <stdio.h>
#include "fm1_flash_map.h"

int main(void)
{
    int ok = FL_JEDEC_OK(0x856014u) && FL_JEDEC_OK(0xB36014u) && !FL_JEDEC_OK(0u) && !FL_JEDEC_OK(0xFFFFFFu)
             && !FL_JEDEC_OK(0xEB6014u) && !FL_JEDEC_OK(0xB36015u) && !FL_JEDEC_OK(0x856015u);
    printf("%s\n", ok ? "fm1 jedec test passed" : "FM1 JEDEC TEST FAILED");
    return !ok;
}
