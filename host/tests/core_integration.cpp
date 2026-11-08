// SPDX-License-Identifier: GPL-3.0-only
// Exercise the public ABI through two actual shared modules, not a fake engine.
#include "fm1core.h"
#include <dlfcn.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
static void require(bool ok, const char *why) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
struct Loaded {
    void *image;
    const fm1core_t *core;
    explicit Loaded(const char *path, const char *id) {
        image = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!image) { std::fprintf(stderr, "%s\n", dlerror()); std::exit(1); }
        auto get = reinterpret_cast<fm1core_get_fn>(dlsym(image, FM1CORE_SYMBOL));
        require(get != nullptr, "descriptor symbol");
        require(get(999) == nullptr, "reject incompatible ABI");
        core = get(FM1CORE_ABI);
        require(core && !std::strcmp(core->id, id), "distinct instrument identity");
        require(core->panel && core->audio && core->frame && core->idle && core->tick, "required callbacks");
        require(core->flash && core->flash_stage && core->flash_size == 0x100000u, "persistent flash contract");
    }
    ~Loaded() { core->shutdown(); dlclose(image); }
};
static uint64_t run(const fm1core_t *c, unsigned duration, bool notes) {
    int16_t pcm[FM1P_BLOCK * 2];
    uint32_t due = 0;
    uint64_t energy = 0;
    for (uint32_t ms = 0; ms < duration; ++ms) {
        c->tick(ms);
        if (notes && ms == 650) c->panel->keys = 1u << 7;
        if (ms == 1000) c->panel->keys = 0;
        if (ms % 15 == 0) c->frame(); else c->idle();
        due += FM1P_FS;
        while (due >= 1000u * FM1P_BLOCK) {
            due -= 1000u * FM1P_BLOCK;
            c->audio(pcm, FM1P_BLOCK);
            for (auto v : pcm) energy += static_cast<uint64_t>(std::abs(static_cast<int>(v)));
        }
    }
    return energy;
}
int main(int argc, char **argv) {
    require(argc == 3, "paths to Paleotone and Felucca modules");
    const char *ids[] = {"paleotone", "felucca"};
    std::vector<uint8_t> saved[2];
    for (unsigned cycle = 0; cycle < 3; ++cycle) {
        for (unsigned i = 0; i < 2; ++i) {
            Loaded l(argv[i + 1], ids[i]);
            auto c = l.core;
            require(c->flash_stage(saved[i].empty() ? nullptr : saved[i].data(),
                                   static_cast<uint32_t>(saved[i].size())) != 0, "stage own saved image");
            c->options(FM1CORE_FLASH_STAGED, 0, 0, 1);
            c->panel->ready = 2;
            c->init(0);
            require(c->panel->ready == 1 && !c->halted(), "boot loaded application");
            auto energy = run(c, 1700, true);
            require(energy > 0, "real key produces audio");
            require(c->panel->lcd_writes > 0, "real firmware display updates");
            for (auto e : c->panel->enc_id) require(e < FM1P_NENC, "encoder mapping valid");
            c->flash_sync();
            saved[i].assign(c->flash(), c->flash() + c->flash_size);
            std::printf("PASS %s cycle %u: audio energy %llu; screen writes %u; independent image %zu bytes\n",
                ids[i], cycle + 1, static_cast<unsigned long long>(energy),
                static_cast<unsigned>(c->panel->lcd_writes), saved[i].size());
        }
    }
}
