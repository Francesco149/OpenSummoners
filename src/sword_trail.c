/*
 * sword_trail.c — the freeroam UP-attack sword-tip TRAIL (res 0x40b sparkles).
 * See sword_trail.h for the model + provenance.  Mirrors particle.c's pool
 * shape, but the emitter is a CAPTURED tip-arc table (the faithful stand-in for
 * the un-ported retail geometry 0x4505c0) rather than a physics integrator —
 * the sparkles are STATIC after-images that only shrink + fade in place.
 */
#include "sword_trail.h"

#include <string.h>

#include "character.h"      /* CHAR_FACE_RIGHT / CHAR_FACE_LEFT                 */

/* res 0x40b bank-13 frame base: the sparkle clip runs frames 24..31. */
#define SWORD_TRAIL_FRAME_BASE 24
/* code 0x186f2 — the 0x45e830-case-2 sword-trail particle (0x557370 tag). */
#define SWORD_TRAIL_CODE 0x186f2u
/*
 * The render LAYER.  0x493480's +0x13e0 DEVICE-band cluster draws at layer 11
 * (the "square", == the fountain water; particle.c PARTICLE_LAYER_WATER) — the
 * sword tip rides ABOVE Arche's head through the overhead thrust, so layer 11
 * (behind the layer-13 cast) matches the recording's z (verified off the .osr
 * draw sequence — the sparkles sit behind the body cel, peeking past the blade).
 */
#define SWORD_TRAIL_LAYER 11u
/*
 * The additive ramp descriptor index.  sword2.osr shows EVERY trail sparkle at
 * ONE constant blend descriptor (blend_ref 39, LUT md5 727d856f) — NOT the
 * per-age ramp the sky particle uses — so the visual fade is the shrinking frame
 * alone.  PINNED by LUT identity (the boot dump of g_ramp_a/b vs the recording):
 * 727d856f is the port's **g_ramp_a[19]**.  This reconciles with the decompile's
 * ramp_b path (46e510 case 0x186f2 -> DAT_008a9308[..]): retail's ramp tables are
 * contiguous 0x50-byte PdBlends, so DAT_008a9308 (ramp_b base) = DAT_008a92b8
 * (ramp_a base) + 0x50 -> ramp_b[18] IS ramp_a[19], the same descriptor.  The
 * port's g_ramp_a/g_ramp_b are SEPARATE arrays (no overlap), so the trail emits
 * the ramp_A entry (no PARTICLE_PARAM8_RAMP_B bit) at index 19. */
#define SWORD_TRAIL_RAMP_IDX 19u
/*
 * The fr=24 cel origin (metric_0c/10), from the recording: ox=oy=6.  The tip-arc
 * table is the fr=24 BLIT dst relative to Arche's stationary up-pose anchor; the
 * present adds the cel's own (growing) origin, so subtracting the fr=24 origin
 * here lands fr=24 exactly on the captured arc and lets the later frames drift
 * +1px/tick (the centred shrink) automatically. */
#define SWORD_TRAIL_OX0 6

/*
 * The sparkle clip 24->31 (res 0x40b, bank 0x1a4): 8 frames, dur 2 (+1 frame
 * every 2 sim-ticks = a 16t one-shot), then `done` expires the slot.  off_x/off_y
 * 0 — the per-frame centring is the cel's own metric_0c origin (added at present),
 * not a clip offset.  Decoded from the recording (frames 24=22x22 .. 31=6x6).
 */
static const anim_clip SWORD_TRAIL_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1, 2, 3, 4, 5, 6, 7 },
    .frame_count = 8,
    .frame_dur   = 2,
    .oneshot     = 1,    /* play-once — expires on completion */
};

/*
 * The captured sword-tip ARC — sword2.osr, the first up-attack (ticks 3889-3897),
 * extracted RELATIVE to Arche's stationary up-pose screen anchor (the finding's
 * (270,336), == project(world)+dst_base).  Two sparkles per swing tick, the fr=24
 * blit dst offset (screen px), RIGHT-facing.  LEFT mirrors dx about the anchor.
 * findings/freeroam-sword-attack.md "## chip 2c ... The sword-tip arc".
 */
static const int16_t SWORD_TRAIL_TIP_ARC[SWORD_TRAIL_EMIT_TICKS][2][2] = {
    /*  sparkle A        sparkle B       swing-tick */
    { { +36, +53 }, { +42, +52 } },   /*  9 */
    { { +48, +50 }, { +53, +48 } },   /* 10 */
    { { +58, +45 }, { +63, +42 } },   /* 11 */
    { { +68, +37 }, { +71, +33 } },   /* 12 */
    { { +74, +28 }, { +77, +22 } },   /* 13 */
    { { +78, +16 }, { +78, +10 } },   /* 14 */
    { { +78,  +4 }, { +76,  -2 } },   /* 15 */
    { { +73,  -7 }, { +70, -12 } },   /* 16 */
    { { +65, -16 }, { +60, -18 } },   /* 17 */
};

void sword_trail_reset(sword_trail *t)
{
    if (t == NULL) return;
    memset(t, 0, sizeof *t);
}

