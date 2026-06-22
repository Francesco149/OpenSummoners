/*
 * test_sword_trail.c — host tests for src/sword_trail.c (the freeroam UP-attack
 * sword-tip TRAIL, res 0x40b sparkles, chip 2c-2).
 *
 * The spec is the captured ground truth (sword2.osr, the first up-attack):
 * 2 sparkles/sim-tick on the captured tip-arc for swing-ticks 9-17 (18 total),
 * each aging frames 24->31 (dur-2 8-frame one-shot, ~16t) then expiring, all at
 * a constant additive ramp_b descriptor.  findings/freeroam-sword-attack.md.
 */
#include "sword_trail.h"
#include "actor_render.h"
#include "anim_clip.h"
#include "character.h"
#include "draw_pool.h"
#include "particle.h"
#include "t.h"

#include <string.h>

/* Pack (bank, frame) into a non-zero cel handle (mirrors test_particle.c). */
static uint32_t resolve_pack(uint16_t bank, uint16_t frame, void *ud)
{
    (void)ud;
    return ((uint32_t)bank << 16) | frame;
}

static sword_trail g_t;

/* Arche's freeroam render anchor (main.c) + an arbitrary stationary world pos. */
#define BX (-30)
#define BY (-24)
#define WX 50000
#define WY 27000

/* ---- the emit WINDOW: only swing-ticks 9..17 spawn (2 each) ---------------- */

int test_sword_trail_emit_window(void)
{
    sword_trail_reset(&g_t);
    /* Before the window: nothing. */
    for (int st = 0; st < SWORD_TRAIL_EMIT_START; st++)
        sword_trail_emit(&g_t, st, WX, WY, BX, BY, CHAR_FACE_RIGHT);
    int alive = 0;
    for (int i = 0; i < SWORD_TRAIL_SLOTS; i++) alive += g_t.states[i].active;
    T_ASSERT_EQ_I(alive, 0);

    /* The 9-tick window: 2 sparkles per tick = 18 total. */
    for (int st = SWORD_TRAIL_EMIT_START;
         st < SWORD_TRAIL_EMIT_START + SWORD_TRAIL_EMIT_TICKS; st++)
        sword_trail_emit(&g_t, st, WX, WY, BX, BY, CHAR_FACE_RIGHT);
    alive = 0;
    for (int i = 0; i < SWORD_TRAIL_SLOTS; i++) alive += g_t.states[i].active;
    T_ASSERT_EQ_I(alive, 2 * SWORD_TRAIL_EMIT_TICKS);

    /* After the window: still nothing new. */
    sword_trail_emit(&g_t, SWORD_TRAIL_EMIT_START + SWORD_TRAIL_EMIT_TICKS,
                     WX, WY, BX, BY, CHAR_FACE_RIGHT);
    alive = 0;
    for (int i = 0; i < SWORD_TRAIL_SLOTS; i++) alive += g_t.states[i].active;
    T_ASSERT_EQ_I(alive, 2 * SWORD_TRAIL_EMIT_TICKS);
    return 0;
}

/* ---- spawn config: bank 0x1a4, frame_base 24, clip, layer 11, anchor ------- */

int test_sword_trail_spawn_config(void)
{
    sword_trail_reset(&g_t);
    /* swing-tick 9 (k=0): sparkle A tip (+36,+53), sparkle B (+42,+52). */
    sword_trail_emit(&g_t, 9, WX, WY, BX, BY, CHAR_FACE_RIGHT);

    /* Round-robin alloc fills slots 0,1 first. */
    const actor *a0 = &g_t.actors[0];
    const actor_render_state *r0 = &g_t.states[0];
    T_ASSERT(r0->active);
    T_ASSERT_EQ_I(a0->sprite_table[0].bank, (int)SWORD_TRAIL_BANK);   /* 0x1a4 */
    T_ASSERT_EQ_I(a0->sprite_table[0].frame_base, 24);
    T_ASSERT_EQ_I((int)a0->layer, 11);
    T_ASSERT(r0->clip != NULL);
    T_ASSERT_EQ_I(r0->facing, CHAR_FACE_RIGHT);
    T_ASSERT_EQ_I(r0->world_x, WX);
    T_ASSERT_EQ_I(r0->world_y, WY);
    /* dst_base = anchor + tip - fr24-origin(6).  A: (-30+36-6, -24+53-6). */
    T_ASSERT_EQ_I(r0->dst_base_x, BX + 36 - 6);   /* = 0   */
    T_ASSERT_EQ_I(r0->dst_base_y, BY + 53 - 6);   /* = 23  */
    /* B: (-30+42-6, -24+52-6). */
    T_ASSERT_EQ_I(g_t.states[1].dst_base_x, BX + 42 - 6);
    T_ASSERT_EQ_I(g_t.states[1].dst_base_y, BY + 52 - 6);
    return 0;
}

