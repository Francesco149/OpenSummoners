/*
 * test_particle.c — host tests for src/particle.c (the fountain WATER particle,
 * code 0x18708: alloc/config FUN_00557370/FUN_00557550, emitter FUN_0054f980
 * case 0x112e5, step FUN_0046e510 case 0x18708).
 *
 * The spec is the decompile (findings/in-game-intro.md "The FOUNTAIN SPRAY" +
 * engine-quirk #87): each primary fountain tick spawns one droplet at the
 * emitter center + jitter, draws exactly 6 LCG values, and launches it UP
 * (vel_y in (-90000,-80000]) with a 3-way horizontal spread (counter % 3:
 * left / right / left-weak).  FUN_0046e510 then applies gravity (+8000/tick,
 * cap 80000) and a fade that expires the droplet after sub_phase reaches 8.
 */
#include "particle.h"
#include "actor_render.h"
#include "anim_clip.h"
#include "draw_pool.h"
#include "rng.h"
#include "t.h"

#include <string.h>

/* Pack (bank, frame) into a non-zero cel handle (mirrors the actor_spawn test). */
static uint32_t resolve_pack(uint16_t bank, uint16_t frame, void *ud)
{
    (void)ud;
    return ((uint32_t)bank << 16) | frame;
}

/* A static pool — 1024 (actor + render-state) pairs is large for the stack. */
static particle_pool g_pp;

/* ---- spawn config: bank 0x1aa frame 6 + the 2-frame water clip ------------- */

int test_particle_spawn_config(void)
{
    particle_pool_reset(&g_pp);
    int slot = particle_spawn_water(&g_pp, 17700, 3900);
    T_ASSERT(slot >= 0);

    const actor *a = &g_pp.actors[slot];
    const actor_render_state *rs = &g_pp.states[slot];
    T_ASSERT_EQ_U(a->sprite_table[0].bank, 0x1aau);
    T_ASSERT_EQ_I(a->sprite_table[0].frame_base, 6);
    T_ASSERT_EQ_U(a->code, 0x18708u);
    T_ASSERT_EQ_U(a->layer, 11u);
    T_ASSERT_EQ_U(rs->active, 1u);
    T_ASSERT_EQ_I(rs->world_x, 17700);
    T_ASSERT_EQ_I(rs->world_y, 3900);
    T_ASSERT(rs->clip != NULL);
    T_ASSERT_EQ_U(rs->clip->frame_count, 2u);   /* the water loop */
    T_ASSERT_EQ_U(rs->clip->frame_dur, 2u);
    T_ASSERT_EQ_I(rs->vel_x, 0);                 /* the emitter sets velocity   */
    T_ASSERT_EQ_I(rs->vel_y, 0);
    return 0;
}

/* ---- the emitter draws exactly 6 LCG values per primary tick --------------- */

int test_particle_fountain_draw_count(void)
{
    int counter = 0;
    /* state after 6 raw draws */
    rng_srand(0x4f5347u);
    for (int i = 0; i < 6; i++) (void)rng_rand();
    uint32_t after6 = rng_peek_state();
    /* state after one emit (pool not full) */
    particle_pool_reset(&g_pp);
    rng_srand(0x4f5347u);
    particle_fountain_emit(&g_pp, 177000, 39000, &counter);
    uint32_t afterEmit = rng_peek_state();
    T_ASSERT_EQ_U(afterEmit, after6);
    return 0;
}

/* ---- the 3-way velocity cycle: all UP, with left / right / left-weak x ----- */

int test_particle_fountain_velocity_cycle(void)
{
    /* counter increments first, so start at 2 -> %3 = 0, then 1, then 2. */
    int expect_x_sign[3] = { -1, +1, -1 };  /* case 0 left, 1 right, 2 left-weak */
    int counter = 2;
    for (int c = 0; c < 3; c++) {
        particle_pool_reset(&g_pp);
        rng_srand(0x1234567u + (uint32_t)c);
        particle_fountain_emit(&g_pp, 177000, 39000, &counter);
        /* exactly one active particle — find it */
        int slot = -1;
        for (int i = 0; i < PARTICLE_POOL_SLOTS; i++)
            if (g_pp.states[i].active) { slot = i; break; }
        T_ASSERT(slot >= 0);
        const actor_render_state *rs = &g_pp.states[slot];
        /* launched UP: vel_y in (-90000, -80000] */
        T_ASSERT(rs->vel_y <= -80000 && rs->vel_y > -90000);
        if (expect_x_sign[c] < 0) T_ASSERT(rs->vel_x < 0);
        else                      T_ASSERT(rs->vel_x > 0);
    }
    return 0;
}

