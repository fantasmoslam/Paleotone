#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "firmware/src/pt/pt.h"
static void u16(FILE*f,unsigned v){fputc(v&255,f);fputc((v>>8)&255,f);}static void u32(FILE*f,unsigned v){u16(f,v);u16(f,v>>16);}
int main(int argc,char**argv){int voices[]={1,2,5,8},k,n,b,j;float pcm[256];char path[1024];(void)argc;for(k=0;k<4;k++)for(n=24;n<=36;n+=12){FILE*f;snprintf(path,sizeof path,"%s/tone-%d-%d.wav",argv[1],voices[k],n);f=fopen(path,"wb");fwrite("RIFF",1,4,f);u32(f,36+6*44100*4);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,3);u16(f,1);u32(f,44100);u32(f,176400);u16(f,4);u16(f,32);fwrite("data",1,4,f);u32(f,6*44100*4);pt_init();pt_sound(PT_SEQ2,voices[k]);pt_note_on(PT_SEQ2,n,100,4*44100);for(b=0;b<6*44100;b+=256){int z=6*44100-b<256?6*44100-b:256;pt_render(PT_SEQ2,pcm,z);for(j=0;j<z;j++)fwrite(&pcm[j],4,1,f);}fclose(f);printf("%s note %d rendered\n",pt_name(PT_SEQ2,voices[k]),n);}return 0;}
