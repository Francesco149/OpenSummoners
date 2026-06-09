/*
 * ambient.c — see ambient.h.  The town's four irregular per-sim-tick RNG
 * timers, ported consume-to-advance from their seed-pinned cadences so the
 * shared LCG stays aligned across the whole settled-town window (engine-quirk
 * #95; closes PORT-DEBT(fountain-rng-phase)).
 */
#include "ambient.h"

#include <string.h>

#include "rng.h"   /* rng_rand — the shared engine LCG (0x5bf505) */

/* 0x467380's spawn-set initial countdown for the 0xe2a5 object (its +0x20c).
 * Seed-pinned ground truth (cd=184 -> fires at cd==1 = tick 183); the real
 * source is the unported 0xe2a5 spawn arm (0x431cb0) that writes +0x20c at
 * room load.  Valid under OSS_RNG_DEFAULT_SEED; PORT-DEBT(ambient-event-cd). */
#define EVENT_CD_INIT     184
/* 0x442a70:462 — 0x467380(param,0x49,uVar19=400,100): the reload base
 * (param_3) and modulo (param_4).  Reload = (rand*1000)%100 + 400. */
#define EVENT_RELOAD_BASE 400
#define EVENT_RELOAD_MOD  100

/* (rand * n) >> 15, the exact round-toward-zero form the engine uses; n is
 * small and non-negative here so the product never overflows 32 bits. */
static int32_t rng_scaled(uint32_t n)
{
    return (int32_t)(((uint32_t)rng_rand() * n) >> 15);
}

void ambient_reset(ambient_pool *p)
{
    if (p == NULL) return;
    memset(p, 0, sizeof *p);
    /* The 0x467380 countdown is set during the (unported) 0xe2a5 spawn, not by
     * a per-tick draw, so it is pre-armed with the seed-pinned spawn value. */
    p->event.cd    = EVENT_CD_INIT;
    p->event.armed = 1;
}

/* 0x5531b0 (the ambient sound timer, called from 0x54f980 cases 0x1136f/0x11370).
 * First call: +0x5c = (rand*300)>>15 [init draw].  Then if +0x5c < 1: fire —
 * reload (rand*300)>>15 + base [draw], pick (rand*2)>>15 [draw], and (both town
 * waves are non-zero so always) a sound-param [draw]; else +0x5c-- (no draw).
 * `base` = the caller's param_3 (300 for both town emitters). */
static int sound_timer_step(ambient_timer *t, int base)
{
    int draws = 0;
    if (!t->armed) {                          /* 0x5531b0:33  +0x58 == 0       */
        t->cd    = rng_scaled(300);           /* :34  init draw                */
        t->armed = 1;
        draws++;
    }
    if (t->cd < 1) {                          /* :39  fire                     */
        t->cd = rng_scaled(300) + base;       /* :40  reload                   */
        (void)rng_rand();                     /* :44  pick (rand*2)>>15        */
        (void)rng_rand();                     /* :61  sound-param (wave != 0)  */
        draws += 3;
    } else {
        t->cd--;                              /* :94  decrement                */
    }
    return draws;
}

/* The wagon 0x1872d's idle-wander (0x54f980:932-966, the deferred
 * PORT-DEBT(actor-protagonist-clip) RNG behaviour).  First call: +0x5c =
 * (rand*300)>>15.  Then if +0x5c > 0: decrement (no draw); else (cd <= 0) fire —
 * reload (rand*300)>>15 + 500 [draw], pick (rand*2)>>15 [draw] (always 0/1, so
 * the 0xb7/0xb8 arm is always taken), and its trailing draw [draw]. */
static int wagon_timer_step(ambient_timer *t)
{
    int draws = 0;
    if (!t->armed) {                          /* 0x54f980:932  +0x58 == 0      */
        t->cd    = rng_scaled(300);           /* :933  init draw               */
        t->armed = 1;
        draws++;
    }
    if (t->cd > 0) {                          /* :937  decrement               */
        t->cd--;
    } else {                                  /* fire (cd <= 0)                */
        t->cd = rng_scaled(300) + 500;        /* :943  reload                  */
        (void)rng_rand();                     /* :946  pick (rand*2)>>15       */
        (void)rng_rand();                     /* :961  0xb7/0xb8-arm draw      */
        draws += 3;
    }
    return draws;
}

int ambient_character_step(ambient_pool *p)
{
    if (p == NULL) return 0;
    int draws = 0;
    draws += sound_timer_step(&p->sound_a, 300);   /* 0x1136f (slot 78) */
    draws += sound_timer_step(&p->sound_b, 300);   /* 0x11370 (slot 83) */
    draws += wagon_timer_step(&p->wagon);          /* 0x1872d           */
    return draws;
}

/* The 0x467380 event timer (the 0xe2a5 EFFECT object, via 0x442a70).  No init
 * draw — its +0x20c is pre-set at spawn.  When +0x20c == 1: fire — a draw
 * (0x467380:30), then 0x4099a0 (2 draws), then the reload (rand*1000)%100 +
 * 400 [draw]; else if +0x20c > 1: decrement (no draw). */
int ambient_effect_step(ambient_pool *p)
{
    if (p == NULL) return 0;
    ambient_timer *t = &p->event;
    int draws = 0;
    if (t->cd == 1) {                          /* 0x467380:19  fire             */
        (void)rng_rand();                      /* :30  fire draw (500-(rand*1000>>15)) */
        (void)rng_rand();                      /* :44  0x4099a0 draw 1      */
        (void)rng_rand();                      /* :44  0x4099a0 draw 2      */
        t->cd = (int32_t)(((uint32_t)rng_rand() * 1000u) % EVENT_RELOAD_MOD)
                + EVENT_RELOAD_BASE;           /* :50  reload                   */
        draws += 4;
    } else if (t->cd > 1) {
        t->cd--;                               /* :56  decrement                */
    }
    return draws;
}