/* ---- gravity + integrate: vel_y += 8000 (cap 80000), x/y move --------------- */

int test_particle_step_gravity(void)
{
    particle_pool_reset(&g_pp);
    int slot = particle_spawn_water(&g_pp, 100000, 200000);
    T_ASSERT(slot >= 0);
    g_pp.states[slot].vel_y = -80000;
    g_pp.states[slot].vel_x = -25000;   /* facing 0 -> moves +250/tick (right)   */

    particle_pool_step(&g_pp);
    const actor_render_state *rs = &g_pp.states[slot];
    T_ASSERT_EQ_I(rs->vel_y, -72000);                  /* -80000 + 8000           */
    T_ASSERT_EQ_I(rs->world_y, 200000 + (-72000)/100); /* y += vel_y/100          */
    T_ASSERT_EQ_I(rs->world_x, 100000 - (-25000)/100); /* x += -(vel_x/100)       */

    /* gravity caps at 80000 after enough ticks (and stays active a while) */
    for (int i = 0; i < 40; i++) particle_pool_step(&g_pp);
    /* if still active, vel_y must be clamped; either way it never exceeds 80000 */
    T_ASSERT(g_pp.states[slot].vel_y <= 80000);
    return 0;
}

/* ---- lifetime: the droplet fades out and frees its slot -------------------- */

int test_particle_step_expires(void)
{
    particle_pool_reset(&g_pp);
    int slot = particle_spawn_water(&g_pp, 100000, 50000);
    T_ASSERT(slot >= 0);
    g_pp.states[slot].vel_y = -85000;

    T_ASSERT_EQ_U(g_pp.states[slot].active, 1u);
    /* sub_phase increments every 3rd tick; expires once it passes 8 (~27 ticks). */
    int active_at_20 = 0;
    for (int i = 0; i < 20; i++) particle_pool_step(&g_pp);
    active_at_20 = g_pp.states[slot].active;
    for (int i = 0; i < 20; i++) particle_pool_step(&g_pp);
    T_ASSERT(active_at_20 == 1);                    /* alive mid-life            */
    T_ASSERT_EQ_U(g_pp.states[slot].active, 0u);    /* freed by ~tick 27         */
    return 0;
}

/* ---- the clip cycles sprites 6,7 (frame_base 6 + delta {0,1}) -------------- */

int test_particle_clip_cycles(void)
{
    particle_pool_reset(&g_pp);
    int slot = particle_spawn_water(&g_pp, 0, 0);
    T_ASSERT(slot >= 0);
    g_pp.states[slot].vel_y = -85000;  /* keep it aloft so it doesn't expire fast */

    /* dur 2 -> frame flips 0->1 after 2 ticks, back to 0 after 2 more. */
    T_ASSERT_EQ_U(g_pp.states[slot].frame, 0u);
    particle_pool_step(&g_pp);
    particle_pool_step(&g_pp);
    T_ASSERT_EQ_U(g_pp.states[slot].frame, 1u);
    return 0;
}

/* ---- round-robin alloc walks distinct slots ------------------------------- */

int test_particle_roundrobin(void)
{
    particle_pool_reset(&g_pp);
    int a = particle_spawn_water(&g_pp, 1, 1);
    int b = particle_spawn_water(&g_pp, 2, 2);
    int c = particle_spawn_water(&g_pp, 3, 3);
    T_ASSERT(a >= 0 && b >= 0 && c >= 0);
    T_ASSERT(a != b && b != c && a != c);
    /* freeing a slot lets the cursor reuse it on the next wrap */
    g_pp.states[b].active = 0;
    return 0;
}

/* ---- render: an active particle emits one node ----------------------------- */

int test_particle_render_emits(void)
{
    particle_pool_reset(&g_pp);
    (void)particle_spawn_water(&g_pp, 12000, 8000);
    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    int n = particle_pool_render(&g_pp, &dp, resolve_pack, NULL);
    T_ASSERT_EQ_I(n, 1);
    draw_pool_free(&dp);
    return 0;
}