/* 0x557370: round-robin from the cursor to the first free slot. */
static int sword_trail_alloc(sword_trail *t)
{
    for (int i = 0; i < SWORD_TRAIL_SLOTS; i++) {
        uint16_t idx = t->cursor;
        t->cursor = (uint16_t)((t->cursor + 1) & (SWORD_TRAIL_SLOTS - 1));
        if (!t->states[idx].active) return idx;
    }
    return -1;   /* full — never fires (18 alive in a 32 pool) */
}

/* Install one sparkle at world (wx, wy) with screen anchor offset (ox, oy). */
static void sword_trail_spawn(sword_trail *t, int32_t wx, int32_t wy,
                              int32_t base_dx, int32_t base_dy)
{
    int slot = sword_trail_alloc(t);
    if (slot < 0) return;

    actor *a = &t->actors[slot];
    actor_render_state *rs = &t->states[slot];
    memset(a, 0, sizeof *a);
    memset(rs, 0, sizeof *rs);

    a->sprite_table[0].bank       = SWORD_TRAIL_BANK;
    a->sprite_table[0].frame_base = SWORD_TRAIL_FRAME_BASE;
    a->dir   = 0;
    a->code  = SWORD_TRAIL_CODE;
    a->layer = SWORD_TRAIL_LAYER;

    rs->active     = 1;
    rs->world_x    = wx;
    rs->world_y    = wy;
    rs->facing     = CHAR_FACE_RIGHT;  /* the dx mirror is baked into base_dx */
    rs->dst_base_x = base_dx;
    rs->dst_base_y = base_dy;
    rs->clip       = &SWORD_TRAIL_CLIP;   /* frames 24..31, dur-2 one-shot */
    /* frame/timer/done left 0 — renders fr=24 unstepped until the next step */
}

void sword_trail_emit(sword_trail *t, int swing_tick,
                      int32_t anchor_wx, int32_t anchor_wy,
                      int32_t base_dx, int32_t base_dy, int facing)
{
    if (t == NULL) return;
    int k = swing_tick - SWORD_TRAIL_EMIT_START;
    if (k < 0 || k >= SWORD_TRAIL_EMIT_TICKS) return;   /* outside the window */

    /* Each sparkle co-locates with Arche's world pos (she is stationary through
     * the thrust); the tip-arc offset rides in dst_base (screen px, added post
     * -projection), minus the fr=24 cel origin so fr=24 lands on the arc.  LEFT
     * mirrors the arc dx about the anchor (facing==3 -> negate). */
    int mir = (facing == CHAR_FACE_LEFT) ? -1 : 1;
    for (int s = 0; s < 2; s++) {
        int32_t tip_dx = SWORD_TRAIL_TIP_ARC[k][s][0];
        int32_t tip_dy = SWORD_TRAIL_TIP_ARC[k][s][1];
        int32_t dx = base_dx + mir * tip_dx - SWORD_TRAIL_OX0;
        int32_t dy = base_dy + tip_dy - SWORD_TRAIL_OX0;
        sword_trail_spawn(t, anchor_wx, anchor_wy, dx, dy);
    }
}

void sword_trail_step(sword_trail *t)
{
    if (t == NULL) return;
    for (int i = 0; i < SWORD_TRAIL_SLOTS; i++) {
        actor_render_state *rs = &t->states[i];
        if (!rs->active || rs->clip == NULL) continue;
        /* Advance the 24->31 one-shot; expire the slot when it finishes. */
        anim_state st = { .clip = rs->clip, .timer = rs->timer,
                          .frame = rs->frame, .done = rs->done };
        anim_clip_advance(&st);
        rs->timer = st.timer;
        rs->frame = st.frame;
        rs->done  = st.done;
        if (rs->done) rs->active = 0;   /* lifetime ~16t (8 frames * dur 2) */
    }
}

int sword_trail_render(const sword_trail *t, draw_pool *pool,
                       mr_sprite_fn resolve, void *resolve_ctx)
{
    if (t == NULL || pool == NULL || resolve == NULL) return 0;
    int emitted = 0;
    for (int i = 0; i < SWORD_TRAIL_SLOTS; i++) {
        const actor_render_state *rs = &t->states[i];
        if (!rs->active) continue;
        const actor *a = &t->actors[i];

        actor_desc d;
        if (!actor_render_describe(a, rs, /*flip_table=*/NULL, &d)) continue;
        uint32_t cel = resolve(d.bank, (uint16_t)d.frame, resolve_ctx);
        if (cel == 0) continue;

        /* 0x493480 default arm: a MODE-1 (alpha) node carrying the constant
         * additive ramp_A descriptor index (engine-quirk #87); the present
         * (PRESENT_ALPHA) blends the cel through g_ramp_a[idx] (no RAMP_B bit). */
        uint32_t param8 = SWORD_TRAIL_RAMP_IDX;
        if (draw_pool_emit(pool, a->layer, /*mode=*/1u, cel,
                           rs->world_x, rs->world_y,
                           (uint32_t)(rs->dst_base_x + d.off_x),
                           (uint32_t)(rs->dst_base_y + d.off_y),
                           param8) != NULL)
            emitted++;
    }
    return emitted;
}
