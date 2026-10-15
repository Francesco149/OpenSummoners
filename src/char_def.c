/* char_def.c — the character definition helpers.  See char_def.h.
 *
 * Transcribed from the asm (store widths: the counts +0xc754 / +0x14228 / +0xca24 / +0x1489c /
 * +0x14894 / +0x15b2c and +0xc840 / +0xc874..+0xc87c / +0x14828.. / +0x15948 / +0x159c0 /
 * +0x159ec / +0x15a00 / +0x15a02 / +0x15a08 / +0x15a0a / +0x15a14 / +0x15a20 / +0xe8 / +0x20c
 * are u16).  The tables are retail .data (g_eng.data). */
#include "char_def.h"

#include <string.h>

#include "engine_calls.h"
#include "entity_life.h"
#include "entity_move.h"
#include "rng.h"

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void stp(void *p, uint32_t off, const void *v) { st32(p, off, (int32_t)(intptr_t)v); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
/* imul semantics (wraps) */
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
#define B(c) ((uint8_t *)(c))
#define SKILL_TBL   0x6826c0u   /* 768-byte skill rows, keyed by +0 */
#define TRAILER     0x8a9b6cu   /* the empty string retail strcpys into a skill's +4 name */

uint8_t *char_skill_find(wl_char *c, uint16_t id)
{
    uint16_t n = ld16(c, 0xc754);
    for (uint16_t i = 0; i < n; i++)
        if ((uint32_t)ld32(c, 0x754 + 768u * i) == id) return B(c) + 0x754 + 768u * i;
    return NULL;
}

int char_ground_probe(wl_char *c)
{
    wl_body *b = c->base.body;
    int32_t y = b->y;
    c->platform = NULL;
    return entity_move_vertical(&c->base, b, 1, 0, 0, &y, (wl_entity **)&c->platform);
}

void voice_set_stop(wl_voiceset *v)
{
    for (uint16_t i = 0; i < v->n; i++) {
        wl_vgroup *g = &v->grp[i];
        for (uint16_t k = 0; k < g->n; k++) {
            wl_vline *l = &g->line[k];
            if (l->voice == NULL) continue;
            g_eng.dsb_stop_5bb8e0(l->voice->buf);
            l->voice->owner = 0;
            l->voice = NULL;
        }
    }
}

void voice_set_free(wl_voiceset *v)
{
    if (v->grp == NULL) { v->n = 0; return; }
    voice_set_stop(v);
    g_eng.op_delete(v->grp);
    v->grp = NULL;
    v->n = 0;
}

int tod_off(const void *sys)
{
    int32_t t = ld32(sys, 0x289c);
    return t <= 0 || t > 4;
}

int32_t grid_f1c(const void *grid) { return ld32(grid, 0x1c); }

int32_t stat_jitter(int32_t base, int32_t pm, int32_t r)
{
    int32_t q = mul(base, pm) / 1000;
    int32_t lo = mul(q, 20) / 100, hi = mul(q, 50) / 100;
    int32_t d = lo >= 2 ? lo : 2;
    if (hi < d) d = hi;
    if (d > 100) d = 100;
    return q + mul(r, d) / 1000 - d;
}

/* 0x426fd0's row select: the first row of `code` at `level` (any level for 0), else row 0 */
static const uint8_t *stat_row(int32_t code, int32_t level, int any)
{
    const uint8_t *t = g_eng.data(0x67ac58u);
    for (uint16_t i = 0; ld32(t, 220u * i) != 0; i++) {
        const uint8_t *r = t + 220u * i;
        if (ld32(r, 0) == code && (ld32(r, 4) == level || (any && level == 0))) return r;
    }
    return t;
}

void char_stats_init(uint8_t *st, int32_t code, int32_t level)
{
    const uint8_t *row = stat_row(code, level, 1);
    const uint8_t *row1 = stat_row(code, 1, 0);
    strcpy((char *)st, (const char *)(*(const char *)(row + 0x3a) != 0 ? row : row1) + 0x3a);
    const uint8_t *vt = g_eng.data(0x682660u), *v = NULL;   /* 24-byte rows keyed by +0 */
    for (uint16_t i = 0; ld32(vt, 24u * i) != 0; i++)
        if (ld32(vt, 24u * i) == code) { v = vt + 24u * i; break; }
    stp(st, 0x50, v);
    st32(st, 0xe8, 0);
    st32(st, 0xec, ld32(row, 0x20));
    st32(st, 0xd8, 0);
    st32(st, 0xdc, 0);
    st32(st, 0xe0, ld32(row, 0x04));
    st32(st, 0xe4, ld32(row, 0x24));
    st32(st, 0xf0, ld32(row, 0x28));
    int32_t s = ld32(row, 0x2c);
    int32_t j = mul(rnd(), 500) / 0x8000;
    st32(st, 0xf4, mul(s, 750) / 1000 + mul(j, s) / 1000);
    st32(st, 0xf8, ld32(row, 0x30));
    st32(st, 0xfc, ld32(row, 0x34));
    for (uint32_t o = 0x8c; o <= 0xa0; o += 4) st32(st, o, 0);
    st32(st, 0x84, 0);
    st32(st, 0x88, 0);
    for (uint32_t o = 0x74; o <= 0x80; o += 4) st32(st, o, 0);
    st32(st, 0x58, ld32(row, 0x08));
    st32(st, 0x54, ld32(row, 0x08));
    st32(st, 0x60, ld32(row, 0x0c));
    st32(st, 0x5c, ld32(row, 0x0c));
    st32(st, 0x64, ld32(row, 0x10));
    st32(st, 0x68, ld32(row, 0x14));
    st32(st, 0x6c, ld32(row, 0x18));
    st32(st, 0x70, ld32(row, 0x1c));
    st16(st, 0x4d8, 0);
    st16(st, 0x4da, 0);
    for (int i = 0; i < 12; i++) st32(st, 0xa4 + 4u * i, 1000);
    for (int i = 0; i < 18; i++) {
        st16(st, 0x448 + 2u * i, 0);
        st32(st, 0x490 + 4u * i, 0);
        st16(st, 0x46c + 2u * i, 0);
    }
    st32(st, 0xd4, 0);
    memset(st + 0x100, 0, 0x20);
    st16(st, 0x120, 0);
    for (uint32_t o = 0x124; o <= 0x138; o += 4) st32(st, o, 0);
    st16(st, 0x13c, 0);
    st32(st, 0x140, 1);
    st16(st, 0x144, 0);
    for (int i = 0; i < 64; i++) {
        st32(st, 0x148 + 12u * i, 0);
        st32(st, 0x14c + 12u * i, 0);
        st16(st, 0x150 + 12u * i, 0xff);
    }
}

void char_idle_init(wl_char *c, int32_t first, int32_t next, int32_t jitter)
{
    memset(B(c) + 0x15b2c, 0, 0x28);
    st32(c, 0x15b40, first);
    st32(c, 0x15b44, next);
    st32(c, 0x15b48, jitter);
    st32(c, 0x15b50, ld32(c, 0x15b40) + (int32_t)((uint32_t)mul(rnd(), ld32(c, 0x15b48)) >> 15));
}

void char_idle_add(wl_char *c, int32_t pose)
{
    uint16_t n = ld16(c, 0x15b2c);
    if (n >= 4) return;
    st32(c, 0x15b30 + 4u * n, pose);
    st16(c, 0x15b2c, (uint16_t)(n + 1));
}

void char_member_skill_add(wl_char *c, int32_t key, int32_t val)
{
    uint16_t n = ld16(c, 0x14894);
    if (n >= 4) return;
    st32(c, 0x14874 + 8u * n, key);
    st32(c, 0x14878 + 8u * n, val);
    st16(c, 0x14894, (uint16_t)(n + 1));
}

uint8_t *char_combo_add(wl_char *c, int32_t direct, int32_t skill, int32_t arg, int32_t ts_mode, int32_t release,
                        int32_t lo, int32_t hi, int32_t lo2, int32_t hi2, uint16_t id0, uint16_t id1, uint16_t id2,
                        uint16_t id3, uint16_t id4)
{
    uint16_t n = ld16(c, 0x1489c);
    uint8_t *r = B(c) + 0x148a0 + 64u * n;
    uint8_t *k = char_skill_find(c, (uint16_t)skill);
    if ((direct != 0 && k == NULL) || n >= 0x40) return r;
    st32(r, 0x00, direct);
    st32(r, 0x04, skill);
    st32(r, 0x08, arg);
    st32(r, 0x0c, ts_mode);
    st32(r, 0x38, release);
    st32(r, 0x10, lo);
    stp(r, 0x3c, k);
    st32(r, 0x14, hi);
    st32(r, 0x1c, lo2);
    st32(r, 0x20, id0);
    st32(r, 0x18, hi2);
    st16(r, 0x34, 1);
    /* id1 SETS the count to 2; the later ones count up from whatever it is (a later id with an
     * earlier one 0 lands past the count — retail) */
    if (id1 != 0) { st16(r, 0x34, 2); st32(r, 0x24, id1); }
    if (id2 != 0) { st16(r, 0x34, (uint16_t)(ld16(r, 0x34) + 1)); st32(r, 0x28, id2); }
    if (id3 != 0) { st16(r, 0x34, (uint16_t)(ld16(r, 0x34) + 1)); st32(r, 0x2c, id3); }
    if (id4 != 0) { st16(r, 0x34, (uint16_t)(ld16(r, 0x34) + 1)); st32(r, 0x30, id4); }
    st16(c, 0x1489c, (uint16_t)(n + 1));
    return r;
}

void char_clips_set(wl_char *c, const void *stand, const void *air0, const void *air1, int32_t unused, int32_t collide)
{
    (void)unused;
    stp(c, 0x15914, stand);
    stp(c, 0x15918, air0);
    stp(c, 0x1591c, air1);
    st32(c, 0x15920, 0);
    entity_set_collide(&c->base, collide, 1);
    for (uint32_t o = 0x15924; o <= 0x15940; o += 4) st32(c, o, 0);
}

void char_clips2_set(wl_char *c, const void *a, const void *b) { stp(c, 0x15924, a); stp(c, 0x15928, b); }

void char_carry_set(wl_char *c, int32_t on, const void *c0, const void *c1, int32_t a5, int32_t a6, int32_t a7)
{
    st32(c, 0x15934, on);
    stp(c, 0x1592c, c0);
    stp(c, 0x15930, c1);
    st32(c, 0x15938, a5);
    st32(c, 0x1593c, a6);
    st32(c, 0x15940, a7);
}

void char_step_set(wl_char *c, int32_t mode, uint16_t sfx) { st32(c, 0x15944, mode); st16(c, 0x15948, sfx); }

void char_pace_raw(wl_char *c, int32_t reach, int32_t c838, uint16_t c840, uint16_t wait, uint16_t c876,
                   uint16_t c878, uint16_t pause_chance, uint16_t pause_len, int32_t c844)
{
    st32(c, 0xc834, reach);
    st32(c, 0xc838, c838);
    st16(c, 0xc840, c840);
    st16(c, 0xc874, wait);
    st16(c, 0xc876, c876);
    st16(c, 0xc878, c878);
    st16(c, 0xc87a, pause_chance);
    st16(c, 0xc87c, pause_len);
    st16(c, 0x14828, 4);
    st16(c, 0x1482a, 2);
    st16(c, 0x1482c, 3);
    st32(c, 0xc844, c844);
}

/* (rand x n) >> 15 + base, drawn in retail's order */
static uint16_t roll(int32_t n, int32_t base) { return (uint16_t)(mul(rnd(), n) / 0x8000 + base); }

void char_pace_set(wl_char *c, int32_t kind, int32_t reach, int32_t c838, int32_t c844)
{
    uint16_t len, pch, c878, c876, wait;
    switch (kind) {
    case 0: char_pace_raw(c, reach, c838, 1000, 950, 1000, 1000, 1000, 2000, c844); return;
    case 1: char_pace_raw(c, reach, c838, 1000, 850, 1000, 1000, 1000, 1000, c844); return;
    case 2: len = roll(50, 950); pch = roll(100, 750); c878 = roll(100, 750); c876 = roll(50, 950); wait = roll(100, 650); break;
    case 3: len = roll(100, 700); pch = roll(100, 700); c878 = roll(100, 550); c876 = roll(100, 650); wait = roll(100, 350); break;
    case 4: len = roll(100, 450); pch = roll(100, 650); c878 = roll(100, 350); c876 = roll(100, 550); wait = roll(100, 150); break;
    case 5: len = roll(100, 300); pch = roll(100, 500); c878 = roll(100, 200); c876 = roll(100, 400); wait = roll(100, 50); break;
    case 6: case 7: char_pace_raw(c, reach, c838, 1000, 0, 0, 0, 0, 0, c844); return;
    default: return;
    }
    char_pace_raw(c, reach, c838, 1000, wait, c876, c878, pch, len, c844);
}

void char_c880_set(wl_char *c, int32_t a, int32_t b, int32_t d)
{
    st32(c, 0xc884, a);
    st32(c, 0xc880, 1);
    st32(c, 0xc888, b);
    st32(c, 0xc88c, d);
}

void char_move_set(wl_char *c, int32_t mode, int32_t fall_cap, int32_t grav, int32_t slide_cap, int32_t slide_grav,
                   int32_t f1f4, int32_t low_bridge, int32_t float_h)
{
    st32(c, 0x1594c, mode);
    st32(c, 0x15950, fall_cap);
    st32(c, 0x15954, grav);
    st32(c, 0x15958, slide_cap);
    st32(c, 0x1595c, slide_grav);
    st32(c, 0x15960, low_bridge);
    st32(c, 0x15964, float_h);
    st32(c, 0x1f4, f1f4);
}

void char_15a5c_set(wl_char *c, int32_t a, int32_t b, int32_t d, int32_t e)
{
    st32(c, 0x15a5c, a);
    st32(c, 0x15a60, b);
    st32(c, 0x15a64, d);
    st32(c, 0x15a68, e);
}

void char_turn_set(wl_char *c, int32_t on, int32_t air) { st32(c, 0x15984, on); st32(c, 0x15988, air); }

void char_walk_set(wl_char *c, int32_t max, int32_t accel, int32_t run_accel, int32_t brake, int32_t f1597c,
                   const void *clip)
{
    st32(c, 0x1596c, max);
    st32(c, 0x15970, accel);
    st32(c, 0x15974, run_accel);
    st32(c, 0x15978, brake);
    st32(c, 0x15968, 1);
    st32(c, 0x1597c, f1597c);
    stp(c, 0x15980, clip);
}

void char_jump_set(wl_char *c, int32_t impulse, int32_t g_held, int32_t g_free, int32_t air_max, int32_t fast,
                   int32_t slow, const void *c0, const void *c1, const void *c2, uint16_t sfx)
{
    st32(c, 0x1599c, impulse);
    st32(c, 0x159a0, g_held);
    st32(c, 0x159a4, g_free);
    st32(c, 0x159ac, fast);
    st32(c, 0x159b0, slow);
    st32(c, 0x159a8, air_max);
    stp(c, 0x159b4, c0);
    stp(c, 0x159b8, c1);
    stp(c, 0x159bc, c2);
    st32(c, 0x15998, 1);
    st16(c, 0x159c0, sfx);
    st32(c, 0x159c4, 0);
    st32(c, 0x159c8, 0);
    st32(c, 0x159cc, 0);
    st32(c, 0x159d0, 0);
}

void char_run_set(wl_char *c, int32_t max, const void *clip)
{
    st32(c, 0x1598c, 1);
    st32(c, 0x15990, max);
    stp(c, 0x15994, clip);
}

void char_hit_set(wl_char *c, uint16_t pose, const void *clip, int32_t f159f8, int32_t f158fc, int32_t unused)
{
    (void)unused;
    st16(c, 0x159ec, pose);
    stp(c, 0x159f0, clip);
    st32(c, 0x159f8, f159f8);
    st32(c, 0x158fc, f158fc);
    st32(c, 0x159fc, 0);
    st16(c, 0x15a00, 0);
    st16(c, 0x15a02, 0);
    st32(c, 0x15a04, 0);
    st32(c, 0x159f4, 0);
}

void char_member_pose_set(wl_char *c, uint16_t a, uint16_t pose, int32_t f15a0c)
{
    st16(c, 0x15a08, a);
    st16(c, 0x15a0a, pose);
    if (f15a0c != 0) st32(c, 0x15a0c, f15a0c);
}

void char_crouch_set(wl_char *c, int32_t margin, const void *c0, const void *c1, const void *c2)
{
    st32(c, 0x159d8, margin);
    stp(c, 0x159dc, c0);
    st32(c, 0x159d4, 1);
    stp(c, 0x159e0, c1);
    stp(c, 0x159e4, c2);
    st32(c, 0x159e8, 0);
}

void char_nav_skills_set(wl_char *c, int32_t sk, int32_t arg, int32_t sk2, int32_t arg2, int32_t lo, int32_t hi,
                         int32_t chase)
{
    st32(c, 0xc854, sk);
    st32(c, 0xc858, arg);
    st32(c, 0xc85c, sk2);
    st32(c, 0xc860, arg2);
    st32(c, 0xc864, lo);
    st32(c, 0xc868, hi);
    st32(c, 0xc86c, chase);
}

void char_approach_add(wl_char *c, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7,
                       int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, uint16_t a13)
{
    uint16_t n = ld16(c, 0xca24);           /* no bound check (retail) */
    uint8_t *r = B(c) + 0xc8a4 + 48u * n;
    st32(r, 0x04, a2);
    st32(r, 0x0c, a4);
    st32(r, 0x08, a3);
    st32(r, 0x10, a5);
    st32(r, 0x00, a6);
    st32(r, 0x24, a7);
    st32(r, 0x28, a8);
    st32(r, 0x14, a9);
    st32(r, 0x1c, a11);
    st32(r, 0x18, a10);
    st32(r, 0x20, a12);
    st16(r, 0x2c, a13);
    st16(c, 0xca24, (uint16_t)(n + 1));
}

/* the skill table row `id` (the scan ends at a 0 key; NULL if absent) */
static const uint8_t *skill_row(int32_t id)
{
    const uint8_t *t = g_eng.data(SKILL_TBL);
    for (uint16_t i = 0; ld32(t, 768u * i) != 0; i++)
        if (ld32(t, 768u * i) == id) return t + 768u * i;
    return NULL;
}

static uint8_t *skill_copy(wl_char *c, const uint8_t *e)
{
    uint16_t n = ld16(c, 0xc754);
    if (n >= 0x40) return NULL;
    uint8_t *r = B(c) + 0x754 + 768u * n;
    strcpy((char *)r + 0x04, (const char *)g_eng.data(TRAILER));
    strcpy((char *)r + 0x54, (const char *)e + 0x54);
    st32(r, 0x000, ld32(e, 0x000));
    st16(r, 0x0ae, ld16(e, 0x0ae));
    for (uint32_t o = 0x0b0; o <= 0x0c0; o += 4) st32(r, o, ld32(e, o));
    for (uint32_t o = 0x2c4; o <= 0x2d4; o += 4) st32(r, o, ld32(e, o));
    st16(r, 0x2d8, ld16(e, 0x2d8));
    st32(r, 0x2f8, 1);
    for (uint32_t o = 0x2dc; o <= 0x2e8; o += 4) st32(r, o, ld32(e, o));
    st16(r, 0x0a4, 3);
    st32(r, 0x0a8, 0);
    st16(r, 0x0ac, 0);
    st32(r, 0x2ec, ld32(e, 0x2ec));
    st16(r, 0x2f0, ld16(e, 0x2f0));
    st16(r, 0x2f2, ld16(e, 0x2f2));
    st32(r, 0x2f4, ld32(e, 0x2f4));
    st32(r, 0x2fc, 0);
    st16(c, 0xc754, (uint16_t)(n + 1));
    st16(r, 0x0a4, ld16(e, 0x0a4));
    st32(r, 0x0a8, ld32(e, 0x0a8));
    st16(r, 0x0ac, ld16(e, 0x0ac));
    return r;
}

void char_skills_by_code(wl_char *c, int32_t code)
{
    const uint8_t *t = g_eng.data(0x6907c0u);
    for (uint16_t i = 0; ld32(t, 16u * i) != 0; i++) {
        if (ld32(t, 16u * i) != code) continue;
        const uint8_t *e = skill_row(ld32(t, 16u * i + 4));
        if (e != NULL) skill_copy(c, e);
    }
}

uint8_t *char_skill_learn(wl_char *c, int32_t id)
{
    const uint8_t *e = skill_row(id);
    return e != NULL ? skill_copy(c, e) : NULL;
}

uint8_t *char_skill_def(wl_char *c, int32_t id, uint16_t fae, int32_t f0b0, int32_t f0b4, int32_t f0b8,
                        int32_t f0bc, int32_t f0c0, int32_t f2c4, int32_t f2c8, int32_t f2cc, int32_t f2d0,
                        int32_t f2d4, uint16_t f2d8, int32_t f2dc, int32_t f2e0, int32_t f2e4, int32_t f2e8,
                        int32_t f2ec, uint16_t f2f0, uint16_t f2f2, int32_t f2f4, const char *name)
{
    uint16_t n = ld16(c, 0xc754);
    if (n >= 0x40) return NULL;
    uint8_t *r = B(c) + 0x754 + 768u * n;
    strcpy((char *)r + 0x04, (const char *)g_eng.data(TRAILER));
    strcpy((char *)r + 0x54, name);
    st32(r, 0x000, id);
    st16(r, 0x0ae, fae);
    st32(r, 0x0b0, f0b0);
    st32(r, 0x0b4, f0b4);
    st32(r, 0x0b8, f0b8);
    st32(r, 0x0bc, f0bc);
    st32(r, 0x0c0, f0c0);
    st32(r, 0x2c4, f2c4);
    st32(r, 0x2c8, f2c8);
    st32(r, 0x2cc, f2cc);
    st32(r, 0x2d0, f2d0);
    st32(r, 0x2d4, f2d4);
    st16(r, 0x2d8, f2d8);
    st32(r, 0x2f8, 1);
    st32(r, 0x2dc, f2dc);
    st32(r, 0x2e0, f2e0);
    st32(r, 0x2e4, f2e4);
    st32(r, 0x2e8, f2e8);
    st16(r, 0x0a4, 3);
    st32(r, 0x0a8, 0);
    st16(r, 0x0ac, 0);
    st32(r, 0x2ec, f2ec);
    st16(r, 0x2f0, f2f0);
    st16(r, 0x2f2, f2f2);
    st32(r, 0x2f4, f2f4);
    st32(r, 0x2fc, 0);
    st16(c, 0xc754, (uint16_t)(n + 1));
    return r;
}

void char_nav_set(wl_char *c, int32_t drop, int32_t rise, int32_t fall)
{
    st32(c, 0xc890, drop);
    st32(c, 0xc898, rise);
    st32(c, 0xc894, fall);
}

void char_retarget_set(wl_char *c, uint16_t t, uint16_t b) { st16(c, 0xc89e, t); st16(c, 0xc8a0, b); }

uint8_t *ai_skill_boxes_add(uint8_t *rec, int32_t x0, int32_t x1, int32_t y0, int32_t y1, int32_t bx0, int32_t bx1,
                            int32_t by0, int32_t by1)
{
    uint16_t n = ld16(rec, 0x10);
    if (n < 8) {
        st32(rec, 0x14 + 16u * n, x0);
        st32(rec, 0x1c + 16u * n, y0);
        st32(rec, 0x18 + 16u * n, x1);
        st32(rec, 0x20 + 16u * n, y1);
        st16(rec, 0x10, (uint16_t)(n + 1));
    }
    n = ld16(rec, 0x94);
    if (n < 8) {
        st32(rec, 0x98 + 16u * n, bx0);
        st32(rec, 0xa0 + 16u * n, by0);
        st32(rec, 0x9c + 16u * n, bx1);
        st32(rec, 0xa4 + 16u * n, by1);
        st16(rec, 0x94, (uint16_t)(n + 1));
    }
    return rec;
}

uint8_t *char_ai_skill_add(wl_char *c, int32_t id, int32_t a3, int32_t a4, int32_t x0, int32_t x1, int32_t y0,
                           int32_t y1, uint16_t f11a, uint16_t f120, uint16_t f124, uint16_t f126, int32_t bx0,
                           int32_t bx1, int32_t by0, int32_t by1, int32_t unused)
{
    (void)unused;
    uint8_t *k = char_skill_find(c, (uint16_t)id);
    uint8_t *r = B(c) + 0xca28 + 320u * ld16(c, 0x14228);   /* no bound check (retail) */
    if (k == NULL) return NULL;
    stp(r, 0x000, k);
    st32(r, 0x00c, a4);
    st16(r, 0x11a, f11a);
    st32(r, 0x120, f120);
    st16(r, 0x124, f124);
    st16(r, 0x126, f126);
    st32(r, 0x004, id);
    st32(r, 0x008, a3);
    st16(r, 0x118, 0);
    st16(r, 0x11c, 0);
    st16(r, 0x010, 0);
    st16(r, 0x094, 0);
    st32(r, 0x128, 0);
    st32(r, 0x12c, 0);
    st32(r, 0x130, 0);
    st32(r, 0x134, 0);
    st32(r, 0x138, 1000);
    st16(r, 0x13c, 0);
    st16(r, 0x13e, 0);
    st32(r, 0x014, x0);
    st32(r, 0x01c, y0);
    st32(r, 0x018, x1);
    st32(r, 0x020, y1);
    st16(r, 0x010, 1);
    st32(r, 0x098, bx0);
    st32(r, 0x0a0, by0);
    st32(r, 0x09c, bx1);
    st32(r, 0x0a4, by1);
    st16(r, 0x094, 1);
    st16(c, 0x14228, (uint16_t)(ld16(c, 0x14228) + 1));
    return r;
}

void char_voice_init(wl_char *c, int32_t code, int32_t mode, int32_t bias)
{
    wl_voiceset *v = g_eng.op_new(0x18);
    if (v != NULL) { v->grp = NULL; v->n = 0; }
    c->voice = v;
    v->mode = 2;
    v->last = 0;
    if (v->grp != NULL) {                    /* (never: just cleared) */
        voice_set_stop(v);
        g_eng.op_delete(v->grp);
        v->grp = NULL;
    }
    v->f08 = 16;
    v->n = 74;
    v->grp = g_eng.op_new(0x176a0);
    memset(v->grp, 0, 1296u * v->n);
    v->fade_step = 400;
    const uint8_t *gt = g_eng.data(0x690ca0u);       /* 12-byte rows: u16 group, cd, u16 prio, u16 chance */
    for (uint16_t i = 0; ld16(gt, 12u * i) != 0; i++) {
        uint16_t g = ld16(gt, 12u * i);
        if (v->n <= g) g_eng.h5bf5db(0);
        wl_vgroup *p = &v->grp[g];
        p->cd_reset = ld32(gt, 12u * i + 4);
        p->prio = ld16(gt, 12u * i + 8);
        p->chance = ld16(gt, 12u * i + 10);
    }
    const uint8_t *lt = g_eng.data(0x691018u);       /* 36-byte rows: code, u16 group, u16 pool, len,
                                                      * .., u16 flag +0x1c, vol +0x20 */
    const uint8_t *sys = g_eng.sys;
    for (uint16_t i = 0; ld32(lt, 36u * i) != 0; i++) {
        const uint8_t *e = lt + 36u * i;
        if (ld32(e, 0) != code) continue;
        int32_t vol = 0;
        if (ld16(e, 0x1c) != 0 && ld32(sys, 0x310) != 0) {
            const uint8_t *cfg = (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(sys, 0x320), 0);
            if (ld32(cfg, 0x1c8) == 0) vol = ld32(e, 0x20);
        }
        void *pool = (void *)(intptr_t)ld32(sys, 0x364 + 4u * ld16(e, 8));
        uint16_t g = ld16(e, 4);
        if (v->n <= g) g_eng.h5bf5db(0);
        wl_vgroup *p = &v->grp[g];
        if (p->n >= 0x20) g_eng.h5bf5db(0);
        wl_vline *l = &p->line[p->n];
        l->len = ld32(e, 0xc);
        l->vol_base = vol;
        l->used = 0;
        l->pool = pool;
        l->voice = NULL;
        p->n++;
    }
    st32(c, 0x15a8c, mode);
    st32(c, 0x15a88, bias);
}

void char_npc_default(wl_char *c, int32_t reach, const void *stand)
{
    stp(c, 0x15914, stand);
    stp(c, 0x15918, g_eng.data(0x62c578u));
    stp(c, 0x1591c, g_eng.data(0x62c710u));
    st32(c, 0x15920, 0);
    entity_set_collide(&c->base, 2, 1);
    st32(c, 0x15924, 0);
    st32(c, 0x15928, 0);
    char_carry_set(c, 0, NULL, NULL, 0, 0, 0);
    stp(c, 0x15920, g_eng.data(0x62af28u));
    st32(c, 0x15984, 1);
    st32(c, 0x15988, 0);
    st32(c, 0x15968, 1);
    st32(c, 0x1596c, 24000);
    st32(c, 0x15970, 1600);
    st32(c, 0x15974, 3200);
    st32(c, 0x15978, -800);
    st32(c, 0x1597c, -1600);
    stp(c, 0x15980, g_eng.data(0x62abf8u));
    st32(c, 0x1598c, 1);
    st32(c, 0x15990, 48000);
    stp(c, 0x15994, g_eng.data(0x62ad90u));
    st32(c, 0x15998, 1);
    st32(c, 0x1599c, -80000);
    st32(c, 0x159a0, 2000);
    st32(c, 0x159a4, 8000);
    st32(c, 0x159ac, 1600);
    st32(c, 0x159b0, 3200);
    st32(c, 0x159a8, 48000);
    stp(c, 0x159b4, g_eng.data(0x62c0b0u));
    stp(c, 0x159b8, g_eng.data(0x62c248u));
    stp(c, 0x159bc, g_eng.data(0x62c3e0u));
    st16(c, 0x159c0, 134);
    st32(c, 0x159c4, 0);
    st32(c, 0x159c8, 0);
    st32(c, 0x159cc, 0);
    stp(c, 0x15924, g_eng.data(0x62d238u));
    st32(c, 0x159d0, 0);
    st32(c, 0x159d4, 1);
    st32(c, 0x159d8, 2400);
    stp(c, 0x159dc, g_eng.data(0x62b720u));
    stp(c, 0x159e0, g_eng.data(0x62b8b8u));
    stp(c, 0x159e4, g_eng.data(0x62ba50u));
    stp(c, 0x159e8, g_eng.data(0x62bbe8u));
    st16(c, 0x159ec, 99);
    stp(c, 0x159f0, g_eng.data(0x62bf18u));
    st32(c, 0x159f8, 0);
    st32(c, 0x158fc, 0);
    st32(c, 0x159fc, 0);
    st16(c, 0x15a00, 0);
    st32(c, 0x159f4, 0);
    st16(c, 0x15a02, 98);
    st32(c, 0x15a04, -400);
    stp(c, 0x15928, g_eng.data(0x62d3d0u));
    st32(c, 0x15934, 1);
    stp(c, 0x1592c, g_eng.data(0x62d568u));
    stp(c, 0x15930, g_eng.data(0x62d700u));
    st32(c, 0x15938, 0);
    st32(c, 0x1593c, 0);
    st32(c, 0x15940, 110);
    char_pace_set(c, 7, reach, 0, 800);
    st16(c, 0x15948, 0);
    st32(c, 0xc880, 1);
    st32(c, 0xc884, 12800);
    st32(c, 0xc888, 400);
    st32(c, 0xc88c, 12800);
    st32(c, 0x15944, 1);
    st32(c, 0x15a90, 1);
    st32(c, 0x1c, -8);
    st32(c, 0x15a94, 1);
}

void char_form0_set(wl_char *c, uint16_t v) { st16(c, 0x14828, v); }

void char_party_mode_set(wl_char *c, int32_t mode)
{
    st32(c->party, 0xd4, mode);
    if (ld32(c, 0x1d4) != 50010) return;    /* Arche only */
    const void *up1 = g_eng.data(0x62e558u);   /* (retail falls back to 0x62e3c0 when it is NULL) */
    if (mode == 1) {
        st32(c, 0x158a0, 2);
        st32(c, 0x15a10, 1);
        stp(c, 0x15a18, g_eng.data(0x62e3c0u));
        stp(c, 0x15a1c, up1);
        st16(c, 0x15a14, 700);
        st16(c, 0x15a20, 75);
        st32(c, 0xc848, 600);
        st32(c, 0x15a24, 0);
        st16(c, 0xe8, 1);
        stp(c, 0x15914, g_eng.data(0x62def8u));
        st32(c, 0x15a80, 0);
    } else {
        st32(c, 0x158a0, 0);
        stp(c, 0x15a18, g_eng.data(0x62e3c0u));
        st32(c, 0x15a10, 1);
        st32(c, 0x15a80, 1);
        stp(c, 0x15a1c, up1);
        st16(c, 0x15a14, 500);
        st16(c, 0x15a20, 78);
        st32(c, 0xc848, 200);
        st32(c, 0x15a24, 0);
        st16(c, 0xe8, 0);
        stp(c, 0x15914, g_eng.data(0x62a8c8u));
    }
}

void char_idle_voice_set(wl_char *c, int32_t on, uint16_t span)
{
    st32(c, 0x208, on);
    if (span != 0) st16(c, 0x20c, (uint16_t)((mul(rnd(), 1000) / 0x8000) % span));
    else st16(c, 0x20c, 0);
}
