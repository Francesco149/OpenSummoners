/*
 * actor_render.{c,h} — the town ACTOR render path: the static-actor descriptor
 * builder (FUN_0044d160) + the per-actor emit (the 0x491ae0 DEFAULT arm).
 *
 * The opening town has 33 active main-band actors (DAT_008a9b50+0x11e0, 128
 * slots).  32 of them render through ONE path: 0x491ae0's behaviour switch
 * has no explicit case for their codes, so they fall through to the default arm
 * (caseD_11257) — which calls FUN_0044d160 to build a 1-element sprite
 * descriptor, then emits one draw node (FUN_00492670, draw_pool_emit_actor)
 * into the view's 27-layer draw_pool.  So one function draws nearly the whole
 * town population (engine-quirk #78).  The behaviour code drives the actor's
 * AI/motion in 0x54f980, NOT its appearance — static scenery actors render
 * identically regardless of code.  (The 1 animated actor, code 0x1872d / the
 * protagonist, takes a separate inline arm that reads its clip — ported here as
 * actor_render_protagonist, a 3-cel composite over the same describe build.
 * RNG-driven motion is deferred per ckpt 73.)
 *
 * WHAT THIS MODULE PORTS (pure, host-tested):
 *   - actor_render_describe  = FUN_0044d160 exactly: pick the per-direction
 *     sprite-table row (actor+0x48, stride 0x14, indexed by actor+0xe8 dir),
 *     resolve the static/animated/mirrored/angle frame + placement offset into
 *     a 10-short descriptor.
 *   - actor_render_static    = the 0x491ae0 default arm: describe + emit one
 *     node (world pos from the render-state, the descriptor's bank/frame/offset,
 *     the actor's layer + alpha) through draw_pool_emit_actor.
 *
 * The render-state and actor are RUNTIME objects (not file data), so — like
 * anim_clip's anim_state — they are modelled as LOGICAL structs (named fields,
 * retail byte offsets in comments), not byte-accurate layouts; the spawn/entity
 * system (still to be RE'd) populates them.  Only the per-direction sprite row
 * (actor_sprite_row) is a real data layout and is pinned with _Static_assert.
 *
 * Win32-free: the cel resolver (FUN_00417c40) comes in as the mr_sprite_fn
 * callback (the same seam map_render_walk uses); the DAT_008a8440 mirror table
 * comes in as a plain pointer (NULL when no mirrored actor is in play).
 */
#ifndef OSS_ACTOR_RENDER_H
#define OSS_ACTOR_RENDER_H

#include <stdint.h>
#include <stddef.h>

#include "anim_clip.h"     /* anim_clip — the render-state's +0x6c clip */
#include "draw_pool.h"
#include "map_render.h"     /* mr_sprite_fn — the (bank,frame)->cel resolver */

/*
 * One per-direction sprite descriptor — the row at actor+0x48 + dir*0x14
 * (stride 0x14).  Pinned from FUN_0044d160's reads (row+0x00 bank, +0x02
 * frame_base, +0x08 mirror-x reference, +0x0c x offset, +0x10 y offset).
 */
typedef struct actor_sprite_row {
    uint16_t bank;        /* +0x00 — sprite bank (0 => this dir draws nothing) */
    int16_t  frame_base;  /* +0x02 — base frame within the bank               */
    int32_t  _u04;        /* +0x04 — unread by the renderer                   */
    int32_t  mirror_x;    /* +0x08 — x reference used when facing==3 (mirror)  */
    int32_t  x_off;       /* +0x0c — x placement offset (non-mirrored)        */
    int32_t  y_off;       /* +0x10 — y placement offset                       */
} actor_sprite_row;

#if defined(UINTPTR_MAX) && UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(actor_sprite_row, bank)       == 0x00, "row bank @0x00");
_Static_assert(offsetof(actor_sprite_row, frame_base) == 0x02, "row frame_base @0x02");
_Static_assert(offsetof(actor_sprite_row, mirror_x)   == 0x08, "row mirror_x @0x08");
_Static_assert(offsetof(actor_sprite_row, x_off)      == 0x0c, "row x_off @0x0c");
_Static_assert(offsetof(actor_sprite_row, y_off)      == 0x10, "row y_off @0x10");
_Static_assert(sizeof(actor_sprite_row)               == 0x14, "row is 0x14 bytes");
#endif

/* The per-direction sprite table spans actor+0x48..+0xe7 = 8 rows of 0x14,
 * with the dir field landing at +0xe8 right after. */
