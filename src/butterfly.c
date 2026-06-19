/*
 * butterfly.c — see butterfly.h.  The town butterflies' per-sim-tick AI (the
 * 0x47b990 0xe29a case) + the open-air horizontal patrol motion (the chip-1
 * reduction of the 0x485fc0 -> 0x442a70 apply pass).  The RNG draw count/order
 * is UNCHANGED from ckpt 98/99 — the heading FSM moves the existing two draws
 * into their real use; it does not add or reorder draws (the ckpt-99 settled-town
 * stream stays bit-exact).
 */
#include "butterfly.h"

#include <string.h>

#include "rng.h"   /* rng_rand — the shared engine LCG (0x5bf505) */
#include "butterfly_flap_ctrl.h"  /* the captured flap TRIGGER (PORT-DEBT) */

void butterfly_pool_reset(butterfly_pool *p)
{
    if (p != NULL) memset(p, 0, sizeof *p);
}

int butterfly_register(butterfly_pool *p, uint16_t wander_freq,
                       int32_t spawn_world_x, int32_t spawn_world_y,
                       int effect_slot)
{
    if (p == NULL || p->count >= BUTTERFLY_MAX) return -1;
    butterfly *b   = &p->b[p->count];
    b->wander_freq = wander_freq;
    b->gate        = 0;   /* 0x14232 — fresh => works on the first (even) tick   */
    b->wander_timer = 0;  /* 0x14236 — 0 => the flit pick fires on the spawn tick */
    /* Motion state (chip-1 open-air reduction). */
    b->world_x  = spawn_world_x;
    b->bound1   = spawn_world_x + BUTTERFLY_BOUND1_OFF;   /* 0x14264 right target */
    b->bound3   = spawn_world_x + BUTTERFLY_BOUND3_OFF;   /* 0x14268 left  target */
    b->heading  = 0;      /* 0x14244 — uninit => the "left" (else) FSM arm        */
    b->hvel     = 0;
    b->facing   = 1;      /* body+0x2c — spawn default (TOWN_EFFECT_DEFS facing 1)*/
    b->cooldown = 0;      /* 0x14248                                              */
    b->cmd_dir  = -1;     /* heading 0 -> target the LEFT bound first             */
    b->effect_slot = effect_slot;
    /* Vertical flutter: integrate from the spawn worldY; match this butterfly to
     * its captured flap-control lane by spawn worldX (PORT-DEBT trigger). */
    b->world_y    = spawn_world_y;
    b->vvel       = 0;
    b->flap_sub   = 0;
    b->flap_cnt   = 0;
    b->prev_state3 = 0;
    b->life_tick  = 0;
    b->ctrl_lane  = -1;
    for (int L = 0; L < 4; L++) {
        if (BUTTERFLY_FLAP_CTRL_WX[L] == spawn_world_x) { b->ctrl_lane = L; break; }
    }
    return p->count++;
}

/* (rand * n) >> 15 with round-toward-zero — the exact form 0x47b990 uses; n is
 * non-negative and <= 1000 here, so the product never overflows 32 bits. */
static int rng_scaled(uint32_t n)
{
    return (int)(((uint32_t)rng_rand() * n) >> 15);
}

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

/* Ramp cur toward target by at most |rate| (the open-air velocity accel). */
static int32_t ramp_toward(int32_t cur, int32_t target, int32_t rate)
{
    if (cur < target) { cur += rate; if (cur > target) cur = target; }
    else if (cur > target) { cur -= rate; if (cur < target) cur = target; }
    return cur;
}

