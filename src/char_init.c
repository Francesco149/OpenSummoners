/* char_init.c — FUN_0041f200.  See char_init.h.
 *
 * Transcribed from the decompile, checked against the asm for the store widths (u16: +0x4, +0x84,
 * +0x86, +0xe8, +0xf0, +0x1b4, +0x1e4, +0x1ec, +0x204, +0x206, +0x20c, +0x26c..+0x272, +0xc754,
 * +0xc78c, +0xc78e, +0xc7e8, +0xc7f0, +0xc89c..+0xc8a0, +0xca24, +0x14228, +0x14248, +0x14832,
 * +0x14868, +0x1486a, +0x14870, +0x14894, +0x1489c, +0x158d0, +0x15900, +0x15948, +0x159c0,
 * +0x159ec, +0x15a02..+0x15a0a, +0x15a14, +0x15a84, +0x15a86, +0x15b2c, +0x15b4c; byte +0x88)
 * and the call arguments (the decompile gives 0x555f00 two: it takes one).  The case tree of
 * the compiled switch is flattened into one switch; the shared tails the compiler jumped into
 * are the small functions below. */
#include "char_init.h"

#include <string.h>

#include "char_def.h"
#include "char_integrate.h"
#include "char_state.h"
#include "device_init.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "npc_ai.h"
#include "particle_init.h"
#include "rng.h"
#include "room_load.h"

