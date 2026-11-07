// SPDX-License-Identifier: GPL-3.0-only
#include <stdint.h>
#include <stdio.h>
extern void fw_ref_boot(void);
extern void fw_ref_ms(void);
extern const uint16_t *fw_ref_screen(void);
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    fw_ref_boot();
    for (unsigned ms = 0; ms < 1000; ++ms) fw_ref_ms();
    FILE *f = fopen(argv[1], "wb");
    if (!f) return 3;
    int ok = fwrite(fw_ref_screen(), 2, 240 * 240, f) == 240 * 240;
    fclose(f);
    return ok ? 0 : 4;
}
