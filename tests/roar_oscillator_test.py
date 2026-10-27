#!/usr/bin/env python3
"""Compile the actual production oscillator functions and compare wrap/branch boundaries."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
def extract(text,name):
    pos=text.index(' '+name+'(')
    start=text.rfind('\n',0,pos)+1
    brace=text.index('{',pos); depth=1; end=brace+1
    while depth:
        depth += (text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
dsp=(ROOT/'firmware/src/dsp.c').read_text()
gurdy=(ROOT/'firmware/src/eng_gurdy.c').read_text()
source='#include <stdint.h>\n#include <stdio.h>\n'+extract(dsp,'blep')+'\n'+extract(dsp,'osc_saw')+'\n'+extract(gurdy,'gurdy_saw')+r'''
static __attribute__((noinline)) int32_t reference(uint32_t p,uint32_t i){return osc_saw(p,i);}
static __attribute__((noinline)) int32_t candidate(uint32_t p,uint32_t i){return gurdy_saw(p,i);}
static uint32_t rng=0x12345678u;
static uint32_t next(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static unsigned cases;
static int compare(uint32_t p,uint32_t i){cases++;if(reference(p,i)!=candidate(p,i)){printf("mismatch phase=%u inc=%u\n",p,i);return 1;}return 0;}
int main(void){
 const uint32_t edge[]={0,1,32767,32768,32769,65534,65535,65536,65537,0x3fffffffu,0x7fffffffu,0x80000000u,0xfffffffeu,0xffffffffu};
 unsigned j,k;int bad=0;
 for(j=0;j<sizeof edge/sizeof edge[0];j++){
   uint32_t i=edge[j];uint32_t p[]={0,1,0xffffffffu,i-1,i,i+1,0xffffffffu-i-1,0xffffffffu-i,0xffffffffu-i+1};
   for(k=0;k<sizeof p/sizeof p[0];k++)bad|=compare(p[k],i);
 }
 for(j=0;j<1000000;j++){uint32_t p=next(),i=next();bad|=compare(p,i);}
 printf("actual ROAR oscillator: %u cases, %s\n",cases,bad?"FAIL":"PASS");return bad;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp); (p/'test.c').write_text(source)
    for opt in ('-Os','-O2'):
        subprocess.run([os.environ.get('CC','cc'),'-std=c99',opt,'-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
        print(opt,flush=True);subprocess.run([str(p/'test')],check=True)
