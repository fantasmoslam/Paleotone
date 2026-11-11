/* SPDX-License-Identifier: GPL-3.0-only */
#define EN_HOST 1
#include <stdio.h>
#include <stdlib.h>
#include "firmware/src/pt/pt.c"
#include "firmware/src/dsp/taiko_rhythm.h"
int main(int argc,char**argv){unsigned r,st,w,fail=0,n=0;int wob[]={0,45,80,127};FILE*f=argc>1?fopen(argv[1],"w"):0;for(r=0;r<TAIKO_N_RHYTHMS;r++)for(w=0;w<4;w++)for(st=0;st<512;st++){unsigned got=taiko_rhythm_hit(r,st,wob[w],0),want=gurdy_hit(r,st,wob[w]);n++;if(got!=want)fail++;}if(taiko_rhythm_hit(99,0,0,0))fail++;if(f){fprintf(f,"step,kind\n");for(st=0;st<256;st++)fprintf(f,"%u,%u\n",st,taiko_rhythm_hit(TAIKO_GALOP,st,45,0));fclose(f);}printf("Paleotone rhythm extraction: %u comparisons, %u failures\n",n,fail);return fail!=0;}
