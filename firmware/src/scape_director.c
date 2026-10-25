/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared musical direction. Audio-clocked, no extra voices, allocations or sequencer.
 * Inactive is an exact bypass; ordinary patterns and live playing keep their mix. */
enum { DR_DRONE, DR_BASS, DR_PAD, DR_MELODY, DR_PERC };
static struct {
    volatile uint8_t active, chord_pc, suspended;
    uint8_t role[NTRK];
    volatile uint32_t elapsed, phase;
    int32_t gain[NTRK];
} scape_music;
static uint32_t scm_role(uint32_t i)
{
    static const uint8_t EXPLICIT[6] = {DR_PAD, DR_PAD, DR_MELODY, DR_BASS, DR_PERC, DR_DRONE};
    static const uint8_t AUTO[NTRK] = {DR_BASS, DR_PAD, DR_MELODY, DR_PERC};
    uint32_t r = (uint32_t)clamp(song.g[G_SROLE0+i], 0, 5), e = trk[i].eng_req % NENGINES;
    if (e == ENGI_DRUM) return DR_PERC;
    if (r) return EXPLICIT[r];
    if (e == ENGI_SAURUS || e == ENGI_GURDY) return DR_DRONE;
    return AUTO[i];
}
static int scm_running(void) { return scape_music.active && song.playing && !scape_music.suspended; }
static int scm_space(void)
{
    uint32_t i;
    if (!scm_running()) return 1;
    for (i=0;i<NTRK;i++) if (scape_music.role[i]==DR_MELODY && !trk[i].p[P_MUTE]) {
        const step_t *s=&trk[i].step[trk[i].seq_idx % NSTEP];
        if (s->time==ST_NOTE || s->time==ST_TIE) return 0;
    }
    return 1;
}
static uint32_t scm_activity(void)
{
    uint32_t p=scape_music.phase / (FS / 2u), a=p<120u?p:240u-p;
    return 127u - (uint32_t)clamp(song.g[G_SMOTION],0,127)*(120u-a)/160u;
}
static void scm_block(uint32_t n)
{
    static const uint16_t PERIOD[5]={31,23,29,13,17};
    uint32_t i;
    if (!scm_running()) return;
    if (song.g[G_DREAM]) {
        scape_music.elapsed += n;
        scape_music.phase += n;
        if (scape_music.phase>=FS*120u) scape_music.phase-=FS*120u;
    }
    for(i=0;i<NTRK;i++) {
        uint32_t role=scape_music.role[i], id=role==DR_PAD?G_SBED:role==DR_MELODY?G_SMELO:role==DR_BASS?G_SBASS:role==DR_PERC?G_SPULSE:G_SBED;
        uint32_t period=FS*PERIOD[role], ph=(scape_music.phase+i*FS*3u)%period;
        uint32_t tri=ph<period/2u?ph:period-ph;
        int32_t breath=(int32_t)(tri*127u/(period/2u));
        int32_t motion=clamp(song.g[G_SMOTION],0,127), arc=(int32_t)scm_activity();
        int32_t target=clamp(song.g[id],0,127)*32768/127;
        target=target*(127-motion*(127-breath)/254)/127;
        target=target*(127-motion*(127-arc)/254)/127;
        scape_music.gain[i]+=clamp(target-scape_music.gain[i],-128,128);
    }
}
static int32_t scm_level(track_t *t, int32_t level)
{
    return scm_running() ? mulq15(level,scape_music.gain[t-trk]) : level;
}
