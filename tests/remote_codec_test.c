/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "remote_codec.h"
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"wb");assert(f);uint32_t seed=19;
    for(uint32_t count=1;count<=128;count++)for(uint32_t kind=0;kind<8;kind++){
        uint8_t raw[128],packed[129],wire[180];
        for(uint32_t i=0;i<count;i++){
            seed=seed*1664525u+1013904223u;
            raw[i]=kind==0?0:kind==1?255:kind==2?(uint8_t)i:kind==3?(uint8_t)(i%2):kind==4?(uint8_t)(i/3):kind==5?(i%9<7?0:(uint8_t)seed):kind==6?(i<3||i>count/2?128:1):(uint8_t)(seed>>24);
        }
        packed[128]=0xa5;uint32_t n=remote_pack(raw,count,packed);
        assert(packed[128]==0xa5 && n<count);
        const uint8_t *data=n?packed:raw;uint32_t len=n?n:count;
        uint32_t w=0;wire[w++]=1;wire[w++]=0;wire[w++]=0;wire[w++]=0;wire[w++]=0;
        wire[w++]=count&127u;wire[w++]=count>>7;wire[w++]=n!=0;wire[w++]=len&127u;wire[w++]=len>>7;
        for(uint32_t i=0;i<len;){
            uint32_t size=len-i>7?7:len-i,mask=0;
            for(uint32_t k=0;k<size;k++)mask|=(uint32_t)(data[i+k]>>7)<<k;
            wire[w++]=(uint8_t)mask;
            for(uint32_t k=0;k<size;k++)wire[w++]=data[i++]&127u;
        }
        assert(w+6<=180);fwrite(&w,4,1,f);fwrite(wire,1,w,f);fwrite(raw,1,count,f);
    }
    fclose(f);puts("remote codec: 1024 bounded C wire fixtures generated");return 0;
}
