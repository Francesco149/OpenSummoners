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
    T_ASSERT_EQ_I(rs->facing, 1);                /* trace-confirmed +0x2c == 1  */
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
    g_pp.states[slot].vel_x = -25000;   /* facing 1 (spawn) -> x += vel_x/100 left */

    particle_pool_step(&g_pp);
    const actor_render_state *rs = &g_pp.states[slot];
    T_ASSERT_EQ_I(rs->vel_y, -72000);                  /* -80000 + 8000           */
    T_ASSERT_EQ_I(rs->world_y, 200000 + (-72000)/100); /* y += vel_y/100          */
    T_ASSERT_EQ_I(rs->world_x, 100000 + (-25000)/100); /* x += vel_x/100 (facing1)*/

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
    int slot = particle_spawn_water(&g_pp, 12000, 8000);
    T_ASSERT(slot >= 0);
    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    int n = particle_pool_render(&g_pp, &dp, resolve_pack, NULL);
    T_ASSERT_EQ_I(n, 1);
    /* FRESH (sub_phase 0): the config alpha is ramp_b[10] (mode 0, NORMAL blend
     * = DAT_008a9330), NOT ramp_a[10] (mode 1 ADD).  Rendering it via ramp_a's
     * add-blend blew the spawn-cluster droplets out white (R7 fade fix, ckpt 107;
     * 0x557550 case 0x18708 / 0x4385c0). */
    const draw_node *fresh = &dp.layers[11 /*PARTICLE_LAYER_WATER*/].nodes[0];
    T_ASSERT((fresh->param8 & PARTICLE_PARAM8_RAMP_B) != 0u);
    T_ASSERT_EQ_U(fresh->param8 & PARTICLE_PARAM8_IDX_MASK, 10u);
    draw_pool_free(&dp);

    /* AGED (sub_phase 2): ramp_a (no RAMP_B bit), idx = 10 - sub_phase = 8. */
    g_pp.states[slot].sub_phase = 2;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    T_ASSERT_EQ_I(particle_pool_render(&g_pp, &dp, resolve_pack, NULL), 1);
    const draw_node *aged = &dp.layers[11].nodes[0];
    T_ASSERT((aged->param8 & PARTICLE_PARAM8_RAMP_B) == 0u);
    T_ASSERT_EQ_U(aged->param8 & PARTICLE_PARAM8_IDX_MASK, 8u);
    draw_pool_free(&dp);
    return 0;
}

/* ===== the SKY-AMBIENT particle (code 0x18704) ============================== */

/* ---- spawn config: bank 0x1aa frame 8, layer 6, 6-frame oneshot clip ------- */

int test_particle_sky_spawn_config(void)
{
    particle_pool_reset(&g_pp);
    rng_srand(0x4f5347u);
    int slot = particle_spawn_sky(&g_pp, 50000, 10000);
    T_ASSERT(slot >= 0);

    const actor *a = &g_pp.actors[slot];
    const actor_render_state *rs = &g_pp.states[slot];
    T_ASSERT_EQ_U(a->sprite_table[0].bank, 0x1aau);
    T_ASSERT_EQ_I(a->sprite_table[0].frame_base, 8);
    T_ASSERT_EQ_U(a->code, 0x18704u);
    T_ASSERT_EQ_U(a->layer, 6u);
    T_ASSERT_EQ_U(rs->active, 1u);
    T_ASSERT_EQ_I(rs->world_x, 50000);
    T_ASSERT_EQ_I(rs->world_y, 10000);
    T_ASSERT(rs->clip != NULL);
    T_ASSERT_EQ_U(rs->clip->frame_count, 6u);    /* 0x644b58: 6 frames, dur 20  */
    T_ASSERT_EQ_U(rs->clip->frame_dur, 20u);
    T_ASSERT(rs->clip->oneshot != 0);            /* ONESHOT — expires when done */
    /* 0x453960(-10000,5000,-1000,1000): vel_y in [-1000,0), vel_x in [-10000,-5000). */
    T_ASSERT(rs->vel_y >= -1000 && rs->vel_y < 0);
    T_ASSERT(rs->vel_x >= -10000 && rs->vel_x < -5000);
    T_ASSERT_EQ_I(rs->facing, 1);                /* trace-confirmed +0x2c == 1  */
    return 0;
}

/* ---- the emitter spawns once every 6th tick, drawing 4 LCG on that tick ----- */

