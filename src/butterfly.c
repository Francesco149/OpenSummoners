/*
 * butterfly.c — see butterfly.h.  The town butterflies' per-sim-tick LCG draws
 * (0x47b990's 0xe29a case), ported consume-to-advance to keep the shared stream
 * aligned for the downstream particle emitters.
 */
#include "butterfly.h"

#include <string.h>

#include "rng.h"   /* rng_rand — the shared engine LCG (0x5bf505) */

void butterfly_pool_reset(butterfly_pool *p)
{
    if (p != NULL) memset(p, 0, sizeof *p);
}

int butterfly_register(butterfly_pool *p, uint16_t wander_freq)
{
    if (p == NULL || p->count >= BUTTERFLY_MAX) return -1;
    butterfly *b   = &p->b[p->count];
    b->wander_freq = wander_freq;
    b->gate        = 0;   /* 0x14232 — fresh => works on the first (even) tick   */
    b->wander_timer = 0;  /* 0x14236 — 0 => the flit pick fires on the spawn tick */
    return p->count++;
}

/* (rand * n) >> 15 with round-toward-zero — the exact form 0x47b990 uses; n is
 * non-negative and <= 1000 here, so the product never overflows 32 bits. */
static int rng_scaled(uint32_t n)
{
    return (int)(((uint32_t)rng_rand() * n) >> 15);
}

int butterfly_step(butterfly_pool *p)
{
    if (p == NULL) return 0;
    int draws = 0;
    for (int i = 0; i < p->count; i++) {
        butterfly *b = &p->b[i];

        /* 0x47b990:515-519 — the every-other-tick gate. */
        if (b->gate != 0) { b->gate--; continue; }   /* skip this tick */
        b->gate = 1;

        /* 0x47b990:520-542 — the flit-pick timer (NPC arm, +0x1f0 == 0). */
        if (b->wander_timer == 0) {
            int r = rng_scaled(1000);                 /* :523 the move test     */
            draws++;
            if (r < b->wander_freq) {                 /* :524 passed -> set flit */
                (void)rng_rand();                     /* :534 the flit offset    */
                draws++;
            }
            b->wander_timer = BUTTERFLY_WANDER_PERIOD; /* :540 reload 0x50       */
        } else {
            b->wander_timer--;                        /* :543 countdown          */
        }

        /* 0x47b990:768-801 — the 0xe29a case: heading + flutter flag, always. */
        (void)rng_rand();   /* :770 — 0xc890 = (rand*0xc80>>15)+0x640 (heading) */
        (void)rng_rand();   /* :776 — bVar11 = (rand*1000>>15) < 100 (flag)     */
        draws += 2;
    }
    return draws;
}
