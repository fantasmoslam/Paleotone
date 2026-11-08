// SPDX-License-Identifier: GPL-3.0-only
// Match the original firmware's native/browser unit, sample for sample and
// pixel for pixel, while playing notes and operating its original panel.
#include "fm1core.h"
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
void fw_ref_boot();
void fw_ref_ms();
void fw_ref_keys(uint32_t);
void fw_ref_buttons(uint32_t);
void fw_ref_enc(uint32_t, int32_t);
const uint16_t *fw_ref_screen();
uint32_t fw_ref_audio_take(int16_t *, uint32_t);
}
static void require(bool ok, const char *why, unsigned ms) {
    if (!ok) { std::fprintf(stderr, "FAIL at %u ms: %s\n", ms, why); std::exit(1); }
}
int main(int argc, char **argv) {
    require(argc == 2, "Paleotone module path", 0);
    void *image = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    require(image != nullptr, "load actual module", 0);
    auto get = reinterpret_cast<fm1core_get_fn>(dlsym(image, FM1CORE_SYMBOL));
    require(get != nullptr, "descriptor", 0);
    const fm1core_t *c = get(FM1CORE_ABI);
    require(c != nullptr, "supported ABI", 0);
    c->flash_stage(nullptr, 0);
    c->options(FM1CORE_FLASH_STAGED, 0, 0, 1);
    c->panel->ready = 2;
    c->init(0);
    fw_ref_boot();
    uint32_t due = 0;
    uint64_t compared = 0;
    int16_t a[FM1P_BLOCK * 2], b[FM1P_BLOCK * 2];
    for (unsigned ms = 0; ms < 2000; ++ms) {
        c->tick(ms);
        if (ms == 650 || ms == 1300) {
            uint32_t keys = ms == 650 ? (1u << 7) | (1u << 11) : 0;
            c->panel->keys = keys;
            fw_ref_keys(keys);
        }
        if (ms == 900 || ms == 950) {
            c->panel->buttons = ms == 900 ? 1u << c->panel->btn_id[FM1P_B_ENV] : 0;
            fw_ref_buttons(ms == 900 ? 1u << FM1P_B_ENV : 0);
        }
        if (ms == 1050 || ms == 1500) {
            int role = ms == 1050 ? FM1P_E_K1 : FM1P_E_SELECT;
            c->panel->enc[c->panel->enc_id[role]] += 3 * c->panel->enc_dir[role];
            fw_ref_enc(static_cast<uint32_t>(role), 3);
        }
        if (ms % 15 == 0) c->frame(); else c->idle();
        fw_ref_ms();
        due += FM1P_FS;
        while (due >= 1000u * FM1P_BLOCK) {
            due -= 1000u * FM1P_BLOCK;
            c->audio(a, FM1P_BLOCK);
            require(fw_ref_audio_take(b, FM1P_BLOCK) == FM1P_BLOCK, "reference clock/ring", ms);
            require(!std::memcmp(a, b, sizeof a), "audio differs from original source", ms);
            compared += FM1P_BLOCK * 2;
        }
        if (ms == 600 || ms == 850 || ms == 1200 || ms == 1800)
            require(!std::memcmp(c->panel->lcd, fw_ref_screen(), sizeof c->panel->lcd), "screen differs from original source", ms);
    }
    c->shutdown();
    dlclose(image);
    std::printf("PASS: %llu PCM sample values and four complete screen snapshots exactly match original Paleotone\n",
                static_cast<unsigned long long>(compared));
}