#define ACTOR_SPRITE_DIRS 8

/*
 * The actor (the +0x11e0 main-band entity) — the slice the renderer reads, at
 * the retail byte offsets noted.  Logical struct (the spawn fills it).
 */
typedef struct actor {
    actor_sprite_row sprite_table[ACTOR_SPRITE_DIRS]; /* +0x48 per-dir sprites */
    uint16_t dir;          /* +0xe8 — facing (sprite_table index)             */
    int32_t  angle_anim;   /* +0xec — non-zero => angle-driven frame select   */
    uint16_t angle_div;    /* +0xf0 — quantization divisor for the angle anim */
    int32_t  node_alpha;   /* +0xf4 — per-node alpha override (0 => desc.alpha)*/
    uint32_t layer;        /* +0xfc — draw_pool layer key for this actor       */
    uint32_t code;         /* +0x1d4 — behaviour code (dispatch key; doc only) */
    int32_t  skip;         /* +0x284 — non-zero => actor renders nothing       */
} actor;

/*
 * The actor render-state (the 0x294-byte entry *(actor+0x40) points at) — the
 * slice the renderer reads.  Logical struct; retail byte offsets in comments.
 */
typedef struct actor_render_state {
    uint8_t          active;      /* +0x00 — 0 => skip (the *param_1=='\0' gate) */
    int32_t          world_x;     /* +0x04 — world x (px*100)                  */
    int32_t          world_y;     /* +0x08 — world y                           */
    int32_t          vel_y;       /* +0x18 — particle y velocity (0x46e510)    */
    int32_t          vel_x;       /* +0x28 — particle x velocity (0x46e510)    */
    int32_t          facing;      /* +0x2c — 3 => mirrored (flip horizontally) */
    int32_t          angle;       /* +0x34 — accumulated angle (angle anim)    */
    int32_t          dst_base_x;  /* +0x40 — added to the descriptor off_x     */
    int32_t          dst_base_y;  /* +0x44 — added to the descriptor off_y     */
    uint16_t         sub_phase;   /* +0x58 — particle sub-state (fade/cycle)   */
    int32_t          life;        /* +0x5c — particle lifetime/step counter    */
    const anim_clip *clip;        /* +0x6c — current clip (NULL => static)     */
    uint16_t         timer;       /* +0x70 — sim-ticks elapsed in current frame */
    uint16_t         frame;       /* +0x72 — current frame index               */
    uint8_t          done;        /* +0x74 — one-shot finished flag            */
    const uint8_t   *layer_override; /* +0x284 — non-NULL => layer = *(it+0x100)*/
} actor_render_state;

/*
 * The 10-short sprite descriptor FUN_0044d160 builds and the 0x491ae0 emit
 * loop reads (the &local_140 / local_136 stack block).  Byte layout matches the
 * retail stack stores: off_x@+0x00, off_y@+0x04, bank@+0x08, frame@+0x0a,
 * alpha@+0x10 (FUN_0044d160 writes param_2[0..1]/[2](u16)/+10(s16)/[4]).
 */
typedef struct actor_desc {
    int32_t  off_x;   /* +0x00 */
    int32_t  off_y;   /* +0x04 */
    uint16_t bank;    /* +0x08 */
    int16_t  frame;   /* +0x0a */
    int16_t  _pad0c;  /* +0x0c */
    int16_t  _pad0e;  /* +0x0e */
    int32_t  alpha;   /* +0x10 */
} actor_desc;

/*
 * Port of FUN_0044d160 — build one sprite descriptor for an actor render-state.
 * Returns 1 and fills `*out` (off_x/off_y placement, bank, frame, alpha=0), or
 * 0 to skip the actor entirely (the direction's sprite bank is 0, the
 * render-state is inactive, or an animated clip has hit its -1 terminator).
 *
 *   STATIC (clip NULL):  off = (x_off, y_off) from the row, frame = frame_base.
 *   ANIMATED (clip set): off = the clip's per-frame (off_x[f], off_y[f]),
 *                        frame = frame_base + base_sprite + frame_delta[f];
 *                        clip.flag_abs => "absolute" offset (no row x_off added),
 *                        clip.link may override the direction.
 *   MIRRORED (facing==3): off_x = row.mirror_x - off_x, frame += flip_table[bank].
 *   ANGLE (actor.angle_anim): frame += a quantized function of render-state angle.
 *
 * `flip_table` is the DAT_008a8440 mirror table (indexed by sprite bank); it is
 * only read on the mirrored / angle+mirror paths — pass NULL when no such actor
 * is present (it is then never dereferenced).
 */