#define IP(p) ((intptr_t)(p))
#define W (*g_eng.world)
#define TRAILER 0x8a9b6cu

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void st8(void *p, uint32_t off, uint8_t v) { ((uint8_t *)p)[off] = v; }
static void stp(void *p, uint32_t off, const void *v) { st32(p, off, (int32_t)(intptr_t)v); }
static const char *S(uint32_t va) { return (const char *)g_eng.data(va); }
static const void *D(uint32_t va) { return g_eng.data(va); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static uint16_t roll(int32_t n, int32_t base) { return (uint16_t)(mul(rnd(), n) / 0x8000 + base); }

/* the loading log: 0x406440 / 0x408b90 on 0x8a6620 while 0x8a6b54 is 0 */
static void step_log(uint32_t msg)
{
    if (*g_eng.quiet == 0) g_eng.log_fail_406440(g_eng.dlog, S(msg), S(TRAILER));
}
static void step_log2(uint32_t msg)
{
    if (*g_eng.quiet == 0) g_eng.h408b90(g_eng.dlog, IP(S(msg)), 0, IP(S(TRAILER)));
}

/* the arguments the cases read (y / tint are rewritten by some) */
typedef struct ci_args {
    int32_t x, y, anchor, f1f8, a8, facing, tint, variant, a13;
    uint32_t code;
    uint16_t track;     /* the dramatist row's (0 = none) */
    int32_t folk;       /* a townsperson: +0x15908 = 1 instead of the +0x2788 list */
    int32_t post;       /* the home-post half width (0 = by the AI mode) */
} ci_args;

#define SETUP(fac, w, h, dx, dy) \
    entity_setup(&c->base, 1, (int32_t)P->code, fac, P->x, P->y, w, h, P->facing, 1, dx, dy, P->anchor, P->tint, 8)
#define TRK(t) entity_track_on(&c->base, 0, (int16_t)(t), 0)
#define NAME(va) strcpy((char *)c->party, S(va))
#define PARTY_MODE(m) st32(c->party, 0xd4, m)
#define F1F4 ld32(c, 0x1f4)

/* the variant's track unless the dramatist row gives one: a12 == 1 / 2 / 3 -> t1 / t2 / t3
 * (0 = that variant falls to the default) */
static uint16_t pick(const ci_args *P, uint16_t d, uint16_t t1, uint16_t t2, uint16_t t3)
{
    if (P->track != 0) return P->track;
    if (P->variant == 1 && t1 != 0) return t1;
    if (P->variant == 2 && t2 != 0) return t2;
    if (P->variant == 3 && t3 != 0) return t3;
    return d;
}

/* a townsperson's tail: the track, the clips, the walk / run (walkers follow mode 12 for a13
 * == 1), the step sound, mode 13, +0x1e8 and the name (0 = none) */
static void folk(wl_char *c, ci_args *P, uint16_t trk, uint32_t clip, int walker, int32_t f1e8, uint32_t name)
{
    TRK(trk);
    char_clips_set(c, D(clip), D(clip), D(clip), 15, 0);
    if (walker) {
        char_walk_set(c, 12000, 800, 800, -800, -800, D(0x629410u));
        char_run_set(c, 24000, D(0x6295a8u));
    }
    P->folk = 1;
    char_step_set(c, 1, 0);
    char_follow_set(c, walker && P->a13 == 1 ? 12 : 13, NULL, 0, 0);
    st32(c, 0x1e8, f1e8);
    if (name != 0) NAME(name);
}

/* the party hold (+0xc790 == 1): the party mode 0, body 0's three flags, +0xc764; else mode 1 */
static void party_hold(wl_char *c, wl_body *b0, int32_t c764)
{
    if (ld32(c, 0xc790) != 1) { PARTY_MODE(1); return; }
    PARTY_MODE(0);
    st8(b0, 0x7e, 1);
    st8(b0, 0x7d, 1);
    st8(b0, 0x7c, 1);
    st32(c, 0xc764, c764);
}

static void guard_post(wl_char *c)
{
    st32(c, 0x15a44, 0);
    st32(c, 0x15a48, 0);
    st32(c, 0x15a50, 500);
    st32(c, 0x15a54, 500);
    st32(c, 0x15a4c, 0);
}

/* 100175 / 100176 / 100177 and 51024 / 51231 / 51232 (on = 0) */
static void wolf(wl_char *c, ci_args *P, int16_t trk, int32_t on, int32_t reach_on, int32_t skill)
{
    SETUP(2, 3200, 6400, -32, -32);
    TRK(trk);
    g_eng.h42f840(c, on);
    char_pace_set(c, 4, on != 0 ? reach_on : 2000, 2000, 400);
    char_skill_learn(c, skill);
    char_ai_skill_add(c, skill, 0, 3, 0, 28000, -3200, 6400, 500, 1000, 20, 10, 0, 24000, 0, 5600, 0);
}

/* 100188 / 100189 / 51261 / 51262 */
static void golem(wl_char *c, ci_args *P, int32_t id)
{
    SETUP(2, 6400, 6400, -32, -32);
    g_eng.h42b9a0(c, id);
    char_pace_set(c, 3, 12800, 3200, 0);
    char_nav_set(c, F1F4, 2000, 20800);
}

/* the 0x42e290 slimes (anchored 2 px lower at anchor 1) */
static void slime(wl_char *c, ci_args *P, int32_t a, int32_t b, int32_t k, int32_t n)
{
    if (P->anchor == 1) P->y += 200;
    SETUP(2, 3200, 3200, -24, -46);
    g_eng.h42e290(c, a, b, k, n);
}

static void l549(wl_char *c)
{
    g_eng.h430400(c, 2);
    guard_post(c);
    st32(c, 0x15a94, 1);
    char_member_pose_set(c, 0, 18, 0);
}

static void l468(wl_char *c, ci_args *P, int16_t trk)
{
    SETUP(2, 3600, 7800, -64, -82);
    TRK(trk);
    g_eng.h430400(c, 2);
}

/* 51071 / 51072 */
static void l442(wl_char *c)
{
    char_clips_set(c, D(0x64e868u), D(0x64ea00u), D(0x64eb98u), 0, 2);
    char_walk_set(c, 24000, 2000, 4000, -800, -1600, D(0x64f060u));
    char_run_set(c, 50000, D(0x64f060u));
    char_hit_set(c, 104, D(0x64eec8u), 0, 1, 0);
    char_jump_set(c, -160000, 6000, 20000, 48000, 1600, 3200, D(0x64eb98u), D(0x64ed30u), D(0x64ed30u), 14);
    char_step_set(c, 1, 0);
    char_clips2_set(c, D(0x62a400u), D(0x62a400u));
    char_move_set(c, 0, 120000, 6000, 120000, 6000, 0, 0, 0);
    char_skill_def(c, 61, 1, 0, 0, 0, 0, 0xf00000, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, S(TRAILER));
    char_skill_def(c, 62, 1, 0, 0, 0, 0, 0xf00000, 700, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, S(TRAILER));
    char_ai_skill_add(c, 61, 0, 1, 0, 5600, 0, 4800, 10, 1000, 10, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 61, 6, 1, 0, 7200, 0, 8000, 5, 1000, 10, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 62, 0, 4, 12800, 20000, -3200, 8000, 100, 1000, 20, 10, 12800, 16000, -1600, 6400, 0);
    uint8_t *r = char_ai_skill_add(c, 62, 1, 4, 4000, 16000, -12000, 10000, 60, 900, 10, 10, 6400, 12000, -11200, 6000, 0);
    if (r != NULL) ai_skill_boxes_add(r, 41600, 12800, -4800, 12800, 42400, 4800, -3200, 9600);
    char_ai_skill_add(c, 62, 7, 2, 9600, 12800, 4800, 12800, 60, 800, 10, 10, 9600, 8000, 6400, 9600, 0);
    char_approach_add(c, 13600, 9600, -2000, 13600, 15000, 4000, 1600, 0, 8000, 0, 16000, 1000);
    char_approach_add(c, 6400, 8000, -8000, 13600, 0, 4000, 1600, 0, 8000, 0, 16000, 1000);
    char_approach_add(c, 3200, 8000, -2000, 13600, 0, 1600, 3200, 0, 8000, 0, 16000, 300);
    char_combo_add(c, 1, 62, 7, 2, 0, 10, 300, 0, 210, 8, 3, 0, 0, 0);
    char_combo_add(c, 1, 62, 1, 2, 0, 10, 300, 0, 210, 8, 1, 0, 0, 0);
    char_combo_add(c, 1, 62, 0, 2, 0, 10, 300, 0, 210, 8, 5, 0, 0, 0);
    char_combo_add(c, 1, 61, 0, 0, 1, 0, 400, 0, 0, 8, 0, 0, 0, 0);
    char_follow_set(c, 8, NULL, 0, 0);
    char_pace_set(c, 3, 0, 1000, 500);
    char_c880_set(c, 12800, 800, 12800);
    char_voice_init(c, 51071, 0, 0);
}

/* 50012 (on = 0) / 59993 */
static void l506(wl_char *c, ci_args *P, int32_t on)
{
    SETUP(0, 2000, 5600, -30, -24);
    TRK(145);
    entity_set_track(&c->base, 3, 146, 0, 0, 0, 0, 0);
    char_npc_default(c, 8000, D(0x62aa60u));
    char_set_look_up(c, 500, D(0x62e3c0u), D(0x62e558u), 78, 300, 0);
    if (on != 0) {
        char_skill_learn(c, 10302);
        char_skill_learn(c, 10301);
        char_skill_learn(c, 20112);
        char_skill_learn(c, 20114);
        guard_post(c);
    } else {
        char_skills_by_code(c, 50012);
        uint8_t *k = char_skill_def(c, 130, 1, 0, 0, 0, 0, 0xf00000, 1300, 10, 0, 0, 0, 20, 0, 0, 1, 0, 0, 0, 0, 3,
                                    S(TRAILER));
        st32(k, 0x2fc, 1);   /* (no NULL check: retail) */
    }
    char_ai_skill_add(c, 10301, 0, 1, 4000, 12800, 0, 6400, 120, 1000, 5, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 10301, 3, 1, 1600, 4800, 0, 6400, 20, 1000, 2, 8, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 10301, 1, 1, 1600, 8000, -9600, 8000, 20, 1000, 2, 8, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 10302, 0, 1, 0, 4000, 0, 4000, 5, 1000, 5, 10, 0, 0, 0, 0, 0);
    uint8_t *r = char_ai_skill_add(c, 10302, 4, 1, -8000, 6000, -800, 4800, 20, 1000, 5, 10, 0, 0, 0, 0, 0);
    if (r != NULL) st32(r, 0x134, 1);
    char_nav_skills_set(c, 10303, 0, 0, 0, 0, 0, 0);
    char_party_ai_skills(c, 200, 5000, 0, 0xf00000);
    st32(c, 0x14898, 1);
    char_combo_add(c, 1, 10301, 3, 2, 0, 10, 300, 0, 210, 8, 3, 0, 0, 0);
    char_combo_add(c, 1, 10301, 1, 2, 0, 10, 300, 0, 210, 8, 1, 0, 0, 0);
    char_combo_add(c, 1, 10301, 0, 2, 0, 10, 300, 0, 210, 8, 5, 0, 0, 0);
    char_combo_add(c, 1, 10302, 4, 2, 0, 10, 300, 0, 210, 8, 6, 0, 0, 0);
    char_combo_add(c, 1, 10302, 0, 0, 1, 0, 400, 0, 0, 8, 0, 0, 0, 0);
    char_combo_add(c, 1, 10303, 0, 1, 0, 10, 200, 40, 200, 6, 3, 0, 0, 0);
    char_combo_add(c, 1, 10303, 2, 1, 0, 10, 200, 40, 200, 5, 3, 0, 0, 0);
    char_15a5c_set(c, 2, 1, -1800, -2600);
    st16(c, 0x15a84, 0);
    st16(c, 0x15a86, 900);
    st16(c, 0xc89c, 60);
    char_voice_init(c, 50012, 1, 200);
    st32(c, 0x1e8, 3800);
    st32(c, 0xc83c, 10);
    char_retarget_set(c, 5, 50);
    if (on != 0) {
        st32(c, 0x1dc, 2);
        int32_t d = W->f2780;
        char_pace_set(c, d == 10 ? 4 : d == 20 ? 6 : 7, 8000, 0, 400);
        char_form0_set(c, 5);
        char_follow_set(c, 9, NULL, 0, 0);
        st32(c, 0x15a94, 1);
        st32(c, 0x15904, 0);
        char_member_pose_set(c, 107, 115, 8000);
    } else {
        char_member_pose_set(c, 107, 111, 4000);
        char_idle_init(c, 400, 400, 400);
        char_idle_add(c, 15);
        char_idle_add(c, 16);
    }
}

/* 100169 (on) / 51042 */
static void l537(wl_char *c, ci_args *P, int32_t on)
{
    SETUP(2, 3200, 7200, -64, -56);
    TRK(324);
    g_eng.h42ff00(c, (intptr_t)P->code);
    char_skill_learn(c, 20126);
    char_party_ai_skills(c, 1000, 1000, 0, 0xf00000);
    char_15a5c_set(c, 5, 1, 1600, -5600);
    if (on != 0) {
        guard_post(c);
        char_member_pose_set(c, 0, 18, 0);
        char_follow_set(c, 9, NULL, 0, 0);
        char_pace_set(c, 4, 5600, 2000, 300);
    } else {
        char_follow_set(c, 8, NULL, 0, 0);
        char_pace_set(c, 2, 5600, 2000, 300);
    }
}

/* an Arche AI record built in place (the inline of 0x4287d0's head) */
static uint8_t *arche_rec(wl_char *c, uint16_t id, uint16_t f11a, int32_t f120, uint16_t f124, int32_t x0, int32_t x1,
                          int32_t y0, int32_t y1)
{
    uint8_t *k = char_skill_find(c, id);
    uint8_t *r = (uint8_t *)c + 0xca28 + 320u * ld16(c, 0x14228);
    if (k == NULL) return NULL;
    st32(r, 0x004, id);
    st32(r, 0x008, 0);
    stp(r, 0x000, k);
    st32(r, 0x00c, 1);
    st16(r, 0x11a, f11a);
    st16(r, 0x118, 0);
    st16(r, 0x11c, 0);
    st32(r, 0x120, f120);
    st16(r, 0x124, f124);
    st16(r, 0x126, f124);
    st16(r, 0x010, 0);
    st16(r, 0x094, 0);
    st32(r, 0x128, 0);
    st32(r, 0x12c, 0);
    st32(r, 0x130, 0);
    st32(r, 0x134, 0);
    st32(r, 0x138, 1000);
    st16(r, 0x13c, 0);
    st16(r, 0x13e, 0);
    ai_skill_boxes_add(r, x0, x1, y0, y1, 0, 0, 0, 0);
    st16(c, 0x14228, (uint16_t)(ld16(c, 0x14228) + 1));
    return r;
}

/* 50010 — Arche, built in place */
static void arche(wl_char *c, ci_args *P)
{
    step_log(0x8551c0u);   /* "Begin Setup" */
    int32_t x = P->x, y = P->y;
    wl_entity *e = &c->base;
    st32(c, 0x1b8, 1);
    st32(c, 0x1bc, 0);
    st32(c, 0x1d4, (int32_t)P->code);
    st32(c, 0x1d0, 1);
    st32(c, 0x1dc, 0);
    st32(c, 0xf4, 0);
    st32(c, 0xf8, 0);
    st32(c, 0x3c, 0);
    st16(c, 0x1ec, 1);
    st32(c, 0x1c4, 0);
    st32(c, 0x1c8, 0);
    st32(c, 0x1cc, 0);
    st32(c, 0x1c0, 1);
    st32(c, 0x1c, 0);
    st32(c, 0x20, 32);
    st32(c, 0x24, 0);
    st32(c, 0x1f0, 0);
    st32(c, 0x1f4, 0);
    st32(c, 0x38, 0);
    for (int i = 0; i < 8; i++) {
        uint32_t o = 0x48 + 20u * i;
        st16(c, o, 0);
        st16(c, o + 2, 0);
        st8(c, o + 4, 0);
        st32(c, o + 8, 0);
        st32(c, o + 0xc, 0);
        st32(c, o + 0x10, 0);
    }
    st16(c, 0xe8, 0);
    st32(c, 0xfc, P->tint);
    st32(c, 0x100, P->tint);
    st32(c, 0x1d8, 0);
    st32(c, 0x104, 0);
    st32(c, 0x108, 0);
    st32(c, 0x1e8, 0);
    st16(c, 0x1e4, 1);
    st32(c, 0x1e0, 0);
    st32(c, 0xc, 0);
    st32(c, 0x10, 0);
    st32(c, 0x28, 2000);
    st32(c, 0x2c, 5600);
    st32(c, 0x30, -30);
    st32(c, 0x34, -24);
    entity_anchor(e, x, y, P->anchor, &x, &y);
    st32(c, 0xec, 0);
    st16(c, 0xf0, 1);
    for (uint16_t m = 0; m < e->nbody; m++) {
        wl_body *b = &e->body[m];
        uint8_t *q = (uint8_t *)b;
        st8(q, 0x00, 1);
        st32(q, 0x04, x);
        st32(q, 0x08, y);
        st32(q, 0x0c, 2000);
        st32(q, 0x10, 5600);
        st32(q, 0x14, 0);
        st32(q, 0x18, 0);
        st32(q, 0x1c, 0);
        st32(q, 0x28, 0);
        st32(q, 0x2c, P->facing);
        st32(q, 0x30, 0);
        st32(q, 0x34, 0);
        st8(q, 0x20, 0);
        st32(q, 0x40, -30);
        st32(q, 0x44, -24);
        st16(q, 0x66, 0);
        st8(q, 0x68, 0);
        st8(q, 0x7c, 0);
        st8(q, 0x7d, 0);
        st8(q, 0x7e, 0);
        st32(q, 0x284, 0);
        st32(q, 0x24, 1);
        body_action_reset(b);
        body_set_state(b, 0);
        body_set_clip(b, NULL);
        st16(q, 0x80, 0);
        if (m != 0) {
            st32(q, 0x28c, ld32(e->body, 0x28c));
            st32(q, 0x288, ld32(e->body, 0x288));
        } else {
            body_region_flags(e->body);
        }
        st32(q, 0x290, 0);
    }
    entity_fx_reset(e, 8);
    void *p0 = (void *)(intptr_t)ld32(c, 0x0);
    if (p0 != NULL) { g_eng.op_delete(p0); st32(c, 0x0, 0); }
    void *p8 = (void *)(intptr_t)ld32(c, 0x8);
    if (p8 != NULL) { parts_free(p8, 1); st32(c, 0x8, 0); }
    st16(c, 0x4, 0);
    entity_set_track(e, 0, 139, 0, 1, 0, 0, 0);
    entity_set_track(e, 1, 140, 0, 1, 0, 0, 0);
    entity_set_track(e, 2, 141, 0, 1, 0, 0, 0);
    st16(c, 0x84, 142);
    st16(c, 0x86, 0);
    st8(c, 0x88, 0);
    st32(c, 0x90, 0);
    st32(c, 0x8c, 0);
    st32(c, 0x94, 0);
    char_clips_set(c, D(0x62a8c8u), D(0x62c578u), D(0x62c710u), 15, 2);
    stp(c, 0x15920, D(0x62af28u));
    st32(c, 0x15984, 1);
    st32(c, 0x15988, 0);
    char_walk_set(c, 24000, 1600, 3200, -800, -1600, D(0x62abf8u));
    char_run_set(c, 48000, D(0x62ad90u));
    char_jump_set(c, -80000, 2000, 8000, 48000, 1600, 3200, D(0x62c0b0u), D(0x62c248u), D(0x62c3e0u), 134);
    char_crouch_set(c, 2400, D(0x62b720u), D(0x62b8b8u), D(0x62ba50u));
    stp(c, 0x159e8, D(0x62bbe8u));
    char_hit_set(c, 99, D(0x62bf18u), 0, 0, 0);
    st16(c, 0x15a02, 98);
    st32(c, 0x15a04, -400);
    stp(c, 0x15924, D(0x62d238u));
    stp(c, 0x15928, D(0x62d3d0u));
    char_carry_set(c, 1, D(0x62d568u), D(0x62d700u), 0, 0, 110);
    char_pace_set(c, 7, 3600, 0, 800);
    char_c880_set(c, 12800, 400, 12800);
    st32(c, 0x15944, 1);
    st16(c, 0x15948, 0);
    st32(c, 0x15a90, 1);
    st32(c, 0x1c, -8);
    st32(c, 0x15a94, 1);
    const uint8_t *t = D(0x6907c0u);   /* 16-byte rows: code, skill */
    for (uint16_t i = 0; ld32(t, 16u * i) != 0; i++)
        if (ld32(t, 16u * i) == (int32_t)P->code) char_skill_learn(c, ld32(t, 16u * i + 4));
    step_log(0x8551b4u);   /* "Setup AI" */
    arche_rec(c, 0x2775, 10, 1000, 10, 0, 4800, 0, 4800);
    uint8_t *r = arche_rec(c, 0x2785, 10, 900, 6, -800, 8000, 0, 5600);
    if (r != NULL) { st32(r, 0x128, 20); st32(r, 0x12c, 0); }
    arche_rec(c, 0x2776, 5, 900, 4, -800, 5600, 1600, 4800);
    arche_rec(c, 0x2778, 5, 900, 10, -3200, 9600, -11200, 12800);
    arche_rec(c, 0x277d, 30, 700, 10, 3200, 4800, 800, 4800);
    arche_rec(c, 0x2786, 300, 600, 10, 9600, 8000, -1600, 5600);
    char_ai_skill_add(c, 10110, 0, 1, 5600, 6400, 0, 5600, 300, 600, 5, 5, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 10103, 0, 1, -3200, 6400, 6400, 12800, 5, 800, 4, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 10120, 0, 1, -5600, 16000, -5600, 11200, 50, 800, 4, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 10101, 6, 1, 0, 6400, 0, 8000, 5, 1000, 10, 10, 0, 0, 0, 0, 0);
    r = char_ai_skill_add(c, 10105, 0, 1, -9600, 6400, 800, 4800, 5, 900, 5, 10, 0, 0, 0, 0, 0);
    if (r != NULL) st32(r, 0x134, 1);
    r = char_ai_skill_add(c, 10122, 0, 1, -12800, 6400, 800, 4800, 10, 900, 10, 10, 0, 0, 0, 0, 0);
    if (r != NULL) st32(r, 0x134, 1);
    r = char_ai_skill_add(c, 10106, 0, 1, 3200, 4800, -800, 4800, 300, 600, 10, 10, 0, 0, 0, 0, 0);
    if (r != NULL) { st32(r, 0x130, 1); st32(r, 0x128, -10); st32(r, 0x12c, 5); }
    r = char_ai_skill_add(c, 10116, 0, 1, -8000, 12800, -11200, 11200, 200, 600, 10, 10, 0, 0, 0, 0, 0);
    if (r != NULL) { st32(r, 0x130, 1); st32(r, 0x128, -10); st32(r, 0x12c, 5); }
    char_ai_skill_add(c, 10111, 0, 1, -1600, 8000, -4000, 8000, 20, 700, 5, 5, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 10112, 0, 1, 3200, 4800, 800, 4800, 30, 600, 5, 5, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 10115, 1, 2, 6400, 12000, -9600, 10000, 60, 900, 25, 5, 8000, 8000, -8000, 6000, 0);
    char_ai_skill_add(c, 10115, 0, 2, 6400, 16000, 0, 6400, 120, 600, 20, 10, 8000, 12800, 800, 3200, 0);
    char_nav_skills_set(c, 10108, 0, 10107, 0, 0, 0, 0);
    char_party_ai_skills(c, 200, 2000, 0, 0xf00000);
    char_approach_add(c, 16000, 9600, -4800, 12800, 40000, 3200, -3200, 0, 6400, 0, 8000, 1000);
    char_approach_add(c, 6400, 6400, -6400, 8000, 0, 3200, 0, 1600, 4000, 0, 8000, 1000);
    char_approach_add(c, 3200, 6400, -3200, 12800, 0, 3200, 4000, 0, 6400, 0, 9600, 300);
    step_log2(0x8551a4u);  /* "Setup Player" */
    st32(c, 0x14898, 1);
    char_combo_add(c, 1, 10113, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    char_combo_add(c, 1, 10110, 0, 1, 0, 10, 300, 0, 200, 8, 1, 5, 0, 0);
    char_combo_add(c, 1, 10118, 0, 1, 0, 10, 300, 0, 200, 8, 5, 1, 0, 0);
    char_combo_add(c, 1, 10106, 0, 1, 0, 10, 300, 0, 200, 8, 3, 5, 0, 0);
    char_combo_add(c, 1, 10116, 0, 1, 0, 10, 300, 0, 200, 8, 3, 6, 0, 0);
    char_combo_add(c, 1, 10122, 0, 1, 0, 10, 300, 50, 200, 8, 6, 0, 0, 0);
    char_combo_add(c, 1, 10108, 0, 1, 0, 10, 200, 40, 200, 6, 3, 0, 0, 0);
    char_combo_add(c, 1, 10107, 0, 1, 0, 10, 200, 40, 200, 5, 3, 0, 0, 0);
    char_combo_add(c, 1, 10111, 0, 2, 0, 10, 300, 0, 400, 8, 1, 0, 0, 0);
    char_combo_add(c, 1, 10112, 0, 2, 0, 10, 300, 0, 400, 8, 5, 0, 0, 0);
    char_combo_add(c, 1, 10105, 0, 2, 0, 10, 300, 0, 210, 8, 6, 0, 0, 0);
    char_combo_add(c, 1, 10117, 0, 2, 0, 10, 300, 0, 210, 8, 3, 0, 0, 0);
    char_combo_add(c, 1, 10102, 0, 2, 0, 10, 300, 0, 210, 8, 3, 0, 0, 0);
    char_combo_add(c, 1, 10103, 0, 2, 0, 10, 300, 0, 210, 8, 3, 0, 0, 0);
    char_combo_add(c, 1, 10120, 0, 2, 0, 10, 300, 0, 210, 8, 1, 0, 0, 0);
    char_combo_add(c, 1, 10109, 0, 2, 0, 10, 300, 0, 210, 8, 5, 0, 0, 0);
    char_combo_add(c, 1, 10104, 0, 2, 0, 10, 300, 0, 210, 8, 1, 0, 0, 0);
    char_combo_add(c, 1, 10101, 0, 0, 1, 0, 400, 0, 0, 8, 0, 0, 0, 0);
    char_member_skill_add(c, 0, 3);
    char_member_skill_add(c, 1, 4);
    char_member_pose_set(c, 107, 111, 4500);
    step_log2(0x855190u);  /* "Setup Voice Manager" */
    char_voice_init(c, 50010, 1, 200);
    char_15a5c_set(c, 3, 1, 0, -400);
    st32(c, 0x1e8, 4000);
    st16(c, 0x15a84, 1000);
    st16(c, 0x15a86, 900);
    char_idle_init(c, 200, 400, 400);
    char_idle_add(c, 11);
    char_idle_add(c, 12);
}

/* 50011 */
static void sana(wl_char *c, ci_args *P)
{
    SETUP(0, 2000, 5600, -30, -24);
    TRK(143);
    entity_set_track(&c->base, 3, 144, 0, 0, 0, 0, 0);
    char_npc_default(c, 18000, D(0x62a8c8u));
    char_set_look_up(c, 500, D(0x6360c8u), NULL, 77, 300, 0);
    char_skills_by_code(c, (int32_t)P->code);
    char_ai_skill_add(c, 10201, 0, 1, -1600, 6400, 0, 5600, 10, 1000, 6, 6, 0, 0, 0, 0, 0);
    uint8_t *r = char_ai_skill_add(c, 10201, 4, 1, -6800, 4000, 0, 5600, 10, 1000, 6, 6, 0, 0, 0, 0, 0);
    if (r != NULL) st32(r, 0x134, 1);
    char_ai_skill_add(c, 10202, 1, 2, 4000, 12000, -12000, 10000, 60, 900, 25, 5, 6400, 8000, -11200, 6000, 0);
    char_ai_skill_add(c, 10202, 8, 2, 12000, 12800, 0, 6400, 160, 750, 20, 10, 12000, 6400, 800, 3200, 0);
    char_ai_skill_add(c, 10202, 7, 2, 9600, 12800, 4800, 12800, 100, 800, 10, 10, 9600, 6400, 4800, 6400, 0);
    char_ai_skill_add(c, 10202, 0, 2, 22000, 12800, 0, 6400, 240, 600, 20, 10, 24000, 6400, 800, 3200, 0);
    char_party_ai_skills(c, 200, 1000, 0, 0xf00000);
    char_nav_skills_set(c, 10303, 0, 0, 0, 0, 0, 0);
    char_approach_add(c, 9600, 22000, -6400, 4800, 0, 22000, -3200, 0, 22000, 0, 4000, 1000);
    char_approach_add(c, 16000, 12800, -3200, 16000, 0, 16000, 6000, 12000, 6400, 6400, 9600, 200);
    st32(c, 0x14898, 1);
    char_combo_add(c, 1, 10202, 1, 2, 0, 10, 300, 0, 210, 8, 1, 0, 0, 0);
    char_combo_add(c, 1, 10202, 8, 2, 0, 10, 300, 0, 210, 8, 3, 0, 0, 0);
    char_combo_add(c, 1, 10202, 0, 2, 0, 10, 300, 0, 210, 8, 5, 0, 0, 0);
    char_combo_add(c, 1, 10201, 4, 2, 0, 10, 300, 0, 210, 8, 6, 0, 0, 0);
    char_combo_add(c, 1, 10201, 0, 0, 1, 0, 400, 0, 0, 8, 0, 0, 0, 0);
    char_combo_add(c, 1, 10303, 0, 1, 0, 10, 200, 40, 200, 6, 3, 0, 0, 0);
    char_combo_add(c, 1, 10303, 2, 1, 0, 10, 200, 40, 200, 5, 3, 0, 0, 0);
    char_member_pose_set(c, 107, 111, 3000);
    char_voice_init(c, 50011, 1, 200);
    st32(c, 0xc83c, 40);
    char_retarget_set(c, 5, 50);
    char_15a5c_set(c, 1, 1, 0, -4200);
    st32(c, 0xc7ec, 1);
    st16(c, 0x15a84, 0);
    st16(c, 0x15a86, 1000);
    st32(c, 0x1e8, 4200);
    char_idle_init(c, 300, 400, 400);
    char_idle_add(c, 13);
    char_idle_add(c, 14);
}

/* 50013 */
static void c50013(wl_char *c, ci_args *P)
{
    SETUP(0, 2000, 2000, -22, -22);
    TRK(336);
    const void *k = D(0x641b88u);
    char_clips_set(c, k, k, k, 0, 0);
    char_jump_set(c, -40000, 1000, 4000, 0, 0, 0, k, k, k, 0);
    char_walk_set(c, 24000, 1600, 3200, -800, -1600, k);
    char_run_set(c, 48000, D(0x641d20u));
    char_turn_set(c, 1, 1);
    char_skill_def(c, 75, 1, 0, 0, 0, 0, 0xf00000, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, S(TRAILER));
    char_skill_def(c, 76, 1, 0, 0, 0, 0, 0xf00000, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, S(TRAILER));
    char_ai_skill_add(c, 75, 0, 0, 0, 6400, -1600, 6400, 200, 1000, 0, 0, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 76, 0, 0, 8000, 6400, -1600, 6400, 400, 1000, 0, 0, 0, 0, 0, 0, 0);
    char_move_set(c, 3, 0, 2000, 40000, 2000, 3200, 0, 0);
    char_pace_set(c, 7, 3200, 0, 0);
    char_nav_set(c, F1F4, 0, 4000);
    st16(c, 0x15a84, 0);
    st16(c, 0x15a86, 0);
    st32(c, 0x1e8, 4500);
    st32(c, 0x15a90, 0);
    st32(c, 0x1590c, 1);
    P->folk = 1;
    st32(c, 0x20, 64);
    char_voice_init(c, 50013, 1, 0);
    char_idle_init(c, 600, 800, 400);
    char_idle_add(c, 17);
}

/* 100178 */
static void c100178(wl_char *c, ci_args *P)
{
    SETUP(2, 10000, 6400, -70, -175);
    TRK(303);
    char_clips_set(c, D(0x655750u), D(0x655db0u), D(0x655a80u), 0, 2);
    char_walk_set(c, 24000, 1600, 3200, -800, -1600, D(0x6558e8u));
    char_jump_set(c, -80000, 2000, 8000, 48000, 1600, 3200, D(0x655a80u), D(0x655c18u), D(0x655c18u), 14);
    char_set_look_up(c, 700, D(0x6560e0u), NULL, 75, 500, 0);
    char_turn_set(c, 1, 1);
    char_hit_set(c, 104, D(0x655c18u), 2, 1, 0);
    char_clips2_set(c, D(0x62a598u), D(0x62a598u));
    char_step_set(c, 3, 148);
    char_skill_def(c, 69, 1, 0, 0, 0, 0, 0xf00000, 1200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, S(TRAILER));
    char_skill_def(c, 70, 1, 5, 150, 0, 0, 0xf00000, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, S(TRAILER));
    char_skill_def(c, 71, 1, 3, 500, 0, 0, 0xf00000, 1000, 0, 0, 0, 0, 40, 0, 0, 1, 0, 0, 0, 0, 3, S(TRAILER));
    char_ai_skill_add(c, 69, 0, 1, 0, 10000, 0, 4800, 5, 1000, 5, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 70, 0, 1, 6000, 10000, -1600, 6400, 20, 1000, 10, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 70, 4, 1, -18000, 8000, -1600, 6400, 20, 1000, 10, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 70, 1, 1, 0, 12000, -12000, 8000, 20, 1000, 10, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 71, 0, 3, 14000, 28000, 0, 6400, 500, 1000, 20, 10, 12000, 24000, 0, 5600, 0);
    st32(c, 0x14898, 2);
    char_combo_add(c, 1, 71, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    char_combo_add(c, 1, 70, 0, 2, 0, 10, 300, 0, 210, 8, 5, 0, 0, 0);
    char_combo_add(c, 1, 70, 4, 2, 0, 10, 300, 0, 210, 8, 6, 0, 0, 0);
    char_combo_add(c, 1, 70, 1, 2, 0, 10, 300, 0, 210, 8, 1, 0, 0, 0);
    char_combo_add(c, 1, 69, 0, 0, 1, 0, 400, 0, 0, 8, 0, 0, 0, 0);
    char_follow_set(c, 5, NULL, 0, 0);
    char_pace_set(c, 4, 2000, 2000, 200);
    char_c880_set(c, 12800, 800, 12800);
    st32(c, 0x15a30, 1200);
    guard_post(c);
    st32(c, 0x15a94, 1);
    char_member_pose_set(c, 0, 18, 0);
    char_voice_init(c, 51028, 0, 0);
}

/* 51091 / 51092: the two caster skills (40 + `id`) */
static void caster(wl_char *c, ci_args *P, int32_t type, int32_t id, int32_t a8, int32_t a9, int32_t f2d4,
                   int32_t id_b0, int32_t id_b4, int32_t id_f2d4)
{
    SETUP(2, 3200, 6400, -48, -48);
    g_eng.h42c130(c, type);
    char_skill_def(c, 40, 2, 9, 0, 1, 0, 0xf00000, a8, a9, 1200, 0, f2d4, 20, 1, 1, 1, 0, 0, 0, 0, 3, S(TRAILER));
    char_skill_def(c, id, 1, id_b0, id_b4, 0, 0, 0xf00000, 0, 0, 0, 0, id_f2d4, 30, 1, 1, 1, 0, 0, 0, 0, 3, S(TRAILER));
    char_ai_skill_add(c, 40, 0, 3, 0, 0, 0, 0, 1500, 1000, 0, 0, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, id, 0, 3, -32000, 64000, -24000, 48000, 1000, 1000, 20, 10, -30000, 60000, -24000, 48000, 0);
    st32(c, 0x14898, 2);
    char_combo_add(c, 1, id, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    char_pace_set(c, 3, 12800, 6400, 0);
    char_nav_set(c, F1F4, -800, 12800);
}

/* the 0x426440 gate of 51022 / 51061 / 51062: one of the system mode, the grid's +0x1c or
 * world+0x27b4 */
static int gated(void)
{
    return sys_by_mode(g_eng.sys, 1, 1, 1) == 0 && grid_f1c(W->grid) == 0 && W->f27b4 == 0;
}

static void fatal(void)
{
    g_eng.h426110(g_eng.log, IP(S(0x855064u)), 1, IP(S(TRAILER)));   /* "Unknown Object Type Character.\n" */
    g_eng.h5bf5db(0);
}

/* the case body; 0 = return without the epilogue */
static int by_code(wl_char *c, ci_args *P, wl_body *b0)
{
    switch (P->code) {
    case 0xc35a: arche(c, P); break;
    case 0xc35b: sana(c, P); break;
    case 0xc35c: l506(c, P, 0); break;
    case 0xc35d: c50013(c, P); break;
    case 0xea59: l506(c, P, 1); break;
    /* ---- townsfolk ---- */
    case 0xc3be: { uint16_t t = pick(P, 212, 213, 0, 0);
        SETUP(0, 2000, 5600, -30, -24); folk(c, P, t, 0x6290e0u, 1, 3500, 0x855150u); st32(c, 0x1c, -8); break; }
    case 0xc3c8: case 0xc3c9: { uint16_t t = P->code == 0xc3c8 ? pick(P, 215, 216, 0, 0) : pick(P, 217, 218, 0, 0);
        SETUP(0, 2000, 5600, -30, -24); folk(c, P, t, 0x6290e0u, 1, 2200, 0x85514cu); st32(c, 0x1c, -8); break; }
    case 0xc3dc: case 0xc3dd: { uint16_t t = P->code == 0xc3dc ? pick(P, 221, 222, 223, 224) : pick(P, 225, 226, 0, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x6290e0u, 0, 1500, 0x855148u); break; }
    case 0xc3e6: case 0xc3eb: { uint16_t t = P->code == 0xc3e6 ? pick(P, 229, 230, 231, 0) : pick(P, 232, 233, 234, 0);
        SETUP(0, 2000, 9600, -30, -32); folk(c, P, t, 0x6290e0u, 0, 1200, 0x855140u); break; }
    case 0xc3f1: case 0xc3f2: { uint16_t t = P->code == 0xc3f1 ? pick(P, 238, 239, 0, 0) : pick(P, 240, 241, 0, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x6290e0u, 0, 0, 0x855138u); break; }
    case 0xc3f0: case 0xc3f3: case 0xc3f4: {
        uint16_t t = P->code == 0xc3f0 ? pick(P, 235, 236, 237, 0) : P->code == 0xc3f3 ? pick(P, 242, 243, 244, 0)
                                                                     : pick(P, 245, 246, 247, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x6290e0u, 0, 1000, 0x855138u); break; }
    case 0xc3f5:
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, 248, 0x6290e0u, 0, 1000, 0x855138u); break;
    case 0xc404: { uint16_t t = pick(P, 249, 250, 251, 252);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x6290e0u, 0, 1000, 0x855138u); break; }
    case 0xc422: case 0xc423: case 0xc424: {
        uint16_t t = P->code == 0xc422 ? pick(P, 147, 148, 0, 0) : P->code == 0xc423 ? pick(P, 149, 150, 0, 0)
                                                                   : pick(P, 151, 152, 0, 0);
        SETUP(0, 2000, 5600, -30, -24); folk(c, P, t, 0x6290e0u, 1, 5000, 0x855184u); st32(c, 0x1c, -8); break; }
    case 0xc42c: case 0xc42d: { uint16_t t = P->code == 0xc42c ? pick(P, 153, 154, 155, 0) : pick(P, 156, 157, 0, 0);
        SETUP(0, 2000, 5600, -30, -24); folk(c, P, t, 0x6290e0u, 1, 4000, 0x85517cu); st32(c, 0x1c, -8); break; }
    case 0xc432: { uint16_t t = pick(P, 162, 163, 164, 0);
        SETUP(0, 2000, 5600, -30, -24); folk(c, P, t, 0x6290e0u, 1, 4000, 0x855174u); st32(c, 0x1c, -8); break; }
    case 0xc440: case 0xc441: case 0xc442: {
        uint16_t t = P->code == 0xc440 ? pick(P, 165, 166, 167, 168) : P->code == 0xc441 ? pick(P, 169, 170, 171, 172)
                                                                       : pick(P, 173, 174, 175, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x6290e0u, 1, 2500, 0x855174u); break; }
    case 0xc443: { uint16_t t = pick(P, 176, 177, 178, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x629278u, 0, 2500, 0x855168u); break; }
    case 0xc454: { uint16_t t = pick(P, 204, 205, 206, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x629278u, 0, 2000, 0x85515cu); break; }
    case 0xc455:
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, 207, 0x6290e0u, 0, 2000, 0x85517cu); break;
    case 0xc468: case 0xc469: { uint16_t t = P->code == 0xc468 ? pick(P, 208, 209, 0, 0) : pick(P, 210, 211, 0, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x6290e0u, 0, 1500, 0x85515cu); break; }
    case 0xc472: case 0xc473: case 0xc474: {
        uint16_t t = P->code == 0xc472 ? pick(P, 184, 185, 186, 0) : P->code == 0xc473 ? pick(P, 187, 188, 189, 0)
                                                                     : pick(P, 190, 191, 192, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x629278u, 0, 2500, 0x855130u); break; }
    case 0xc47c: { uint16_t t = pick(P, 193, 194, 195, 0);
        SETUP(0, 2000, 9600, -30, -32); folk(c, P, t, 0x6290e0u, 0, 2500, 0x855124u); break; }
    case 0xc486: { uint16_t t = pick(P, 196, 197, 0, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x629c08u, 0, 2500, 0x855174u); break; }
    case 0xc490:
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, 198, 0x629c08u, 0, 2500, 0x855174u); break;
    case 0xc49a:
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, 199, 0x629c08u, 0, 2500, 0x855174u); break;
    case 0xc4a4:
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, 200, 0x629c08u, 0, 1000, 0x855138u); break;
    case 0xc4ae:   /* the baby: no clips */
        SETUP(0, 2000, 5600, -30, -24);
        TRK(253);
        char_clips_set(c, NULL, NULL, NULL, 15, 0);
        P->folk = 1;
        char_step_set(c, 1, 0);
        char_follow_set(c, 13, NULL, 0, 0);
        st32(c, 0x1e8, 5000);
        NAME(0x85511cu);
        st32(c, 0x1c, -8);
        char_voice_init(c, 50350, 0, -1000);
        char_idle_voice_set(c, 1, 300);
        break;
    case 0xc4b8: { uint16_t t = pick(P, 254, 255, 0, 0);
        SETUP(0, 2000, 9600, -30, -32); folk(c, P, t, 0x6290e0u, 0, 1000, 0x855110u); break; }
    case 0xc4c2: { uint16_t t = pick(P, 182, 183, 0, 0);
        SETUP(0, 2000, 7600, -30, -20); folk(c, P, t, 0x6290e0u, 0, 2500, 0x85510cu); break; }
    case 0x18756: case 0x18757: case 0x18758:
        SETUP(0, 2000, 5600, -30, -24);
        folk(c, P, (uint16_t)(201 + (P->code - 0x18756)), 0x629c08u, 0, 4000, 0x85517cu);
        break;
    /* ---- animals ---- */
    case 0xe29a:   /* 58010: only when the time of day is not 1..4 */
        if (!tod_off(g_eng.sys)) return 0;
        P->tint = 12;
        SETUP(0, 3200, 3200, 0, 0);
        entity_track_on(&c->base, 0, 326, (int16_t)P->a8);
        char_clips_set(c, D(0x65ddf0u), D(0x65ddf0u), D(0x65ddf0u), 0, 0);
        char_jump_set(c, -32000, 1000, 4000, 10000, 1000, 2000, D(0x65ddf0u), D(0x65ddf0u), D(0x65ddf0u), 0);
        char_move_set(c, 1, 16000, 2000, 40000, 8000, 4800, 0, 0);
        char_pace_set(c, 2, 0, 0, 0);
        char_nav_set(c, F1F4, 0, 8000);
        st32(c, 0xc850, 1);
        st32(c, 0x1590c, 1);
        P->folk = 1;
        P->post = 8000;
        break;
    case 0xe2a4:   /* 58020: the chicken */
        SETUP(0, 2000, 2000, -14, -28);
        NAME(0x855100u);
        TRK(327);
        char_clips_set(c, D(0x65df88u), D(0x65df88u), D(0x65df88u), 0, 0);
        char_walk_set(c, 8000, 1600, 3200, -1600, -3200, D(0x65df88u));
        char_follow_set(c, 12, NULL, 0, 0);
        P->folk = 1;
        st32(c, 0x1e8, 5000);
        char_voice_init(c, 58020, 0, -1000);
        char_idle_voice_set(c, 1, 100);
        break;
    case 0xe2a5: { uint16_t t = pick(P, 332, 333, 0, 0);
        SETUP(0, 3200, 3200, -16, -32);
        TRK(t);
        char_clips_set(c, D(0x629da0u), D(0x629da0u), D(0x629da0u), 15, 0);
        char_walk_set(c, 12000, 800, 800, -800, -800, D(0x629410u));
        char_run_set(c, 24000, D(0x6295a8u));
        P->folk = 1;
        char_step_set(c, 1, 0);
        char_follow_set(c, P->a13 == 1 ? 12 : 13, NULL, 0, 0);
        st32(c, 0x1e8, 2000);
        NAME(0x855108u);
        char_voice_init(c, 58021, 0, -1000);
        char_idle_voice_set(c, 1, 200);
        break; }
    case 0xe2a6: { uint16_t t = pick(P, 334, 335, 0, 0);
        SETUP(0, 3200, 3200, 0, 0);
        folk(c, P, t, 0x62a0d0u, 0, 3000, 0x8550fcu);
        char_voice_init(c, 58022, 0, -1000);
        char_idle_voice_set(c, 1, 200);
        break; }
    case 0xe2a7:   /* 58023: the goat */
        SETUP(0, 6400, 6400, -8, -16);
        folk(c, P, 328, 0x629f38u, 0, 0, 0x8550ecu);
        char_voice_init(c, 58023, 0, -1000);
        char_idle_voice_set(c, 1, 100);
        break;
    case 0xe2a8: { uint16_t t = pick(P, 329, 330, 331, 0);   /* 58024: the horse */
        SETUP(0, 9600, 9600, -16, -32);
        folk(c, P, t, 0x62a268u, 0, 0, 0x8550f4u);
        char_voice_init(c, 58024, 0, -1000);
        char_idle_voice_set(c, 1, 200);
        break; }
    /* ---- the per-type inits ---- */
    case 0xc742: slime(c, P, 268, 0, 0, 0); break;
    case 0xc743: case 0x18744: slime(c, P, 270, 0, 0, 0); break;
    case 0xc744: slime(c, P, 271, 272, 0, 0); break;
    case 0xc745: slime(c, P, 273, 274, 5, 100); break;
    case 0xc746: case 0x1875a: slime(c, P, 275, 276, 0, 10); break;
    case 0xc747: slime(c, P, 277, 278, 3, 100); break;
    case 0xc74e: case 0xc775: case 0xc776:
        if (gated()) return 0;
        SETUP(2, 4000, 4000, -20, -20);
        g_eng.h42c970(c, P->code == 0xc74e ? 288 : P->code == 0xc775 ? 289 : 290);
        break;
    case 0xc74f: case 0xc76b: case 0xc76c:
        SETUP(2, 5600, 4000, -12, -40);
        if (P->code == 0xc74f) { TRK(294); g_eng.h42fbf0(c, (intptr_t)P->code, 0, 0); }
        else if (P->code == 0xc76b) {
            TRK(295); g_eng.h42fbf0(c, (intptr_t)P->code, 3, 150);
            st32(c, 0x15a90, 0);
            st32(c, 0x1590c, 0);
        } else { TRK(296); g_eng.h42fbf0(c, (intptr_t)P->code, 5, 100); }
        break;
    case 0xc750: wolf(c, P, 291, 0, 16000, 132); break;
    case 0x1874f: wolf(c, P, 291, 1, 16000, 132); break;
    case 0xc81f: wolf(c, P, 292, 0, 8000, 131); break;
    case 0x18750: wolf(c, P, 292, 1, 8000, 131); break;
    case 0xc820: wolf(c, P, 293, 0, 2000, 133); break;
    case 0x18751: wolf(c, P, 293, 1, 2000, 133); break;
    case 0xc751:
        g_eng.h42c510(c, 0, 0, 1, 500, 0);
        SETUP(2, 4000, 4000, -20, -20);
        TRK(297);
        break;
    case 0xc815:
        SETUP(2, 4000, 4000, -20, -20);
        TRK(298);
        g_eng.h42c510(c, 3, 150, 1, 500, 0);
        break;
    case 0xc753: case 0xc834:
        SETUP(2, 3600, 8000, -48, -48);
        g_eng.h428c00(c, (intptr_t)P->code, 1, P->code == 0xc753 ? 301 : 302);
        party_hold(c, b0, 0);
        break;
    case 0xc835:
        SETUP(2, 3600, 8000, -48, -48);
        g_eng.h428c00(c, (intptr_t)P->code, 1, 300);
        party_hold(c, b0, 0);
        char_skill_learn(c, 20103);
        char_skill_learn(c, 47);
        char_skill_learn(c, 20123);
        char_party_ai_skills(c, 5000, 5000, 0, 1);
        char_follow_set(c, 9, NULL, 0, 0);
        char_pace_set(c, 4, 2000, 2000, 200);
        break;
    case 0x18745: case 0x18746: case 0x18747: case 0x18748: {
        int hero = P->code >= 0x18747;
        SETUP(2, 3600, 8000, -48, -48);
        g_eng.h428c00(c, (intptr_t)P->code, 0, hero ? 300 : 299);
        char_skill_learn(c, 20103);
        if (hero) { char_skill_learn(c, 47); char_skill_learn(c, 20123); }
        char_party_ai_skills(c, 5000, 5000, 0, 1);
        guard_post(c);
        st32(c, 0x15904, 0);
        char_member_pose_set(c, 0, 65, 0);
        st32(c, 0x15a94, 1);
        char_follow_set(c, 9, NULL, 0, 0);
        char_pace_set(c, 4, 2000, 2000, 200);
        party_hold(c, b0, 0);
        break; }
    case 0xc756: case 0xc757: case 0x1874b:
        SETUP(2, 3200, 5600, -32, -40);
        g_eng.h42cce0(c, P->code == 0xc756 ? 314 : 315);
        if (P->code == 0x1874b) {
            char_follow_set(c, 9, NULL, 0, 0);
            char_pace_set(c, 4, 2000, 2000, 200);
            st32(c, 0x15a94, 1);
        } else {
            char_follow_set(c, 8, NULL, 0, 0);
            char_pace_set(c, P->code == 0xc756 ? 2 : 5, 2000, 2000, 200);
        }
        break;
    case 0xc760: case 0xc761:
        SETUP(2, 3200, 7200, -64, -56);
        TRK(P->code == 0xc760 ? 322 : 323);
        g_eng.h42ff00(c, (intptr_t)P->code);
        if (P->code == 0xc760) {
            char_follow_set(c, 5, NULL, 0, 0);
            char_pace_set(c, 2, 5600, 2000, 300);
        } else {
            char_skill_learn(c, 20101);
            char_skill_learn(c, 20102);
            char_party_ai_skills(c, 1000, 1000, 0, 0xf00000);
            char_15a5c_set(c, 5, 1, 1600, -5600);
            char_follow_set(c, 5, NULL, 0, 0);
            char_pace_set(c, 2, 16000, 2000, 300);
        }
        break;
    case 0xc762: l537(c, P, 0); break;
    case 0x18749: l537(c, P, 1); break;
    case 0xc763:
        SETUP(2, 3200, 7200, -64, -56);
        TRK(325);
        g_eng.h42ff00(c, 0xc763);
        char_skill_learn(c, 20114);
        char_skill_learn(c, 20125);
        char_party_ai_skills(c, 1000, 1000, 0, 0xf00000);
        char_15a5c_set(c, 6, 1, 1600, -5600);
        char_follow_set(c, 8, NULL, 0, 0);
        char_pace_set(c, 3, 5600, 2000, 300);
        break;
    case 0xc77f: case 0xc780:
        SETUP(2, 4000, 6400, -28, -32);
        TRK(P->code == 0xc77f ? 319 : 320);
        l442(c);
        break;
    case 0xc789: case 0xc78a: case 0xc78b:
        SETUP(2, 4000, 4800, -44, -48);
        g_eng.h42da50(c, (intptr_t)P->code, 279 + (int32_t)(P->code - 0xc789));
        party_hold(c, b0, 1);
        break;
    case 0x18759:
        SETUP(2, 8000, 9600, -72, -96);
        g_eng.h42d3c0(c);
        party_hold(c, b0, 1);
        break;
    case 0xc792:
        SETUP(2, 3200, 6400, -48, -48);
        g_eng.h42c130(c, 284);
        char_pace_set(c, 3, 6400, 800, 0);
        char_nav_set(c, F1F4, 0, 12800);
        break;
    case 0xc793: caster(c, P, 283, 41, 200, 100, 8, 6, 400, 5); break;
    case 0xc794: caster(c, P, 285, 42, 360, 180, 12, 1, 600, 6); break;
    case 0xc79c: l468(c, P, 262); break;
    case 0xc7a6: l468(c, P, 266); break;
    case 0xc7a7: l468(c, P, 267); break;
    case 0xc7b0: l468(c, P, 264); break;
    case 0xc7b1: l468(c, P, 265); break;
    case 0xc7b2: l468(c, P, 263); break;
    case 0x1874c: case 0x1874d:
        SETUP(2, 3600, 7800, -64, -82);
        TRK(P->code == 0x1874c ? 262 : 267);
        l549(c);
        break;
    case 0xc80b: case 0xc80c: case 0xc80d:
        SETUP(2, 6400, 4800, -32, -80);
        g_eng.h42a500(c, P->code == 0xc80b ? 309 : P->code == 0xc80c ? 311 : 312);
        char_pace_set(c, P->code == 0xc80b ? 4 : 5, 2000, 2000, 200);
        break;
    case 0x1875b:
        SETUP(2, 6400, 4800, -32, -80);
        g_eng.h42a500(c, 313);
        char_pace_set(c, 7, 2000, 2000, 200);
        st32(c, 0x15a30, 500);
        st32(c, 0x15a38, 1500);
        break;
    case 0xc829: case 0xc82a:
        SETUP(2, 3200, 4800, -24, -32);
        g_eng.h429db0(c, P->code == 0xc829 ? 316 : 317);
        break;
    case 0x1874a:
        SETUP(2, 3200, 4800, -24, -32);
        g_eng.h429db0(c, 316);
        guard_post(c);
        char_member_pose_set(c, 0, 18, 0);
        char_follow_set(c, 9, NULL, 0, 0);
        char_pace_set(c, 6, 16000, 2000, 200);
        break;
    case 0xc83d: case 0x1875c: golem(c, P, 286); break;
    case 0xc83e: case 0x1875d: golem(c, P, 287); break;
    case 0x1874e:
        SETUP(2, 2400, 8800, -52, -28);
        g_eng.h429060(c);
        break;
    case 0x18752: c100178(c, P); break;
    case 0x18753: case 0x18754:
        SETUP(2, 12000, 12000, 0, -40);
        g_eng.h42a9a0(c);
        break;
    default: fatal(); break;
    }
    return 1;
}

