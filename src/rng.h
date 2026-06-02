/*
 * src/rng.h — the engine's pseudo-random generator (MSVC `rand`/`srand`).
 *
 * sotes.exe links the statically-compiled MSVC CRT `rand`, a 32-bit linear
 * congruential generator over a single global seed word:
 *
 *     FUN_005bf505 (rand):   seed = seed*0x343fd + 0x269ec3;
 *                            return (seed >> 16) & 0x7fff;     // 0..32767
 *     FUN_005bf4fb (srand):  seed = arg;
 *
 * The seed global is DAT_008a4f94.  Retail seeds it once at startup
 * (FUN_00562210 @ 0x56227a: `srand(time(NULL))` — FUN_005bf6df is time()),
 * so retail's random stream is **wall-clock-dependent and not reproducible**
 * across runs.  Every random effect that funnels through this generator —
 * the phase-7 title sparkle spawn (FUN_0056c070) being the first the port
 * touches — therefore differs run-to-run on retail.
 *
 * For a bit-exact port the seed is **pinned** on both sides (user directive):
 * the port seeds a fixed constant by default (rng_srand at boot, env-
 * overridable via OPENSUMMONERS_RNG_SEED) and the parity harness writes the
 * same constant into DAT_008a4f94 on retail at the matching point, so the two
 * LCG streams march in lockstep.  Any residual divergence under a fixed seed
 * is a real ordering bug to chase (an unaccounted rand() consumer firing
 * between the two sides), not RNG noise — see findings/engine-quirks.md.
 *
 * The generator is a leaf with no Win32 surface, so it builds and runs under
 * the host unit suite unchanged.
 */
#ifndef OPENSUMMONERS_RNG_H
#define OPENSUMMONERS_RNG_H

#include <stdint.h>

/* The default pinned seed.  Arbitrary fixed value; its only contract is that
 * the port and the retail-side parity pin agree on it.  Chosen non-zero and
 * distinctive so it is recognisable in a memory dump. */
#define OSS_RNG_DEFAULT_SEED 0x4f5347u   /* 'OSG' */

/* FUN_005bf4fb — seed the generator (MSVC srand). */
void rng_srand(uint32_t seed);

/* FUN_005bf505 — next value in [0, 0x7fff] (MSVC rand).  Advances the seed. */
uint32_t rng_rand(void);

/* Direct read of the live seed word (DAT_008a4f94) — for tests / probes. */
uint32_t rng_peek_state(void);

#endif /* OPENSUMMONERS_RNG_H */
