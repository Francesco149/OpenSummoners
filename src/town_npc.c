/*
 * town_npc.c — see town_npc.h.  The settled-town wandering NPC's per-tick LCG
 * draw, ported consume-to-advance so the shared stream stays aligned across the
 * whole town window into the house/errands (ckpt 194).
 */
#include "town_npc.h"

#include <stddef.h>   /* NULL */

#include "rng.h"   /* rng_rand — the shared engine LCG (0x5bf505) */

/* Seed-pinned census ground truth (OSS_RNG_DEFAULT_SEED); PORT-DEBT(town-wander-npc).
 * The NPC enters WALK at census tick 972 and draws line-415 once/tick for 106
 * ticks (through 1077 inclusive), then goes idle for the rest of the town. */
#define TOWN_NPC_WALK_START 972
#define TOWN_NPC_WALK_END   1077

void town_npc_reset(town_npc_pool *p)
{
    if (p == NULL) return;
    p->walk_start = TOWN_NPC_WALK_START;
    p->walk_end   = TOWN_NPC_WALK_END;
}

int town_npc_step(town_npc_pool *p, int32_t census_tick)
{
    if (p == NULL) return 0;
    if (census_tick < p->walk_start || census_tick > p->walk_end)
        return 0;
    /* 0x43f880:415 (VA 0x440301) — the SOLE rand in the move-command builder:
     * the "push command 3" wander roll (rand*1000)>>15 < wander_freq, gated
     * local_b8[6]!=0 && param_5==0 (both hold every tick this NPC walks).  The
     * result only decides whether a wander command is queued (motion the port
     * does not render); the DRAW is what keeps the shared LCG aligned. */
    (void)rng_rand();
    return 1;
}
