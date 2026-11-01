/* SPDX-License-Identifier: GPL-3.0-only */
/* Built twice from one script: with the engine's dry UI frames (the app) and with
 * PT_ENGINE_DRAWS (the firmware drawing as before). Both print one digest of all
 * audio and every display snapshot; test.sh requires them to be identical. The
 * drawing build also compares each firmware frame with the display copy's. */
#include "../native/engine.c"
#include "../native/display.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint64_t digest=1469598103934665603ull;
static void mix(const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i){digest^=b[i];digest*=1099511628211ull;}}
static uint32_t lcg=12345;
static unsigned rnd(unsigned n){lcg=lcg*1664525u+1013904223u;return (lcg>>8)%n;}
int main(int argc,char **argv){
    unsigned seconds=argc>1?(unsigned)atoi(argv[1]):120;
    static float out[1024*2];static uint8_t snap[PT_SNAPSHOT_MAX];
    pt_boot(NULL,0);pt_display_boot();
    unsigned frames_seen=pt_ui_frames(),snaps=0,same=0,buttons=0,keys=0,hold=0;
    unsigned long long px_diff=0;unsigned dumped=0;(void)dumped;
    for(unsigned block=0;block<seconds*250u;++block){      /* 192-frame callbacks at 48 kHz */
        if(block%25==0){                                     /* every 0.1 s, a panel or MIDI action */
            switch(rnd(12)){
            case 0: case 1: buttons=1u<<rnd(14);hold=1+rnd(12);break;
            case 2: case 3: pt_encoder(rnd(7),(int)rnd(9)-4);break;
            case 4: keys^=1u<<rnd(27);break;
            case 5: pt_midi(0x90,48+rnd(36),40+rnd(80));break;
            case 6: pt_midi(0x80,48+rnd(36),0);break;
            case 7: pt_control_set(rnd(pt_control_count()),(int)rnd(128));break;
            case 8: pt_scape(PT_ENV_START+rnd(PT_SCAPE_NEXT));break;
            case 9: keys=0;break;
            default: break;
            }
            if(hold&&!--hold)buttons=0;
            pt_buttons(buttons);pt_keys(keys);
        }
        pt_render(out,192,48000);mix(out,192*2*sizeof *out);
        if(pt_ui_frames()!=frames_seen){
            frames_seen=pt_ui_frames();
            uint32_t n=pt_snapshot(snap,sizeof snap);assert(n);mix(snap,n);++snaps;
#ifdef PT_ENGINE_DRAWS
            assert(pt_display_apply(snap,n));
            static uint32_t mine[240*240],theirs[240*240];
            pt_display_screen(theirs);
            for(unsigned i=0;i<240*240;++i){unsigned c=(web_fb[i]>>8)|((web_fb[i]&255)<<8);
                unsigned r=(c>>11)&31,g=(c>>5)&63,b=c&31;mine[i]=0xff000000u|((r<<3|r>>2)<<16)|((g<<2|g>>4)<<8)|(b<<3|b>>2);}
            unsigned d=0;for(unsigned i=0;i<240*240;++i)d+=mine[i]!=theirs[i];
            px_diff+=d;same+=!d;
            if(getenv("PT_DUMP")&&d>0&&(dumped<3||d>300)&&dumped<8){char name[512];snprintf(name,512,"%s/diff%u.raw",getenv("PT_DUMP"),dumped++);
                FILE*f=fopen(name,"wb");fwrite(mine,4,240*240,f);fwrite(theirs,4,240*240,f);fclose(f);}
#endif
        }
    }
#ifdef PT_ENGINE_DRAWS
    fprintf(stderr,"display copy: %u of %u frames pixel-identical to the firmware's own drawing, %.3f%% of pixels differ\n",
        same,snaps,100.0*px_diff/((double)snaps*240*240));
    assert(same*3>snaps*2&&px_diff<0.02*snaps*240*240);   /* (PT_DUMP=dir saves differing frames) */
#endif
    printf("%016llx %u snapshots\n",(unsigned long long)digest,snaps);
}
