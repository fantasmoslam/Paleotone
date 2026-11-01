/* SPDX-License-Identifier: GPL-3.0-only */
#include "../native/output_stage.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    pt_output_t state;float a[2];
    pt_output_init(&state,48000,2);
    a[0]=0.1f;a[1]=-0.05f;pt_output_process(&state,a,1,2);
    assert(fabsf(a[0]-.2f)<1e-6f&&fabsf(a[1]+.1f)<1e-6f);
    /* Full-scale transients and over-range input stay bounded, stereo ratio intact. */
    a[0]=1;a[1]=-.5f;pt_output_process(&state,a,1,4);
    assert(fabsf(a[0]-.98f)<1e-6f&&fabsf(a[1]+.49f)<1e-6f);
    assert(state.limited>0);
    for(unsigned i=0;i<48000;++i){
        a[0]=i&1?8:-8;a[1]=-a[0]/2;pt_output_process(&state,a,1,4);
        assert(isfinite(a[0])&&fabsf(a[0])<=.980001f);
        assert(fabsf(a[1]+a[0]/2)<1e-6f);
    }
    a[0]=NAN;a[1]=INFINITY;pt_output_process(&state,a,1,2);assert(a[0]==0&&a[1]==0);
    for(unsigned i=0;i<48000;++i){a[0]=.1f;a[1]=.1f;pt_output_process(&state,a,1,2);}
    assert(fabsf(a[0]-.2f)<1e-4f);
    puts("PASS: +6 dB correction, stereo limiter, nonfinite input, gain ramp/release");
}
