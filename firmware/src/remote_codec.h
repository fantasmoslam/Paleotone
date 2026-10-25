/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* PackBits: literal token 0..127 => n+1 bytes; repeat 128..255 =>
 * (n&127)+1 copies of the next byte. Abort if it cannot beat raw bytes.
 * Bounded 128-byte input/output, no heap or persistent dictionary. */
static uint32_t remote_pack(const uint8_t *src,uint32_t count,uint8_t *out)
{
    uint32_t i=0,n=0;
    while(i<count) {
        uint32_t run=1;
        while(i+run<count && src[i+run]==src[i])run++;
        if(run>=3u) {
            if(n+2u>=count)return 0;
            out[n++]=(uint8_t)(128u+run-1u);out[n++]=src[i];i+=run;
        } else {
            uint32_t begin=i;
            do {
                i+=run;
                if(i>=count)break;
                run=1;while(i+run<count && src[i+run]==src[i])run++;
            } while(run<3u);
            uint32_t len=i-begin;
            if(n+1u+len>=count)return 0;
            out[n++]=(uint8_t)(len-1u);
            for(uint32_t k=begin;k<i;k++)out[n++]=src[k];
        }
    }
    return n;
}
