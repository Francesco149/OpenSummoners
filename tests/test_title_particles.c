/*
 * tests/test_title_particles.c — host tests for the phase-7 sparkle-particle
 * pool: the LCG (rng.c) and the spawn (FUN_0056c070, title_particles.c).
 *
 * The LCG is anchored against the known MSVC rand() stream (srand(1) → 41,
 * …) so the port is pinned to the real CRT generator, not just internally
 * consistent.  The spawn is checked field-by-field against an independently
 * spelled-out reference of FUN_0056c070's arithmetic, plus the count/cap
 * guard and the rand-call *order* (x, y, spare, phase).
 */
#include "t.h"
#include "rng.h"
#include "title_particles.h"

#include <string.h>

/* ─── the LCG ──────────────────────────────────────────────────────── */

int test_rng_matches_msvc_stream(void)
{
    /* The canonical MSVC sequence for srand(1): 41, 18467, 6334, 26500, … */
    static const uint32_t want[] = { 41u, 18467u, 6334u, 26500u, 19169u };
    rng_srand(1);
    for (size_t i = 0; i < sizeof want / sizeof want[0]; i++)
        T_ASSERT_EQ_U(rng_rand(), want[i]);
    return 0;
}

int test_rng_srand_is_reproducible(void)
{
    rng_srand(0x4f5347u);
    uint32_t a0 = rng_rand(), a1 = rng_rand(), a2 = rng_rand();
    rng_srand(0x4f5347u);
    T_ASSERT_EQ_U(rng_rand(), a0);
    T_ASSERT_EQ_U(rng_rand(), a1);
    T_ASSERT_EQ_U(rng_rand(), a2);
    return 0;
}

/* ─── reference for FUN_0056c070's per-field arithmetic ──────────────── */

static int32_t ref_scale(int32_t r, int32_t mul)
{
    int32_t prod = r * mul;
    return (prod + ((prod >> 31) & 0x7fff)) >> 15;
}

/* ─── the spawn ────────────────────────────────────────────────────── */

int test_particle_pool_init(void)
{
    title_particle_pool pool;
    memset(&pool, 0xaa, sizeof pool);
    title_particle_pool_init(&pool);
    T_ASSERT_EQ_P(pool.group.entries, pool.store);
    T_ASSERT_EQ_U(pool.group.cap, TITLE_PARTICLE_CAP);
    T_ASSERT_EQ_U(pool.group.count, 0);
    return 0;
}

int test_particle_spawn_title_fields_and_order(void)
{
    title_particle_pool pool;
    title_particle_pool_init(&pool);

    const int32_t intensity = (51 * 0xe0) / 900 + 0xc0;   /* a phase-7 value */

    /* Replay the LCG independently to know the four draws, in order. */
    uint32_t seed = 0x4f5347u;
    rng_srand(seed);
    uint32_t ref = seed;
    uint32_t rr[4];
    for (int i = 0; i < 4; i++) {
        ref = ref * 0x343fdu + 0x269ec3u;
        rr[i] = (ref >> 16) & 0x7fffu;
    }

    rng_srand(seed);   /* re-pin so the spawn consumes the same stream */
    title_particle_spawn_title(&pool, intensity);

    T_ASSERT_EQ_U(pool.group.count, 1);
    const title_sprite_entry *e = &pool.store[0];

    /* x/y consume rr[0], rr[1] with jitters 0x10 / 0x18, biases intensity /
     * 0x1a0, centi-pixel scale; spare rr[2]*200/32768; phase rr[3]*20/32768
     * + 0x14, copied into anim_num. */
    T_ASSERT_EQ_I(e->x_num, (ref_scale((int32_t)rr[0], 0x10) + intensity) * 100);
    T_ASSERT_EQ_I(e->y_num, (ref_scale((int32_t)rr[1], 0x18) + 0x1a0) * 100);
    T_ASSERT_EQ_U(e->_pad08, (uint32_t)ref_scale((int32_t)rr[2], 200));

    uint16_t phase = (uint16_t)(ref_scale((int32_t)rr[3], 0x14) + 0x14);
    T_ASSERT_EQ_U(e->anim_div, phase);
    T_ASSERT_EQ_U(e->anim_num, phase);

    /* The constant fields. */
    T_ASSERT_EQ_U(e->bank_id, 0x15);
    T_ASSERT_EQ_U(e->frame_base, 0);
    T_ASSERT_EQ_U(e->frame_count, 8);
    T_ASSERT_EQ_I(e->alpha_level, 800);

    /* Seed advanced by exactly four draws. */
    T_ASSERT_EQ_U(rng_peek_state(), ref);
    return 0;
}

int test_particle_spawn_appends_sequentially(void)
{
    title_particle_pool pool;
    title_particle_pool_init(&pool);
    rng_srand(7);
    title_particle_spawn_title(&pool, 0xc0);
    title_particle_spawn_title(&pool, 0xc0);
    title_particle_spawn_title(&pool, 0xc0);
    T_ASSERT_EQ_U(pool.group.count, 3);
    /* Distinct random y for at least two of them (sanity: stream advances). */
    T_ASSERT(pool.store[0].y_num != pool.store[1].y_num ||
             pool.store[1].y_num != pool.store[2].y_num);
    return 0;
}

int test_particle_spawn_caps_at_capacity(void)
{
    title_particle_pool pool;
    title_particle_pool_init(&pool);
    rng_srand(123);
    for (int i = 0; i < TITLE_PARTICLE_CAP + 50; i++)
        title_particle_spawn_title(&pool, 0xc0);
    T_ASSERT_EQ_U(pool.group.count, TITLE_PARTICLE_CAP);

    /* The cap-th spawn was dropped and consumed no RNG: re-pin, fill, then
     * one extra spawn must leave the seed untouched. */
    title_particle_pool_init(&pool);
    rng_srand(456);
    for (int i = 0; i < TITLE_PARTICLE_CAP; i++)
        title_particle_spawn_title(&pool, 0xc0);
    uint32_t after_full = rng_peek_state();
    title_particle_spawn_title(&pool, 0xc0);   /* full → no-op */
    T_ASSERT_EQ_U(rng_peek_state(), after_full);
    T_ASSERT_EQ_U(pool.group.count, TITLE_PARTICLE_CAP);
    return 0;
}

int test_particle_spawn_x_sweeps_with_intensity(void)
{
    /* p5 (intensity) biases x; a higher intensity must push x right by
     * exactly the intensity delta * 100 (same seed → same jitter). */
    title_particle_pool a, b;
    title_particle_pool_init(&a);
    title_particle_pool_init(&b);
    rng_srand(99);
    title_particle_spawn_title(&a, 0xc0);
    rng_srand(99);
    title_particle_spawn_title(&b, 0xc0 + 10);
    T_ASSERT_EQ_I(b.store[0].x_num - a.store[0].x_num, 10 * 100);
    return 0;
}
