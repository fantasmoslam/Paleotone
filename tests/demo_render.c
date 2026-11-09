/* SPDX-License-Identifier: GPL-3.0-only */
/* The demo song (ui_input.c demo_fill) rendered whole, as TOOLS > DEMO's song plays it: each section of DEMO_ORDER
 * for its eight bars, one after the other, into OUT.wav (a preview to listen to; not a test).
 *   build/host/demo_render OUT.wav */
#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
int main(int argc, char **argv)
{
    uint32_t i, k, f, j;
    const char *out = argc > 1 ? argv[1] : "build/host/demo-song.wav";
    FILE *w;
    outdir = "build/host";
    bank_resolve();
    panel = PANEL_DEFAULT;
    layers_init();
    host_tracks_init();
    for (i = 0; i < NPART; i++) { set_engine_of(&trk[i], TRK_DEF[i][0]); apply_preset_to(&trk[i], TRK_DEF[i][1]); trk[i].engine = trk[i].eng_req; }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    w = fopen(out, "wb");
    if (!w) return 1;
    {
        uint32_t total = 0, len[NELEM(DEMO_ORDER)];
        for (k = 0; k < NELEM(DEMO_ORDER); k++) {
            project_new();
            demo_fill(1, DEMO_ORDER[k]);
            len[k] = (uint32_t)((double)FS * 60.0 * 32.0 / song.g[G_BPM]) / CTL * CTL;
            total += len[k];
        }
        total += 4u * FS / CTL * CTL;
        wav_hdr(w, total);
        for (k = 0; k < NELEM(DEMO_ORDER); k++) {
            int32_t o[2 * CTL];
            transport_req = 2; for (j = 0; j < 4u; j++) mix_block(o, CTL);
            project_new();
            demo_fill(1, DEMO_ORDER[k]);
            transport_req = 1;
            for (f = 0; f < len[k] + (k + 1 == NELEM(DEMO_ORDER) ? 4u * FS / CTL * CTL : 0u); f += CTL) {
                if (k + 1 == NELEM(DEMO_ORDER) && f == len[k]) transport_req = 2;
                mix_block(o, CTL);
                for (j = 0; j < CTL; j++) wav_put(w, o[2 * j], o[2 * j + 1]);
            }
        }
    }
    fclose(w);
    printf("demo: %s\n", out);
    return 0;
}
