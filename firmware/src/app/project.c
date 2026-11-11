/* SPDX-License-Identifier: GPL-3.0-only */
/* BERLIN project: one flash object (OBJ_PROJ). Only BERLIN's own magic and format are read; anything else in the
 * flash (DUB-1's, AMB-1's, an older format, none) gives the recipes. Part of the unity build, after app.h.
 *
 * The pieces (their recipes are the engine's, en_piece): each keeps its own settings as you leave it;
 * Tune and MIDI out are global. */
project_t proj;

const char *piece_name(int i) { return en_piece_name(i); }

static int global_par(int p) { return p == P_TUNE || p == P_MIDI; }

static void recipe(int i, slot_t *s)
{
    en_piece(i, s->par, &s->pool);
    s->pings_ok = 0;
    s->screen = 0;
}

typedef struct { uint32_t magic, format; uint8_t piece, leds; int16_t tune, midi; } project_head_t;
typedef struct { uint32_t magic, format; slot_t slot[10]; } saved_bank_t;
/* Extended globals retain the original zero-padded header as a full prefix.
 * The ten packed rhythm bytes use project_t.rsv, leaving preset layouts intact. */
typedef struct { project_head_t head; uint8_t beats[10]; } saved_head_t;
int project_drum_pattern(void){unsigned p=proj.piece,v=proj.rsv[p/2u]>>((p&1u)*4u)&15u;return v<EN_NBEAT?(int)v:0;}
void project_drum_pattern_set(int i){unsigned p=proj.piece,s=(p&1u)*4u;int v=(i%EN_NBEAT+EN_NBEAT)%EN_NBEAT;proj.rsv[p/2u]=(uint8_t)((proj.rsv[p/2u]&~(15u<<s))|((unsigned)v<<s));}
_Static_assert(sizeof(saved_bank_t) <= 3840u, "ten presets fit an A/B sector pair");
_Static_assert(NPIECES <= 20, "reserved preset object map");

static void take(const slot_t *s)                /* a piece's settings into play, the globals kept */
{
    int p;
    for (p = 0; p < P_NPARAMS; p++)
        if (!global_par(p))
            proj.par[p] = s->par[p];
    proj.pool = s->pool;
    proj.screen = (uint8_t)(s->screen % 3u);      /* (each piece its own view) */
}

static void keep(void)                           /* the piece playing, into its slot (with its ping loops) */
{
    int p;
    for (p = 0; p < P_NPARAMS; p++)
        proj.slot[proj.piece].par[p] = proj.par[p];
    proj.slot[proj.piece].pool = proj.pool;
    proj.slot[proj.piece].screen = proj.screen;
    en_seq_get(proj.slot[proj.piece].pings);
    proj.slot[proj.piece].pings_ok = 1;
}

void project_defaults(void)
{
    int i;
    memset(&proj, 0, sizeof proj);
    proj.magic = PROJ_MAGIC;
    proj.format = PROJ_FORMAT;
    for (i = 0; i < NPIECES; i++)
        recipe(i, &proj.slot[i]);
    for (i = 0; i < P_NPARAMS; i++)
        proj.par[i] = en_param_info(i)->def;
    take(&proj.slot[0]);
    proj.leds = 1;
}

static int pool_ok(uint32_t p)                   /* the root and at least two more notes */
{
    int n = 0, i;
    for (i = 0; i < 12; i++)
        n += (int)(p >> i & 1u);
    return (p & 1u) && !(p >> 12) && n >= 3;
}

static int pars_ok(const int16_t *par)
{
    int i;
    for (i = 0; i < P_NPARAMS; i++) {
        const en_param_t *p = en_param_info(i);
        if (par[i] < p->lo || par[i] > p->hi)
            return 0;
    }
    return 1;
}

static void pars_clamp(int16_t *par)            /* a value out of its (now narrower) range: to its end, not the whole
                                                  * project thrown away (1.3.0's one-sided knobs) */
{
    int i;
    for (i = 0; i < P_NPARAMS; i++) {
        const en_param_t *p = en_param_info(i);
        if (par[i] < p->lo)
            par[i] = p->lo;
        if (par[i] > p->hi)
            par[i] = p->hi;
    }
}
static int project_valid(void)                  /* (not: the recipes again) */
{
    int i;
    if (proj.magic != PROJ_MAGIC || !pool_ok(proj.pool) || !pars_ok(proj.par))
        return 0;
    if (proj.format != PROJ_FORMAT || proj.piece >= NPIECES)
        return 0;
    for (i = 0; i < NPIECES; i++)
        if (!pool_ok(proj.slot[i].pool) || !pars_ok(proj.slot[i].par))
            return 0;
    return 1;
}

