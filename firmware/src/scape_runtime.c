/* Runtime integration. One dedicated SAURUS context; it never enters track_render
 * (which indexes the four music tracks). Configuration remains in song.g. */
#ifndef SCAPE_AUDITION
static track_t sc_animal __attribute__((section(".pool")));
static uint32_t sc_off, sc_cooldown, sc_biome_req;
static int32_t sc_call_l, sc_call_r, sc_call_send, sc_work_gain = 32767;
static uint8_t sc_ready;
static uint32_t scape_busy(void)
{
    uint32_t i,n=sc_animal.v[0].active && sc_animal.v[0].stage != 4u;
    for(i=0;i<SC_NV;i++) n += sc.v[i].type != MV_FREE && sc.v[i].peak > 0;
    return n;
}
static void scape_yield(void)
{
    uint32_t i;
    for(i=0;i<SC_NV;i++) if(sc.v[i].type != MV_FREE) {
        sc.v[i].stage=0; sc.v[i].att=-sc.v[i].env;
        sc.v[i].peak=0; sc.v[i].reps=0;
    }
    if (sc_animal.v[0].active) {
        sc_animal.v[0].gate = 0;
        sc_animal.v[0].stage = 4;
    }
}
static int scape_reduce(void)
{
    if (!song.g[G_SC_LEVEL] || sc_cooldown) return 0;
    sc_cooldown = SC_BPS * 2u;
    scape_yield();
    sc.enc_len = 0;
    sc.enc_wait = SC_BPS * 20u;
    sc.hush = sc.hush_to = 32767;
    return 1;
}
static void scape_select(uint32_t b) { sc_biome_req = b % SC_NBIOME + 1u; }
static void scape_call(uint32_t c, int32_t lvl, int32_t dist, int32_t pan, uint32_t ms)
{
    track_t *t = &sc_animal;
    const preset_t *p = &ENG_SAURUS.presets[c % ENG_SAURUS.npresets];
    uint32_t i;
    int32_t g;
    if (sc_cooldown || sc_animal.v[0].active || voices_busy() >= NVOICE) return;
    memset(t, 0, sizeof *t);
    t->engine = t->eng_req = ENGI_SAURUS;
    for (i = 0; i < P_COUNT; i++) t->p[i] = TP[i].def;
    for (i = 0; i < 8u; i++) t->p[P_E0+i] = p->e[i];
    t->p[P_ATK] = p->env[0]; t->p[P_DEC] = p->env[1];
    t->p[P_SUS] = p->env[2]; t->p[P_REL] = p->env[3];
    voice_start(t, &t->v[0], 50, 100, 0);
    sc_off = (ms ? ms : 3000u) * SC_BPS / 1000u;
    lvl = lvl * 32768 / ((int32_t)sc.level*sc.level*2 | 1);
    g = mulq15(lvl, 32767 - clamp(dist,0,127)*220);
    g = (LEVEL_Q12[104] * g) >> 15;
    sc_call_l = (g * (4096 - (pan > 0 ? pan * 3200 / 32768 : 0))) >> 12;
    sc_call_r = (g * (4096 + (pan < 0 ? pan * 3200 / 32768 : 0))) >> 12;
    sc_call_send = (30 + clamp(dist,0,127)*70/127) * 258;
}
static void scape_animal(int32_t *l, int32_t *r, int32_t *rev, uint32_t n)
{
    voice_t *v = &sc_animal.v[0];
    vmod_t m = {0};
    int32_t buf[CTL] = {0}, env;
    uint32_t i;
    int32_t gain = (int32_t)sc.level*sc.level*2;
    int32_t gl = mulq15(sc_call_l,gain), gr = mulq15(sc_call_r,gain);
    if (!v->active) return;
    if (sc_off && !--sc_off) { v->gate = 0; v->stage = 3; }
    env = saurus_amp(&sc_animal,v,env_tick(&sc_animal,v));
    m.envq15 = env; m.amp0 = v->env_out; m.amp1 = mulq15(env,v->vel*258);
    v->env_out = m.amp1;
    m.pitch16 = v->pitch_cur; m.inc = pitch_inc(m.pitch16);
    m.shape = 64 << 8;
    saurus_render(&sc_animal,v,buf,n,&m);
    for (i=0;i<n;i++) {
        int32_t a = (buf[i] * gl) >> 12, b = (buf[i] * gr) >> 12;
        l[i] += a; r[i] += b;
        rev[i] += mulq15((a+b)>>1,mulq15(sc_call_send,song.g[G_SC_SEND]*258));
    }
}
static void scape_sync(void)
{
    uint32_t i;
    if (!sc_ready) { scape_reset(); sc_ready = 1; sc.level = 0; }
    if (sc_biome_req) {
        uint32_t b = sc_biome_req-1u;
        sc_biome_req=0;
        for (i=0;i<SC_NL;i++) {
            const sc_layer_t *v = &SC_BIOMES[b].l[i];
            song.g[G_SC_L0K+i*4] = v->kind; song.g[G_SC_L0V+i*4] = v->level;
            song.g[G_SC_L0N+i*4] = v->dens; song.g[G_SC_L0D+i*4] = v->dist;
        }
        song.g[G_SC_DTYPE]=SC_BIOMES[b].drone; song.g[G_SC_DLEVEL]=SC_BIOMES[b].dlevel;
    }
    if (sc.biome != song.g[G_SC_BIO]) {
        sc.biome=(uint8_t)song.g[G_SC_BIO];
        memset(sc.bed,0,sizeof sc.bed);
        memset(sc.v,0,sizeof sc.v);
        scape_yield();
    }
    sc.level=(uint8_t)clamp((int32_t)sc.level+clamp(song.g[G_SC_LEVEL]-sc.level,-4,4),0,127);
    sc.tod=(uint8_t)song.g[G_SC_TIME]; sc.weather=(uint8_t)song.g[G_SC_WEATHER];
    sc.danger=(uint8_t)song.g[G_SC_DANGER]; sc.dens=(uint8_t)song.g[G_SC_DENS];
    sc.dtype=(uint8_t)song.g[G_SC_DTYPE]; sc.dlevel=(uint8_t)song.g[G_SC_DLEVEL];
    sc.dmove=(uint8_t)song.g[G_SC_DMOVE];
    sc.dkey=(uint8_t)(song.g[G_SC_KEY] ? song.g[G_SC_KEY]-1 : trk[0].p[P_ROOT]%12);
    for (i=0;i<SC_NL;i++) {
        if(sc.l[i].kind != song.g[G_SC_L0K+i*4]) memset(&sc.bed[i],0,sizeof sc.bed[i]);
        sc.l[i]=(sc_layer_t){(uint8_t)song.g[G_SC_L0K+i*4],(uint8_t)song.g[G_SC_L0V+i*4],
                            (uint8_t)song.g[G_SC_L0N+i*4],(uint8_t)song.g[G_SC_L0D+i*4]};
    }
}
static void scape_restart(void)
{
    memset(&sc_animal,0,sizeof sc_animal);
    memset(&sc,0,sizeof sc);
    sc_ready=0; sc_cooldown=sc_biome_req=0; sc_work_gain=32767;
}
#else
static void scape_restart(void) {}
static uint32_t scape_busy(void) { return 0; }
static void scape_yield(void) {}
static int scape_reduce(void) { return 0; }
static void scape_select(uint32_t b) { (void)b; }
#endif