void char_init(wl_char *c, int32_t x, int32_t y, int32_t anchor, int32_t f1f8, int32_t f1fc, int32_t code,
               int32_t a8, int32_t facing, int32_t handle, int32_t a11, int32_t variant, int32_t a13, int32_t a14,
               int32_t a15)
{
    wl_body *b0 = c->base.body;
    ci_args P = { x, y, anchor, f1f8, a8, facing, code, variant, a13, 0, 0, 0, 0 };
    step_log(0x855244u);   /* "Create Character Begin" */
    st32(c, 0x1f8, f1f8);
    st32(c, 0x1fc, f1fc);
    step_log(0x855230u);   /* "Get Dramatist Info" */
    const uint8_t *dram = NULL;
    uint32_t cd = (uint32_t)code;
    if (handle != 0) {
        const uint8_t *t = D(0x6b6ea8u);   /* 52-byte rows: handle, code, name +8, u16 track +0x30 */
        for (uint16_t i = 0;; i++) {
            int32_t k = ld32(t, 52u * i);
            if (k == 0) break;
            if (k == handle) { dram = t + 52u * i; break; }
        }
        if (dram != NULL) {
            if (f1f8 == 1 || code == 0) cd = (uint32_t)ld32(dram, 4);
            P.track = ld16(dram, 0x30);
        }
    }
    P.code = cd;
    if (a11 == 0) P.tint = 13;
    else if (a11 == 1) P.tint = 7;
    memset((uint8_t *)c + 0x158d0, 0, 0x1d8);
    st32(c, 0x268, 0);
    st32(c, 0x15b5c, 0);
    st32(c, 0x14858, 0);
    st32(c, 0x14854, 0);
    st32(c, 0x1485c, 0);
    st32(c, 0x14860, 0);
    st16(c, 0x14868, 0);
    st32(c, 0x14864, 0);
    st16(c, 0x1486a, 0);
    st32(c, 0x1486c, 0);
    st16(c, 0x14870, 0);
    st16(c, 0x204, 0);
    st32(c, 0x15ab0, 0);
    st32(c, 0xc758, 0);
    st32(c, 0x14240, 0);
    st32(c, 0x1424c, 0);
    st32(c, 0x14250, 0);
    st32(c, 0x14254, 0);
    st32(c, 0x14244, c->base.body->facing);
    st16(c, 0x14248, 0);
    st32(c, 0x14258, 0);
    st16(c, 0x14228, 0);
    st16(c, 0xca24, 0);
    st16(c, 0xc754, 0);
    st32(c, 0xc784, 0);
    st32(c, 0xc788, 0);
    st16(c, 0x26e, 0);
    st16(c, 0x26c, 0);
    st16(c, 0xc78c, 0);
    st16(c, 0xc78e, 0);
    st32(c, 0x15a90, 1);
    st32(c, 0xc7ec, 0);
    st16(c, 0xc7f0, 0);
    st32(c, 0xc794, 0);
    st16(c, 0x270, 0);
    st32(c, 0x264, 0);
    st32(c, 0xc76c, 0);
    st32(c, 0x158f4, 0);
    st32(c, 0x158f8, 16);
    memset((uint8_t *)c + 0x158a8, 0, 0x28);
    st32(c, 0xc764, 0);
    st32(c, 0xc768, a14);
    st32(c, 0xc790, a13);
    st32(c, 0x15b60, 0);
    st16(c, 0x272, 0);
    st16(c, 0x206, 0);
    st32(c, 0x15a78, 0);
    st32(c, 0x15a7c, 0);
    st16(c, 0x15b2c, 0);
    wl_voiceset *v = c->voice;
    if (v != NULL) {
        voice_set_free(v);
        g_eng.op_delete(v);
        c->voice = NULL;
    }
    st32(c, 0x15b58, 0);
    step_log(0x85521cu);   /* "InitCharacterMemory" */
    memset((uint8_t *)c + 0xc7fc, 0, 0x28);
    st32(c, 0xc824, 0);
    st32(c, 0xc828, 0);
    st32(c, 0xc798, 0);
    st32(c, 0xc79c, 0);
    st32(c, 0xc7a0, 0);
    st32(c, 0xc7a4, 0);
    entity_snd_free(&c->base);
    st16(c, 0x1b4, 5);
    void *snd = g_eng.op_new(0xb4);
    stp(c, 0x1b0, snd);
    memset(snd, 0, 36u * ld16(c, 0x1b4));
    st32(c, 0xc7f4, 0);
    st32(c, 0xc7f8, 0);
    st32(c, 0x15a98, 0);
    st32(c, 0x15a9c, 0);
    st32(c, 0x15aa0, 0);
    st32(c, 0x15aa4, 0);
    st32(c, 0x15908, 0);
    st32(c, 0x15a94, 0);
    step_log(0x855204u);   /* "_InitCharacterMoment" */
    st32(c, 0xc7d8, 0);
    st32(c, 0xc7dc, 0);
    st32(c, 0xc7e4, 0);
    st32(c, 0xc7e0, 0);
    st16(c, 0xc7e8, 0);
    for (int i = 0; i < 12; i++) st32(c, 0xc7a8 + 4u * i, 1000);
    for (int i = 0; i < 8; i++) { st32(c, 0x15aec + 8u * i, 0); st16(c, 0x15af0 + 8u * i, 0); }
    st16(c, 0x1489c, 0);
    st32(c, 0x14898, 0);
    st32(c, 0x158a0, 1);
    st16(c, 0x14894, 0);
    step_log(0x8551ecu);   /* "_SetupCharacterDefault" */
    st16(c, 0x158d0, 0);
    st32(c, 0x1590c, 0);
    char_move_set(c, 0, 64000, 4000, 64000, 4000, 0, 0, 0);
    st32(c, 0x15968, 0);
    st32(c, 0x1598c, 0);
    st32(c, 0x15998, 0);
    st32(c, 0x15984, 0);
    st32(c, 0x159d4, 0);
    st16(c, 0x159ec, 0);
    st16(c, 0x15a02, 0);
    st16(c, 0x15a08, 0);
    st16(c, 0x15a0a, 120);
    st32(c, 0x15a0c, 6000);
    st32(c, 0x15a10, 0);
    st32(c, 0x15a28, 0);
    st16(c, 0x159c0, 0);
    char_15a5c_set(c, 0, 0, 0, 0);
    st16(c, 0x15a14, 0);
    st32(c, 0x15a80, 1);
    st32(c, 0x15944, 0);
    st32(c, 0x158fc, 1);
    st16(c, 0x15900, 0);
    st32(c, 0x15904, 1);
    st16(c, 0x15a84, 1000);
    st16(c, 0x15a86, 1000);
    for (int i = 0; i < 12; i++) st32(c, 0x15a2c + 4u * i, 1000);
    /* the base stats, the HP / MP jittered by the difficulty */
    uint8_t *st = (uint8_t *)c + 0x274;
    c->party = (wl_party *)st;
    char_stats_init(st, (int32_t)cd, 1);
    int32_t hp = ld32(st, 0x58), d, s;
    switch (W->f2780) {
    case 10: { int32_t q = mul(hp, 400) / 1000, t = q >= 8 ? q : 8; d = -(hp / 2 < t ? hp / 2 : t); s = -4; break; }
    case 20: { int32_t q = mul(hp, 250) / 1000, t = q >= 4 ? q : 4; d = -(hp / 2 < t ? hp / 2 : t); s = -2; break; }
    case 40: { int32_t q = mul(hp, 100) / 1000; d = q >= 4 ? q : 4; s = 4; break; }
    case 50: { int32_t q = mul(hp, 150) / 1000; d = q >= 8 ? q : 8; s = 8; break; }
    default: d = 0; s = 0; break;
    }
    int32_t hp2 = mul(d + hp, 1000) / 1000;
    if (hp2 < 1) hp2 = 1;
    int32_t t64 = s + ld32(st, 0x64);
    st32(st, 0x64, t64 < 0 ? 0 : t64);
    uint8_t *pt = (uint8_t *)c->party;
    int32_t t6c = s + ld32(pt, 0x6c);
    st32(pt, 0x6c, t6c < 0 ? 0 : t6c);
    int32_t r = mul(rnd(), 1000) / 0x8000;
    st32(c->party, 0x58, stat_jitter(hp2, 1000, r));
    pt = (uint8_t *)c->party;
    st32(pt, 0x54, ld32(pt, 0x9c) + ld32(pt, 0x84) + ld32(pt, 0x58));
    r = mul(rnd(), 1000) / 0x8000;
    pt = (uint8_t *)c->party;
    st32(c->party, 0x60, stat_jitter(ld32(pt, 0x60), 1000, r));
    pt = (uint8_t *)c->party;
    st32(pt, 0x5c, ld32(pt, 0xa0) + ld32(pt, 0x88) + ld32(pt, 0x60));
    st32(c, 0x208, 0);
    st16(c, 0x20c, 0);
    st32(c, 0xc848, 0);
    st32(c, 0xc84c, 1);
    st32(c, 0xc850, 0);
    st32(c, 0xc880, 0);
    st32(c, 0xc870, a15);
    st16(c, 0xc89c, 0);
    for (uint32_t o = 0xc854; o <= 0xc86c; o += 4) st32(c, o, 0);
    st16(c, 0xc89e, 5);
    st16(c, 0xc8a0, 20);
    {   /* the pacing (retail evaluates the last argument first) */
        uint16_t len = roll(50, 950), pch = roll(100, 750), c878 = roll(100, 750), c876 = roll(50, 950);
        uint16_t wait = roll(100, 650);
        char_pace_raw(c, 0, 0, 1000, wait, c876, c878, pch, len, 0);
    }
    st16(c, 0x14832, 0);
    st32(c, 0x15968, 1);
    for (uint32_t o = 0x1596c; o <= 0x15980; o += 4) st32(c, o, 0);
    if (*g_eng.quiet == 0) {
        g_eng.log_fail_406440(g_eng.dlog, S(0x8551dcu), S(TRAILER));   /* "Object ID Case" */
        step_log(0x8551ccu);                                           /* "SetBrainType" */
    }
    st32(c, 0x158a4, 0);
    st32(c, 0x200, 1);
    char_ai_reset(c, 1);

    if (!by_code(c, &P, b0)) return;

    step_log2(0x8550d0u);  /* "Create Character-Case End" */
    if (P.folk) st32(c, 0x15908, 1);
    else wl_list_push(W->list_2788, &c->base);
    wl_list_push(W->list_2794, &c->base);
    if (ld32(c, 0x1f8) != 1 && anchor == 3) b0->y -= ld32(c, 0x1f4);
    step_log2(0x8550b8u);  /* "Create Character-Init2" */
    char_ai_reset(c, 1);
    char_ground_probe(c);
    entity_set_handle(&c->base, handle);
    char_party_mode_set(c, ld32(c->party, 0xd4));
    int32_t post = P.post;
    if (post == 0) {
        switch (ld32(c, 0x14240)) {
        case 5: post = 32000; break;
        case 6: post = 16000; break;
        case 12: post = 20000; break;
        default: break;
        }
    }
    step_log2(0x85509cu);  /* "Create Character-Start Pos" */
    st32(c, 0x14264, part_right(&c->base, 0) + post);
    st32(c, 0x14268, part_left(&c->base, 0) - post);
    st32(c, 0x1426c, part_bottom(&c->base, 0));
    if (dram != NULL && dram[8] != 0) strcpy((char *)c->party, (const char *)dram + 8);
    int32_t mm = ld32(c, 0x1594c);
    if (mm > 0 && (mm <= 2 || mm == 3)) {
        if (mm == 3) st32(c, 0x38, 1);
        st16(c, 0x1ec, 0);
    }
    st32(c, 0x15910, P.tint);
    st32(c, 0xc77c, b0->facing);
    st32(c, 0xc780, b0->x);
    body_set_clip(b0, (const void *)(intptr_t)ld32(c, 0x15914));
    body_clip_jitter(b0);
    int32_t g = *(const int32_t *)D(0x8a940cu);
    if (g != 0 && f1f8 == 1) {
        st32(c->party, 0xf8, g);
        st32(c->party, 0xfc, 70);
    }
    st16(c, 0x1e4, 2);
    uint16_t n = ld16(c, 0x15b2c);
    if (n != 0) st16(c, 0x15b4c, (uint16_t)(mul(rnd(), n) / 0x8000));
    step_log2(0x855084u);  /* "Create Character-Return" */
}