int butterfly_step(butterfly_pool *p)
{
    if (p == NULL) return 0;
    int draws = 0;
    for (int i = 0; i < p->count; i++) {
        butterfly *b = &p->b[i];

        /* 0x47b990:405 — the flip cooldown decrements EVERY tick (before the gate). */
        if (b->cooldown != 0) b->cooldown--;

        /* 0x47b990:515-519 — the every-other-tick gate.  On a skip tick the AI is
         * not run (no draws, no heading change) — but the APPLY below still runs. */
        if (b->gate != 0) {
            b->gate--;
        } else {
            b->gate = 1;

            /* 0x47b990:520-543 — the flit-pick timer (NPC arm, +0x1f0 == 0). */
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

            /* 0x47b990:769-801 — the 0xe29a case. */
            (void)rng_rand();   /* :770 — 0xc890 wander range = (rand*0xc80>>15)+0x640 */
            draws++;
            if (b->cooldown != 0) b->cooldown--;          /* :773 work-tick cooldown dec */
            int flag = rng_scaled(1000) < 100;            /* :776 — the 10% flutter flag */
            draws++;

            /* :778-798 — flip the heading toward the FAR bound when the cooldown
             * has expired and (we are within ~one tile of the CURRENT target bound
             * OR the 10% roll fires).  iVar2 (target_bound) is read for the
             * CURRENT heading BEFORE the flip, so the move command (below) still
             * aims at the old bound on the flip tick (= retail's one-work-tick lag).
             * The 0x47dbb0 collision term is omitted: open air returns clear
             * (chip 2 adds it for grounded actors). */
            int32_t target_bound;
            if (b->heading == 1) {
                target_bound = b->bound1;
                if (b->cooldown == 0 &&
                    (iabs32(b->world_x - b->bound1) < BUTTERFLY_BOUND_NEAR || flag)) {
                    b->heading  = 3;
                    b->cooldown = BUTTERFLY_FLIP_COOLDOWN;
                }
            } else {
                target_bound = b->bound3;
                if (b->cooldown == 0 &&
                    (iabs32(b->world_x - b->bound3) < BUTTERFLY_BOUND_NEAR || flag)) {
                    b->heading  = 1;
                    b->cooldown = BUTTERFLY_FLIP_COOLDOWN;
                }
            }
            /* 0x43f880 writes the move command toward target_bound; in open air it
             * reduces to "drift toward the bound" -> the integrate direction. */
            b->cmd_dir = (target_bound > b->world_x) ? 1 : -1;
        }

        /* 0x485fc0 -> 0x442a70 — the APPLY pass, EVERY tick (both gate phases).
         * Open-air reduction: step worldX by the CURRENT velocity, THEN ramp the
         * velocity toward the commanded direction (+-10/tick, cap +-100).  The
         * step-before-ramp order is the capture's: tick 0 has hvel 0 so dwx is 0
         * even though the AI already commanded a direction; the glide builds from
         * tick 1.  facing follows the velocity sign (it lags the heading through
         * the decel/reverse window). */
        int32_t target_v = b->cmd_dir * BUTTERFLY_HSPEED_CAP;
        b->world_x += b->hvel;
        b->hvel = ramp_toward(b->hvel, target_v, BUTTERFLY_HSPEED_RAMP);
        if (b->hvel > 0)      b->facing = 1;
        else if (b->hvel < 0) b->facing = 3;

        /* ── VERTICAL FLUTTER (0x442a70 case-3 airborne sub-FSM, RE'd off the
         * install 0x427d30/0x427c30 — the SAME FSM as character.c's jump).  The
         * PHYSICS is real logic; the per-tick flap TRIGGER (the body+0x38==3 flap
         * state + the cmd_2==8 "held" control, which retail derives from the
         * terrain-aware mover scanning the floor below) is the captured
         * PORT-DEBT(butterfly-flutter-trigger) control stream, replayed by tick.
         *
         * Order matches the decompile: the windup may snap vvel to the impulse,
         * THEN worldY integrates the (current) vvel, THEN vvel ramps by the accel
         * (capped at the fall terminal).  So the impulse tick moves worldY by the
         * impulse/100 and the same accel step yields -30000 next.  Verified
         * bit-exact (0 vvel mismatches / 1824 ticks) vs runs/butterfly-flutter. */
        b->life_tick++;
        int s3 = 0, held = 0;
        if (b->ctrl_lane >= 0 && b->life_tick < BUTTERFLY_FLAP_CTRL_NTICKS) {
            uint8_t c = BUTTERFLY_FLAP_CTRL[b->life_tick];
            s3   = (c >> (2 * b->ctrl_lane))     & 1;
            held = (c >> (2 * b->ctrl_lane + 1)) & 1;
        }
        if (s3 && !b->prev_state3) { b->flap_sub = 0; b->flap_cnt = 0; } /* flap start */
        int32_t grav;
        if (s3 && b->flap_sub == 0) {            /* windup: glide-hold until impulse */
            b->flap_cnt++;
            if (b->flap_cnt > BUTTERFLY_FLAP_WINDUP) {
                b->vvel     = BUTTERFLY_FLAP_IMPULSE;
                b->flap_sub = 1;
                b->flap_cnt = 0;
            }
            grav = BUTTERFLY_FALL_GRAV;          /* pre-switch default (windup skips select) */
        } else if (s3) {                          /* airborne (sub>=1) */
            grav = (b->vvel < 0)
                     ? (held ? BUTTERFLY_RISE_GRAV_HELD : BUTTERFLY_RISE_GRAV_FREE)
                     : BUTTERFLY_FALL_GRAV;
        } else {                                  /* gliding */
            b->flap_sub = 0; b->flap_cnt = 0;
            grav = BUTTERFLY_FALL_GRAV;
        }
        b->world_y += b->vvel / 100;             /* C trunc toward 0 = the 0x54e5c0 step */
        b->vvel += grav;
        if (b->vvel > BUTTERFLY_FALL_CAP) b->vvel = BUTTERFLY_FALL_CAP;
        b->prev_state3 = (int16_t)s3;
    }
    return draws;
}