int test_particle_sky_emit_cadence(void)
{
    particle_pool_reset(&g_pp);
    int counter = 0;
    /* ticks 1..5: no spawn */
    for (int i = 0; i < 5; i++) {
        particle_sky_emit(&g_pp, 60000, 12000, &counter);
        int active = 0;
        for (int j = 0; j < PARTICLE_POOL_SLOTS; j++)
            if (g_pp.states[j].active) active++;
        T_ASSERT_EQ_I(active, 0);
    }
    /* tick 6: exactly one spawn, counter resets */
    particle_sky_emit(&g_pp, 60000, 12000, &counter);
    int active = 0;
    for (int j = 0; j < PARTICLE_POOL_SLOTS; j++)
        if (g_pp.states[j].active) active++;
    T_ASSERT_EQ_I(active, 1);
    T_ASSERT_EQ_I(counter, 0);

    /* the spawn tick draws exactly 4 LCG (2 jitter + the config's 2 scatter). */
    rng_srand(0x4f5347u);
    for (int i = 0; i < 4; i++) (void)rng_rand();
    uint32_t after4 = rng_peek_state();
    particle_pool_reset(&g_pp);
    rng_srand(0x4f5347u);
    int counter2 = 5;                 /* the next emit is the 6th -> spawns */
    particle_sky_emit(&g_pp, 60000, 12000, &counter2);
    T_ASSERT_EQ_U(rng_peek_state(), after4);
    return 0;
}

/* ---- step: vel_y decelerates toward -5000, x/y integrate ------------------- */

int test_particle_sky_step_physics(void)
{
    particle_pool_reset(&g_pp);
    int slot = particle_spawn_sky(&g_pp, 100000, 200000);
    T_ASSERT(slot >= 0);
    actor_render_state *rs = &g_pp.states[slot];
    rs->vel_y = -1000;
    rs->vel_x = -8000;   /* facing 1 (spawn) -> x += vel_x/100 (drifts LEFT)     */

    particle_pool_step(&g_pp);
    T_ASSERT_EQ_I(rs->vel_y, -1500);                   /* -1000 - 500            */
    T_ASSERT_EQ_I(rs->world_y, 200000 + (-1500)/100);  /* y += vel_y/100         */
    T_ASSERT_EQ_I(rs->world_x, 100000 + (-8000)/100);  /* x += vel_x/100 (facing1)*/
    T_ASSERT_EQ_I(rs->life, 1);

    /* vel_y saturates at the -5000 floor and never undershoots it. */
    for (int i = 0; i < 40; i++) particle_pool_step(&g_pp);
    T_ASSERT_EQ_I(rs->vel_y, -5000);
    return 0;
}

/* ---- lifetime: the oneshot clip finishes and frees the slot ---------------- */

int test_particle_sky_step_expires(void)
{
    particle_pool_reset(&g_pp);
    int slot = particle_spawn_sky(&g_pp, 0, 0);
    T_ASSERT(slot >= 0);
    T_ASSERT_EQ_U(g_pp.states[slot].active, 1u);

    /* 6 frames * dur 20 = 120 ticks to finish the oneshot, then `done` -> the
     * next step expires it.  Alive at 100, gone by ~121. */
    for (int i = 0; i < 100; i++) particle_pool_step(&g_pp);
    int active_at_100 = g_pp.states[slot].active;
    for (int i = 0; i < 40; i++) particle_pool_step(&g_pp);
    T_ASSERT(active_at_100 == 1);
    T_ASSERT_EQ_U(g_pp.states[slot].active, 0u);
    return 0;
}

/* ---- render: the sky node carries the ramp_b selector + the fade index ------ */

int test_particle_sky_render_ramp_b(void)
{
    particle_pool_reset(&g_pp);
    int slot = particle_spawn_sky(&g_pp, 12000, 8000);
    T_ASSERT(slot >= 0);
    g_pp.states[slot].life = 0;          /* steady-state -> fade idx 18         */

    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    int n = particle_pool_render(&g_pp, &dp, resolve_pack, NULL);
    T_ASSERT_EQ_I(n, 1);

    /* the node lands in layer 6 (the sky), mode 1 (alpha), ramp_b idx 18. */
    const draw_layer *L = &dp.layers[6];
    T_ASSERT_EQ_U(L->count, 1u);
    const draw_node *nd = &L->nodes[0];
    T_ASSERT_EQ_U(nd->mode, 1u);
    T_ASSERT((nd->param8 & PARTICLE_PARAM8_RAMP_B) != 0u);
    T_ASSERT_EQ_U(nd->param8 & PARTICLE_PARAM8_IDX_MASK, 18u);

    /* aged past lifetime 40 -> the fade index drops below 18. */
    g_pp.states[slot].life = 60;         /* idx = 18 - (60-40)/4 = 13           */
    draw_pool_reset(&dp);
    T_ASSERT_EQ_I(particle_pool_render(&g_pp, &dp, resolve_pack, NULL), 1);
    T_ASSERT_EQ_U(dp.layers[6].nodes[0].param8 & PARTICLE_PARAM8_IDX_MASK, 13u);

    draw_pool_free(&dp);
    return 0;
}
