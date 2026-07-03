/*
 * town_npc.{c,h} — the settled-town wandering NPC's per-sim-tick RNG consumer
 * (the town analogue of ambient.c/butterfly.c; ckpt 194 census RE).
 *
 * Ground truth (docs/findings/errands-rng-census.md "Count-vs-timing RESOLVED"):
 * a grounded town pedestrian (retail body 0xe8767d8 @ world 41600,45600) sits
 * idle, then enters WALK state at census sim-tick 972 and walks right for 106
 * ticks (through census 1077).  While walking, its move-command builder
 * 0x43f880 draws the shared LCG ONCE per tick at line 415 (VA 0x440301) — the
 * "push command 3" wander roll (rand*1000)>>15 < wander_freq (in_ECX[0x3212],
 * halved when HP<300 permille).  The port models NONE of the town's grounded
 * NPCs, so it OMITS this +1 draw/tick — the FIRST permanent town RNG divergence
 * (census tick 974: the split survives the 972-973 butterfly self-heal, then holds).
 *
 * This module reproduces JUST that per-tick draw (consume-to-advance, exactly as
 * ambient.c does for the wagon/sounds): the roll RESULT drives wander motion the
 * port does not render, but the DRAW keeps the shared LCG aligned so the town
 * reaches the house/errands with retail's stream — the prerequisite for deriving
 * the errands family anim-phase.  Retail draws it FIRST in the per-tick stream
 * (proven: 0x440301 at index 0 in ALL 106 walking ticks, the census randtrace),
 * so town_npc_step runs before butterfly_step in game_actor_update.
 *
 * PORT-DEBT(town-wander-npc): the walk WINDOW [972,1077] is seed-pinned ground
 * truth (OSS_RNG_DEFAULT_SEED).  The faithful retire path is the NPC's full AI:
 * its spawn (a DATA-1022 map CHARACTER at 41600,45600) + the idle->walk trigger +
 * the 0x43f880 command builder + the 0x442a70 walk mover (the town analogue of
 * the errands quirk #86 spawn burst).  Also un-modelled: the NPC's RENDER
 * (if on-screen), and the SECONDARY 0x489280 ±2 consumer (census
 * 979/999/1019... ~every 20t, which continues past the walk) — the next chip.
 */
#ifndef OSS_TOWN_NPC_H
#define OSS_TOWN_NPC_H

#include <stdint.h>

typedef struct town_npc_pool {
    int32_t walk_start;   /* first census sim-tick the NPC draws line-415 (WALK)  */
    int32_t walk_end;     /* last census sim-tick it draws (inclusive)            */
} town_npc_pool;

/* Arm the pool at town entry (sets the seed-pinned walk window). */
void town_npc_reset(town_npc_pool *p);

/* Advance one sim-tick.  `census_tick` is the OSR_STATE tick this game_actor_update
 * feeds (= g_sim_tick_count + 1, since the actor update runs BEFORE the camera
 * easer increments the counter).  Draws the line-415 roll (1 LCG draw) while the
 * NPC is walking; returns the number of draws consumed (0 or 1).  Call FIRST in
 * game_actor_update's RNG section, before butterfly_step. */
int  town_npc_step(town_npc_pool *p, int32_t census_tick);

#endif /* OSS_TOWN_NPC_H */
