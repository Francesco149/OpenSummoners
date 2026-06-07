/*
 * particle.c — the fountain WATER particle (code 0x18708), the first chip of the
 * engine's particle subsystem (the +0x13e0 DEVICE band).  See particle.h +
 * engine-quirk #87 + findings/in-game-intro.md "The FOUNTAIN SPRAY".
 *
 * Provenance: alloc 0x557370, config 0x557550 case 0x18708, emitter
 * 0x54f980 case 0x112e5 (:218-283), step 0x46e510 case 0x18708 (:529),
 * scatter helper 0x453960, render 0x493480 default arm.
 */
#include "particle.h"

#include <string.h>

#include "actor_render.h"
#include "rng.h"            /* rng_rand — the shared LCG (0x5bf505)         */

/* code 0x18708 — the fountain water droplet. */
#define PARTICLE_CODE_WATER 0x18708u
/* bank 0x1aa = res 0x408 (the particle sheet); the water droplet starts at
 * frame 6 (0x557550 case 0x18708: 0x426d70(0,0x1aa,6)). */
#define PARTICLE_BANK_WATER 0x1aau
#define PARTICLE_FRAME_WATER 6
/* 0x493480's +0x13e0 cluster renders at layer 11 (the square). */
#define PARTICLE_LAYER_WATER 11

/*
 * The water droplet's clip 0x6449c0 (decoded from the exe, decoder validated vs
 * IDLE_CLIP): base 0, 2 frames, dur 2, looping, delta {0,1}.  With frame_base 6
 * the droplet alternates sprites 6,7 every 2 sim-ticks.
 */
static const anim_clip WATER_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1 },
    .frame_count = 2,
    .frame_dur   = 2,
    .oneshot     = 0,    /* loops */
};

/*
 * MSVC's `(rand() * N) >> 15` with round-toward-zero — the exact form retail
 * uses everywhere ((iVar * N + (iVar * N >> 0x1f & 0x7fff)) >> 0xf).  rand() is
 * 0..32767, |N| <= 10000 here, so the 32-bit product never overflows.
 */
static int32_t rng_scaled(int32_t n)
{
    int32_t v = (int32_t)(uint32_t)rng_rand() * n;
    return (v + ((v >> 31) & 0x7fff)) >> 15;
}

void particle_pool_reset(particle_pool *pool)
{
    if (pool == NULL) return;
    memset(pool, 0, sizeof *pool);
}

int particle_spawn_water(particle_pool *pool, int32_t world_x, int32_t world_y)
{
    if (pool == NULL) return -1;

    /* 0x557370: round-robin from the cursor to the first free slot
     * (render-state inactive == retail's actor+0x1d0 == 0). */
    int slot = -1;
    for (int i = 0; i < PARTICLE_POOL_SLOTS; i++) {
        uint16_t idx = pool->cursor;
        pool->cursor = (uint16_t)((pool->cursor + 1) & (PARTICLE_POOL_SLOTS - 1));
        if (!pool->states[idx].active) { slot = idx; break; }
    }
    if (slot < 0) return -1;   /* full — retail evicts oldest; PORT-DEBT(particle-evict) */

    actor *a = &pool->actors[slot];
    actor_render_state *rs = &pool->states[slot];
    memset(a, 0, sizeof *a);
    memset(rs, 0, sizeof *rs);

    /* 0x557550 case 0x18708 — bank 0x1aa frame_base 6 + the water clip. */
    a->sprite_table[0].bank       = PARTICLE_BANK_WATER;
    a->sprite_table[0].frame_base = PARTICLE_FRAME_WATER;
    a->dir   = 0;
    a->code  = PARTICLE_CODE_WATER;
    a->layer = PARTICLE_LAYER_WATER;

    rs->active  = 1;
    rs->world_x = world_x;
    rs->world_y = world_y;
    rs->clip    = &WATER_CLIP;     /* +0x6c — animates frames 6,7              */
    /* facing/vel/sub_phase/life/frame/timer left 0 (the emitter sets velocity) */
    return slot;
}

void particle_fountain_emit(particle_pool *pool, int32_t emit_cx, int32_t emit_cy,
                            int *counter)
{
    if (pool == NULL || counter == NULL) return;

    /* 0x54f980:220 — the emitter's +0x5c tick counter, mod 30. */
    *counter = (*counter + 1) % 30;

    /* :229-232 — spawn jitter: Y (rand*400>>15)-3000, then X (rand*800>>15)-400.
     * The position is the emitter's anchor-mode-1 center + the jitter. */
    int32_t y_off = rng_scaled(400) - 3000;
    int32_t x_off = rng_scaled(800) - 400;
    int slot = particle_spawn_water(pool, emit_cx + x_off, emit_cy + y_off);

    /* :236-237 — two further LCG draws (the splash sound / secondary).  Consume
     * them so the per-tick draw count matches retail (6 per fountain tick). */
    (void)rng_rand();
    (void)rng_rand();

    if (slot < 0) return;
    actor_render_state *rs = &pool->states[slot];

    /* :260-283 — set the new droplet's launch velocity from the 3-way cycle
     * (counter % 3): all UP (vel_y ~ [-90000,-80000)) with a horizontal spread.
     * 0x453960 draws vel_y (+0x18) first, then vel_x (+0x28). */
    switch (*counter % 3) {
    case 0:  /* :264-269 (direct) — left-strong */
        rs->vel_y = -80000 - rng_scaled(10000);
        rs->vel_x = rng_scaled(10000) - 30000;
        break;
    case 1:  /* :274 0x453960(20000,10000,-80000,-10000) — right */
        rs->vel_y = rng_scaled(-10000) + (-80000);
        rs->vel_x = rng_scaled(10000) + 20000;
        break;
    default: /* :279 0x453960(-10000,10000,-80000,-10000) — left-weak */
        rs->vel_y = rng_scaled(-10000) + (-80000);
        rs->vel_x = rng_scaled(10000) + (-10000);
        break;
    }
}

