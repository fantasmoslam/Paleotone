/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual integrated generator beside all four music tracks. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc,char **argv)
{
    int32_t out[CTL*2];
    uint32_t i,k,frames,frame,secs,bio,music,on;
    double sum=0,peak=0;
    FILE *f;
    if(argc!=7)return 1;
    secs=atoi(argv[2]);bio=atoi(argv[3]);music=atoi(argv[4]);on=atoi(argv[5]);
    ui_power_on(); gb_scene_load(0); scape_restart();
    song.master_q12=2048;
    song.g[G_SC_BIO]=bio;scape_select(bio);
    song.g[G_SC_LEVEL]=on?112:0;
    song.g[G_SC_DANGER]=atoi(argv[6]);
    song.g[G_SC_TIME]=3;
    if(music)transport_req=1;
    else for(k=0;k<NTRK;k++)trk[k].p[P_MUTE]=1;
    mix_block(out,CTL);
    sc.enc_wait=SC_BPS*6u;
    frames=secs*FS/CTL*CTL;
    f=fopen(argv[1],"wb");if(!f)return 2;wav_hdr(f,frames);
    for(frame=0;frame<frames;frame+=CTL){
        mix_block(out,CTL);
        for(i=0;i<CTL;i++){
            double a=out[i*2],b=out[i*2+1];
            if(fabs(a)>peak)peak=fabs(a);if(fabs(b)>peak)peak=fabs(b);
            sum+=a*a+b*b;wav_put(f,(int32_t)a,(int32_t)b);
        }
    }
    fclose(f);
    printf("integrated: rms %.1f dBFS, peak %.1f dBFS, music voice kills %u\n",
        20*log10(sqrt(sum/(frames*2.0))/32768+1e-12),20*log10(peak/32768+1e-12),voice_kills);
    return peak>=32767?3:0;
}