/* ---- LEFT facing mirrors the tip dx about the anchor (dy unchanged) -------- */

int test_sword_trail_left_mirror(void)
{
    sword_trail_reset(&g_t);
    sword_trail_emit(&g_t, 9, WX, WY, BX, BY, CHAR_FACE_LEFT);
    /* A tip (+36,+53) -> dx negated: (-30 + (-36) - 6, -24 + 53 - 6). */
    T_ASSERT_EQ_I(g_t.states[0].dst_base_x, BX - 36 - 6);
    T_ASSERT_EQ_I(g_t.states[0].dst_base_y, BY + 53 - 6);   /* dy unchanged */
    return 0;
}

/* ---- aging: the clip runs 24->31 then the slot expires --------------------- */

int test_sword_trail_ages_and_expires(void)
{
    sword_trail_reset(&g_t);
    sword_trail_emit(&g_t, 9, WX, WY, BX, BY, CHAR_FACE_RIGHT);
    int slot = 0;
    T_ASSERT(g_t.states[slot].active);
    T_ASSERT_EQ_I(g_t.states[slot].frame, 0);  /* frame index 0 = sprite 24 */

    /* Step until it expires; track the max frame index reached (must hit 7 =
     * sprite 31) and that it eventually goes inactive (~16t lifetime). */
    int max_frame = 0, expired_at = -1;
    for (int tick = 1; tick <= 40; tick++) {
        sword_trail_step(&g_t);
        if (g_t.states[slot].active) {
            if (g_t.states[slot].frame > max_frame)
                max_frame = g_t.states[slot].frame;
        } else { expired_at = tick; break; }
    }
    T_ASSERT_EQ_I(max_frame, 7);            /* reached sprite 31 (24+7) */
    T_ASSERT(expired_at >= 14 && expired_at <= 18);  /* ~16t one-shot */
    return 0;
}

/* ---- render: every live sparkle emits a MODE-1 ramp_b node ----------------- */

int test_sword_trail_render_emits(void)
{
    sword_trail_reset(&g_t);
    sword_trail_emit(&g_t, 9, WX, WY, BX, BY, CHAR_FACE_RIGHT);   /* 2 sparkles */

    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    int n = sword_trail_render(&g_t, &dp, resolve_pack, NULL);
    T_ASSERT_EQ_I(n, 2);

    /* The nodes land on layer 11, mode 1, param8 = the additive ramp_A index 19
     * (the constant trail blend, LUT 727d856f = g_ramp_a[19]; RAMP_B bit CLEAR). */
    const draw_layer *L = &dp.layers[11];
    T_ASSERT_EQ_I(L->count, 2);
    T_ASSERT_EQ_I((int)L->nodes[0].mode, 1);
    T_ASSERT((L->nodes[0].param8 & PARTICLE_PARAM8_RAMP_B) == 0);  /* ramp_A */
    T_ASSERT_EQ_I((int)L->nodes[0].param8, 19);                    /* g_ramp_a[19] */
    /* The cel is bank 0x1a4 frame 24 (resolve_pack). */
    T_ASSERT_EQ_I((int)(L->nodes[0].sprite >> 16), (int)SWORD_TRAIL_BANK);
    T_ASSERT_EQ_I((int)(L->nodes[0].sprite & 0xffff), 24);
    draw_pool_free(&dp);
    return 0;
}
