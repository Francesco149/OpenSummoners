/*
 * src/rng.c — MSVC rand/srand LCG (see rng.h).
 *
 * Bit-exact reimplementation of the CRT generator sotes.exe links:
 *   rand  = FUN_005bf505 @ 0x5bf505
 *   srand = FUN_005bf4fb @ 0x5bf4fb
 *   seed  = DAT_008a4f94
 */

#include "rng.h"

/* DAT_008a4f94.  MSVC's documented default before any srand() is 1; we keep
 * that so an un-seeded run still matches the CRT, but the port pins a fixed
 * seed at boot (see rng.h / main.c). */
static uint32_t g_rng_seed = 1u;

/* Cumulative LCG draw count (the `rngcalls` consumption signal — the trace
 * studio's RNG state panel / census).  Counts every rng_rand() since boot,
 * independent of srand (mirrors retail's per-Flip rngcalls field). */
static uint64_t g_rng_calls = 0u;

void rng_srand(uint32_t seed)
{
    g_rng_seed = seed;                       /* 0x5bf4fb: mov [8a4f94], arg */
}

uint32_t rng_rand(void)
{
    /* 0x5bf505: seed = seed*0x343fd + 0x269ec3; return (seed>>16) & 0x7fff. */
    g_rng_calls++;
    g_rng_seed = g_rng_seed * 0x343fdu + 0x269ec3u;
    return (g_rng_seed >> 16) & 0x7fffu;
}

uint32_t rng_peek_state(void)
{
    return g_rng_seed;
}

uint64_t rng_call_count(void)
{
    return g_rng_calls;
}