/* Two ten-preset banks use A/B pairs at 0x98000..0x9BFFF, inside
 * the original data area; external sample slots 0xA0000..0xDBFFF are untouched.
 * Globals keep their original pair at 0xFC000. Each bank commits atomically. */
int project_load(void)
{
    project_head_t head;
    saved_head_t ext;
    saved_bank_t saved;
    int n, i, bank;
    project_defaults();
    memset(&ext,0,sizeof ext);
    n=plat_store_load(OBJ_PROJ, &ext, sizeof ext);
    head=ext.head;
    if ((n==(int)sizeof head || n==(int)sizeof ext) && head.magic==PROJ_MAGIC && head.format==PROJ_FORMAT) {
        if(n==(int)sizeof ext) memcpy(proj.rsv,ext.beats,sizeof ext.beats);
        for (bank=0;bank<2;bank++) {
            n=plat_store_load(OBJ_BANK0+(uint32_t)bank,&saved,sizeof saved);
            if(n!=(int)sizeof saved || saved.magic!=PROJ_MAGIC || saved.format!=PROJ_FORMAT) continue;
            for(i=0;i<10 && bank*10+i<NPIECES;i++) {
                if(!pool_ok(saved.slot[i].pool)) continue;
                pars_clamp(saved.slot[i].par);
                proj.slot[bank*10+i]=saved.slot[i];
            }
        }
        proj.piece=head.piece<NPIECES ? head.piece : 0;
        proj.leds=head.leds<=2 ? head.leds : 1;
        take(&proj.slot[proj.piece]);
        proj.par[P_TUNE]=head.tune; proj.par[P_MIDI]=head.midi;
        pars_clamp(proj.par);
        return project_valid() ? 0 : (project_defaults(), -1);
    }
    return -1;
}

int project_save(void)
{
    saved_bank_t saved, prior;
    project_head_t head;
    saved_head_t ext;
    int i, bank, rc, n;
    keep();
    for (bank=0;bank<2;bank++) {
        memset(&saved,0,sizeof saved);
        saved.magic=PROJ_MAGIC; saved.format=PROJ_FORMAT;
        for(i=0;i<10 && bank*10+i<NPIECES;i++) saved.slot[i]=proj.slot[bank*10+i];
        n=plat_store_load(OBJ_BANK0+(uint32_t)bank,&prior,sizeof prior);
        if(n==(int)sizeof prior && !memcmp(&prior,&saved,sizeof saved)) continue;
        rc=plat_store_save(OBJ_BANK0+(uint32_t)bank,&saved,sizeof saved);
        if(rc) return rc;
    }
    memset(&head,0,sizeof head);
    head.magic=PROJ_MAGIC; head.format=PROJ_FORMAT;
    head.piece=proj.piece; head.leds=proj.leds;
    head.tune=proj.par[P_TUNE]; head.midi=proj.par[P_MIDI];
    memset(&ext,0,sizeof ext);ext.head=head;memcpy(ext.beats,proj.rsv,sizeof ext.beats);
    return plat_store_save(OBJ_PROJ,&ext,sizeof ext);
}

void project_piece(int i)
{
    keep();
    proj.piece = (uint8_t)((i % NPIECES + NPIECES) % NPIECES);
    take(&proj.slot[proj.piece]);
}

void project_piece_reset(void)
{
    slot_t s;
    recipe(proj.piece, &s);
    project_drum_pattern_set(0);
    s.screen = proj.screen;                      /* (the view stays as it is) */
    take(&s);
    proj.slot[proj.piece].pings_ok = 0;          /* (its loops grown afresh too) */
}

void project_apply(void)                         /* (then en_regrow keeps the piece's loops, if it has them) */
{
    int i;
    const slot_t *s = &proj.slot[proj.piece];
    for (i = 0; i < P_NPARAMS; i++)
        en_set(i, proj.par[i]);
    en_pool(proj.pool);
    en_drum_pattern(project_drum_pattern());
#ifndef EN_LEGACY_TEST_BANK
    en_piece_theme(proj.piece);               /* scene-specific drum profile */
#endif
    en_seq(s->pings_ok ? s->pings : 0);
}