/* 0x46e510 case 0x18708 (:529) — one sim-tick of the droplet's physics. */
static void step_water(actor_render_state *rs)
{
    /* :531-534 — gravity: vel_y += 8000, capped at 80000 (down). */
    int32_t v = rs->vel_y + 8000;
    if (v > 80000) v = 80000;
    rs->vel_y = v;

    /* :535-540 — integrate.  x += ±vel_x/100 (sign by facing: +/100 if facing==1,
     * else -/100); y += vel_y/100.  C's `/100` truncates toward zero, matching the
     * retail 0x51eb851f magic-number division. */
    int32_t sx = rs->vel_x / 100;
    if (rs->facing != 1) sx = -sx;
    rs->world_x += sx;
    rs->world_y += rs->vel_y / 100;

    /* :542-560 — every 3rd tick advance the fade sub-phase; expire at 8. */
    if (rs->life < 2) {
        rs->life++;
    } else {
        rs->life = 0;
        if (rs->sub_phase < 8) {
            rs->sub_phase++;
            /* retail: node_alpha = (&DAT_008a92e0)[-sub_phase] (the fade ramp,
             * = g_ramp_a[10 - sub_phase]).  The port realizes this in
             * particle_pool_render (param8 = 10 - sub_phase) so the present picks
             * the same descriptor — the fade is faithful, no node_alpha needed. */
        } else {
            rs->active = 0;   /* 0x556180 — free the slot */
            return;
        }
    }

    /* :561-586 — retail also expires on a collision-grid water/solid hit
     * ((x,y)/0xc80 -> mapctl+0x21c04).  PORT-DEBT(fountain-collide): approximated
     * by the sub_phase==8 lifetime above (~24 ticks, matching the observed span). */
}

void particle_pool_step(particle_pool *pool)
{
    if (pool == NULL) return;
    for (int i = 0; i < PARTICLE_POOL_SLOTS; i++) {
        actor_render_state *rs = &pool->states[i];
        if (!rs->active) continue;
        if (pool->actors[i].code == PARTICLE_CODE_WATER)
            step_water(rs);
        /* Advance the (RNG-free) clip frame in lockstep — the 2-frame water loop
         * cycles sprites 6,7.  anim_clip_advance no-ops when the slot just
         * expired (clip still set, but the next render skips inactive slots). */
        if (rs->active && rs->clip != NULL) {
            anim_state st = { .clip = rs->clip, .timer = rs->timer,
                              .frame = rs->frame, .done = rs->done };
            anim_clip_advance(&st);
            rs->timer = st.timer;
            rs->frame = st.frame;
            rs->done  = st.done;
        }
    }
}

int particle_pool_render(const particle_pool *pool, draw_pool *pool_out,
                         mr_sprite_fn resolve, void *resolve_ctx)
{
    if (pool == NULL || pool_out == NULL || resolve == NULL) return 0;
    int emitted = 0;
    for (int i = 0; i < PARTICLE_POOL_SLOTS; i++) {
        const actor_render_state *rs = &pool->states[i];
        if (!rs->active) continue;
        const actor *a = &pool->actors[i];

        /* 0x493480 default arm: describe the cel, then emit a MODE-1 (alpha)
         * node via 0x4917b0 (draw_pool_emit) — NOT the keyed actor emit.  The
         * +0x13e0 band always alpha-blends (engine-quirk #87): the node's alpha
         * (param8) is the brightness descriptor (&DAT_008a92e0)[-sub_phase] =
         * g_ramp_a[10 - sub_phase].  We carry the RAMP INDEX in param8 and let
         * the present (game_present_blit) pick the descriptor + orchestrate. */
        actor_desc d;
        if (!actor_render_describe(a, rs, /*flip_table=*/NULL, &d)) continue;
        uint32_t cel = resolve(d.bank, (uint16_t)d.frame, resolve_ctx);
        if (cel == 0) continue;

        int idx = 10 - (int)rs->sub_phase;   /* DAT_008a92e0[-sub_phase] */
        if (idx < 0) idx = 0;
        if (idx > 19) idx = 19;

        if (draw_pool_emit(pool_out, a->layer, /*mode=*/1u, cel,
                           rs->world_x, rs->world_y,
                           (uint32_t)(rs->dst_base_x + d.off_x),
                           (uint32_t)(rs->dst_base_y + d.off_y),
                           /*param8 = ramp_a index (10 - sub_phase)*/
                           (uint32_t)idx) != NULL)
            emitted++;
    }
    return emitted;
}
