#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "amb.h"
static FILE *events; static unsigned samplepos;
void en_midi_out(uint32_t s,uint32_t a,uint32_t b){if(events && (s&0xF0)==0x90 && b)fprintf(events,"%.5f,%u,%u,%u\n",samplepos/44100.0,(s&15)+1,a,b);}
static void u16(FILE*f,unsigned v){fputc(v&255,f);fputc((v>>8)&255,f);}
static void u32(FILE*f,unsigned v){u16(f,v);u16(f,v>>16);}
int main(int argc,char**argv){int preset=atoi(argv[1]),bass=atoi(argv[2]),sec=atoi(argv[3]),j,b;int16_t p[P_NPARAMS];uint16_t pool;int32_t pcm[512];unsigned frames=sec*44100,bytes=frames*4;FILE*f=fopen(argv[4],"wb");FILE*m=fopen(argv[5],"w");events=m;fprintf(m,"time_s,channel,note,velocity\n");(void)argc;
fwrite("RIFF",1,4,f);u32(f,36+bytes);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,1);u16(f,2);u32(f,44100);u32(f,176400);u16(f,4);u16(f,16);fwrite("data",1,4,f);u32(f,bytes);
en_init();en_piece(preset,p,&pool);if(bass==1){p[P_CHORD]=p[P_SEQ]=p[P_LEAD]=p[P_SKLEVEL]=p[P_CLLEVEL]=p[P_CRACKLE]=0;p[P_REVERB]=p[P_ECHO]=p[P_S2ECHO]=0;}
if(bass==2){p[P_CHORD]=p[P_SEQ]=p[P_LEAD]=p[P_SEQ2]=p[P_CLLEVEL]=p[P_CRACKLE]=0;}
if(bass==3){p[P_SEQ]=p[P_LEAD]=p[P_SEQ2]=p[P_SKLEVEL]=p[P_CLLEVEL]=p[P_CRACKLE]=0;}
p[P_MIDI]=1;for(j=0;j<P_NPARAMS;j++)en_set(j,p[j]);en_pool(pool);en_piece_theme(preset);en_regrow();en_play(1);
for(b=0;b<(int)frames;b+=256){int n=frames-b<256?frames-b:256;samplepos=(unsigned)b;en_render(pcm,n,2048);for(j=0;j<n*2;j++)u16(f,(unsigned)(pcm[j]>>8));}
fclose(f);fclose(m);return 0;}
