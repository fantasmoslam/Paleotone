/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"SCAPE FAIL line %d: %s\n",__LINE__,#x); failures++; } } while(0)
int main(void)
{
    project_t a,b;
    project_store_t disk,old;
    int32_t out[CTL*2];
    uint32_t i,j,k;
    ui_power_on();
    project_capture(&a);
    CHECK(proj_pack(&disk,&a));
    CHECK(*(uint32_t *)disk.raw == PROJ_MAGIC);
    old=disk;
    CHECK(proj_import(&b,old.raw,sizeof old));
    CHECK(b.g[G_SC_LEVEL]==0);
    /* Maximum project content plus arbitrary exact FM patches and every layer. */
    for(i=0;i<NTRK;i++) for(j=0;j<FM6_PACKED;j++) a.fm6[i][j]=(i*37+j*19)&127u;
    for(i=G_SC_BIO;i<G_COUNT;i++) a.g[i]=GP[i].max;
    CHECK(proj_pack(&disk,&a));
    CHECK(*(uint32_t *)disk.raw==PROJ_MAGIC_SC);
    CHECK(proj_import(&b,disk.raw,sizeof disk));
    CHECK(!memcmp(a.fm6,b.fm6,sizeof a.fm6));
    for(i=G_SC_BIO;i<G_COUNT;i++) CHECK(a.g[i]==b.g[i]);
    disk.raw[3760+SC_PARAM_COUNT]=1;
    *(uint32_t *)(disk.raw+3836)=proj_hash(disk.raw,3836);
    CHECK(!proj_import(&b,disk.raw,sizeof disk));
    CHECK(proj_import(&b,old.raw,sizeof old));
    CHECK(b.g[G_SC_LEVEL]==0);
    /* Rendering must never rewrite music track parameters or presets. */
    song.g[G_SC_LEVEL]=112;
    for(k=0;k<100;k++) mix_block(out,CTL);
    for(i=0;i<NTRK;i++) for(j=0;j<P_COUNT;j++) a.t[i].p[j]=trk[i].p[j];
    scape_call(1,18000,20,0,1000);
    CHECK(scape_busy());
    for(k=0;k<50;k++) mix_block(out,CTL);
    for(i=0;i<NTRK;i++) CHECK(!memcmp(a.t[i].p,trk[i].p,sizeof trk[i].p));
    /* A music allocation at the full budget releases the creature first. */
    for(i=0;i<NTRK;i++) for(j=0;j<NVOICE;j++) memset(&trk[i].v[j],0,sizeof trk[i].v[j]);
    for(j=0;j<NVOICE-1;j++) { trk[0].v[j].active=1; trk[0].v[j].stage=2; }
    CHECK(voices_busy()==NVOICE);
    (void)voice_room(&trk[1],0);
    CHECK(!scape_busy());
    CHECK(scape_reduce());
    CHECK(!scape_reduce()); /* no permanent shielding of music overload protection */
    CHECK(sc_cooldown>0);
    for(k=0;k<4;k++) scape_block(out,out+CTL,out,CTL);
    CHECK(sc_work_gain==0);
    scape_restart();
    for(i=0;i<NTRK;i++) memset(trk[i].v,0,sizeof trk[i].v);
    song.g[G_SC_LEVEL]=112; song.g[G_SC_DENS]=0;
    song.g[G_SC_DANGER]=0; song.g[G_SC_WEATHER]=0; song.g[G_SC_DTYPE]=0;
    for(k=0;k<20000;k++) mix_block(out,CTL);
    CHECK(sc.nspawn==0);
    sc.danger=110; sc.enc_len=100; sc.hush_to=4000; sc.danger=0;
    sc_encounter(); CHECK(!sc.enc_len && sc.hush_to==32767);
    for(i=0;i<NPAGES;i++) if(PAGES[i].fam==FAM_GLO) { CHECK(PAGES[i].graph==GR_SONG); break; }
    ui_power_on(); hold(B_HOME); ui.menu_sel=MI_SCAPE; press(B_OCTUP);
    CHECK(!ui.menu && !ui.home && cur_page()->fam==FAM_SCAPE && !strcmp(cur_page()->title,"SCAPE"));
    press(B_GLO); frames(320); CHECK(!strcmp(cur_page()->title,"WORLD"));
    go_home(); press(B_GLO); frames(320); CHECK(cur_page()->graph==GR_SONG);
    printf("SCAPE integration: %s\n",failures?"FAIL":"PASS");
    return failures ? 1 : 0;
}