int actor_render_describe(const actor *a, const actor_render_state *rs,
                          const int16_t *flip_table, actor_desc *out);

/*
 * Port of 0x491ae0's DEFAULT arm (caseD_11257) — render one static-band
 * actor: honour the actor's skip flag (+0x284) and layer (+0xfc, overridable by
 * the render-state's +0x284 sub-object +0x100), build the descriptor
 * (actor_render_describe), resolve the cel (`resolve` = FUN_00417c40), and emit
 * one draw node (draw_pool_emit_actor = FUN_00492670) into `pool`.
 *
 * Returns 1 if a node was emitted, 0 if the actor was skipped (skip flag set,
 * describe returned 0, a NULL cel, or the target layer was full).
 *
 * The tile-occlusion mark (0x4927c0 at 0x491ae0:376, writing the grid's
 * occlusion buffer) is deferred — PORT-DEBT(actor-occlusion).  It affects which
 * backdrop tiles get culled BEHIND an actor, not the actor's own blit.
 */
int actor_render_static(const actor *a, const actor_render_state *rs,
                        const int16_t *flip_table, draw_pool *pool,
                        mr_sprite_fn resolve, void *resolve_ctx);

/*
 * The protagonist composite (0x491ae0:114-131) — the animated body (the
 * actor_render_describe result) plus TWO fixed left cels from bank 0x175.  The
 * three cels tile at a -128 px pitch (x = -256, -128, 0) into one wide
 * character sprite.  (Constants read straight off the inline stack stores:
 * local_138/local_136[9]=bank 0x175, local_136[0]/[10]=frames 0/1,
 * local_140=0xffffff00=-256, local_136[5..6]=0xffffff80=-128.)
 */
#define ACTOR_PROT_BANK        0x175  /* the protagonist sprite bank          */
#define ACTOR_PROT_PART0_FRAME 0      /* fixed left cel                       */
#define ACTOR_PROT_PART1_FRAME 1      /* fixed middle cel                     */
#define ACTOR_PROT_PART0_OFF_X (-256) /* local_140                            */
#define ACTOR_PROT_PART1_OFF_X (-128) /* local_136[5..6]                      */

/*
 * Port of 0x491ae0's case-0x1872d arm — render the animated protagonist as a
 * THREE-cel composite.  Part 2 (the animated body) is built by the same
 * FUN_0044d160 logic actor_render_describe ports (and carries all three of its
 * early-return gates: a zero direction bank, an inactive render-state, or a
 * clip-terminator frame skip the WHOLE actor, exactly as retail does); parts 0
 * and 1 are the two fixed bank-0x175 cels at x-256 / x-128.  All three emit on
 * the actor's layer, back-to-front (left cel first, body last).
 *
 * Returns the number of draw nodes emitted (0 if the actor was skipped, else up
 * to 3 — a part still contributes nothing if its cel resolves NULL or its layer
 * is full, matching the retail per-element emit).
 */
int actor_render_protagonist(const actor *a, const actor_render_state *rs,
                             const int16_t *flip_table, draw_pool *pool,
                             mr_sprite_fn resolve, void *resolve_ctx);

/*
 * The per-sim-tick animation step for one actor render-state — the deterministic
 * frame-stepper every animating behaviour in 0x54f980 runs (e.g. the
 * protagonist's case-0x1872d at 0x54f980:911-928), advancing the render-state's
 * anim sub-block (+0x6c clip / +0x70 timer / +0x72 frame / +0x74 done) by one
 * tick.  That block IS an anim_state, so this bridges to the single ported
 * stepper anim_clip_advance: a NULL clip (the 32 static town actors) is a no-op,
 * a looping clip cycles, a one-shot freezes on its last frame + sets `done`.
 *
 * This is ONLY the deterministic anim half of 0x54f980.  The RNG-driven
 * behaviour half (idle waits + wander, 0x54f980:929+, drawing the LCG
 * 0x5bf505) is deferred — defer-all-RNG-order-parity, ckpt 73 / engine-quirk
 * #77.  Drive it once per SIM-TICK via actor_pool_update.
 */
void actor_anim_advance(actor_render_state *rs);

#endif /* OSS_ACTOR_RENDER_H */
