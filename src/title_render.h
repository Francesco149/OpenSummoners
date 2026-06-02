/*
 * src/title_render.h — title-screen render bridge.
 *
 * The home for the sprite-display-list compositor + the per-sprite
 * wrappers that turn the title scene's draw stream into real ZDD blits.
 * This is the only module that bridges BOTH the asset/sprite pool
 * (asset_register.h: ar_sprite_slot + ar_sprite_slot_frame) AND the
 * DDraw wrapper (zdd.h: zdd_object + zdd_blit_orchestrate).
 *
 * Pure logic, Win32-free — every leaf it calls (ar_pool_get_slot,
 * ar_sprite_slot_frame, zdd_blit_orchestrate) is already ported, so this
 * module needs no _win32.c companion.
 *
 * Provenance + the full decode of the pipeline: see
 * docs/findings/sprite-pipeline.md.  Ported so far:
 *   - FUN_0056c180  title_compositor_draw  (the per-frame compositor that
 *                                           walks the scene's sprite-group
 *                                           display list)
 *     + title_compositor_resolve            (the per-entry resolution —
 *                                            frame-index math + ramp clamp +
 *                                            pool/frame lookup + geometry,
 *                                            factored out pure for testing)
 */
#ifndef OPENSUMMONERS_TITLE_RENDER_H
#define OPENSUMMONERS_TITLE_RENDER_H

#include <stddef.h>
#include <stdint.h>

#include "zdd.h"

/* ─── the sprite-group display-list entry (0x1c bytes) ───────────────
 *
 * The compositor walks an array of these (the scene's sprite group).
 * Positions are stored in centi-pixels (the compositor divides by 100).
 * Offsets pinned at 0x56c19b..0x56c28d.  +0x08 and +0x16 are never read
 * by the compositor — kept as pad to preserve the 0x1c record stride. */
typedef struct title_sprite_entry {
    int32_t   x_num;        /* +0x00 dst-left numerator  (÷100 + sprite metric_0c) */
    int32_t   y_num;        /* +0x04 dst-top  numerator  (÷100 + sprite metric_10) */
    uint32_t  _pad08;       /* +0x08 unread by the compositor                       */
    uint16_t  anim_num;     /* +0x0c animation progress numerator                   */
    uint16_t  anim_div;     /* +0x0e animation progress divisor                     */
    uint16_t  bank_id;      /* +0x10 index into the asset pool (ar_pool_get_slot)   */
    uint16_t  frame_base;   /* +0x12 first frame of the animation                   */
    uint16_t  frame_count;  /* +0x14 number of frames in the animation              */
    uint16_t  _pad16;       /* +0x16 unread by the compositor                       */
    int32_t   alpha_level;  /* +0x18 0..1000 fade level → blend-descriptor ramp idx */
} title_sprite_entry;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(title_sprite_entry)                 == 0x1c, "title_sprite_entry must be 0x1c bytes");
_Static_assert(offsetof(title_sprite_entry, x_num)        == 0x00, "entry x_num offset");
_Static_assert(offsetof(title_sprite_entry, y_num)        == 0x04, "entry y_num offset");
_Static_assert(offsetof(title_sprite_entry, anim_num)     == 0x0c, "entry anim_num offset");
_Static_assert(offsetof(title_sprite_entry, anim_div)     == 0x0e, "entry anim_div offset");
_Static_assert(offsetof(title_sprite_entry, bank_id)      == 0x10, "entry bank_id offset");
_Static_assert(offsetof(title_sprite_entry, frame_base)   == 0x12, "entry frame_base offset");
_Static_assert(offsetof(title_sprite_entry, frame_count)  == 0x14, "entry frame_count offset");
_Static_assert(offsetof(title_sprite_entry, alpha_level)  == 0x18, "entry alpha_level offset");
#endif

/* ─── the sprite-group header ────────────────────────────────────────
 *
 * The compositor reads only two fields off the group: the entries array
 * pointer (+0x00) and the entry count (u16 @+0x06).  In retail this is a
 * sub-object embedded in the title scene struct (the call site passes
 * `&scene[esp+0x38]`); we model just the two pinned fields plus the +0x04
 * gap.  Full header size is not yet pinned — only these two offsets are
 * asserted. */
typedef struct title_sprite_group {
    title_sprite_entry *entries;   /* +0x00 */
    uint16_t            cap;       /* +0x04 capacity (retail: 500 @0x56b6xx)*/
    uint16_t            count;     /* +0x06 entry count                    */
} title_sprite_group;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(title_sprite_group, entries) == 0x00, "group entries offset");
_Static_assert(offsetof(title_sprite_group, cap)     == 0x04, "group cap offset");
_Static_assert(offsetof(title_sprite_group, count)   == 0x06, "group count offset");
#endif

/* The blend-descriptor ramp the compositor clamps into.  Retail reads the
 * 20-entry pointer table at 0x8a92b8 (DAT_008a92b8 — the same table
 * pixel_drawer fills as g_pd_boot_group_a, viewed here as an array of
 * blend-descriptor pointers).  Index <0 clamps to [0], >=20 to [19]
 * (retail's [0x8a9304] default literally == &table[19]).  Each entry may
 * be NULL (table not yet populated / ramp 0) ⇒ the blit becomes a no-op
 * at the alpha-blit layer (see zdd_alpha_blit's NULL-desc guard). */
#define TITLE_FADE_RAMP_LEN 20

/* The resolved per-entry blit parameters — the output of the pure
 * resolution step, exactly the args the compositor hands to
 * zdd_blit_orchestrate (src_x/src_y are always 0; gdi_ctx always NULL). */
typedef struct title_blit_params {
    int                   valid;     /* 0 ⇒ skip this entry (NULL sprite/div0) */
    const zdd_blend_desc *desc;      /* clamped ramp entry (may be NULL)        */
    zdd_object           *src;       /* resolved sprite frame surface           */
    int32_t               dst_x;     /* sprite->metric_0c + x_num/100           */
    int32_t               dst_y;     /* sprite->metric_10 + y_num/100           */
    int32_t               width;     /* sprite->metric_14                       */
    int32_t               height;    /* sprite->metric_18                       */
    int32_t               colorkey;  /* sprite->colorkey_out                    */
} title_blit_params;

/* FUN_0056c180 (per-entry body) — resolve one display-list entry to its
 * blit parameters.  Pure: the frame-index animation math, the ramp-index
 * clamp, the asset-pool + frame-surface lookup (ar_pool_get_slot →
 * ar_sprite_slot_frame), and the centi-pixel geometry.  No surfaces are
 * touched beyond reading the resolved sprite's metric/colorkey fields.
 *
 *   p     = min((anim_num * frame_count) / anim_div, frame_count - 1)
 *   frame = (uint16_t)(frame_base - p + frame_count - 1)
 *   src   = ar_sprite_slot_frame(ar_pool_get_slot(bank_id), frame)
 *   idx   = clamp((alpha_level * 20) / 1000, 0, 19)
 *   desc  = ramp ? ramp[idx] : NULL
 *   dst_x = src->metric_0c + x_num/100;  dst_y = src->metric_10 + y_num/100
 *
 * Returns {.valid=0} when anim_div == 0 (retail divides by zero here —
 * headless safety) or the sprite resolves to NULL (unloaded bank; retail
 * dereferences NULL).  Both are the same defensive class as the getter's
 * NULL return. */
title_blit_params title_compositor_resolve(const title_sprite_entry *entry,
                                           const zdd_blend_desc *const *ramp);

/* Optional blit hook (porting affordance for host tests).  NULL ⇒ the
 * draw loop calls zdd_blit_orchestrate directly (the retail path); a test
 * installs a capturing hook to observe per-entry forwarding without real
 * locked surfaces.  Signature matches zdd_blit_orchestrate exactly. */
typedef void (*title_blit_fn)(const zdd_blend_desc *desc, zdd_object *dest,
                              zdd_object *src,
                              int32_t dst_x, int32_t dst_y,
                              int32_t width, int32_t height,
                              int32_t src_x, int32_t src_y,
                              int32_t colorkey, zdd_object *gdi_ctx);
extern title_blit_fn title_compositor_blit_hook;

/* FUN_0056c180 — the per-frame compositor.  Walks `group`'s display list
 * ([0, group->count)) and blits each entry onto `dest` (the primary
 * surface = DAT_008a93cc->[0x16c] at the retail call site), resolving the
 * source frame + blend descriptor + geometry via title_compositor_resolve.
 * `ramp` is the 20-entry blend-descriptor pointer table (0x8a92b8); NULL
 * is tolerated (all blits become no-ops).  Entries that resolve invalid
 * are skipped.  Each surviving entry forwards to zdd_blit_orchestrate
 * (or title_compositor_blit_hook if installed) with src_x=src_y=0,
 * gdi_ctx=NULL (DAT_008a6ec0 is always NULL — engine-quirk #45). */
void title_compositor_draw(const title_sprite_group *group, zdd_object *dest,
                           const zdd_blend_desc *const *ramp);

/* ─── the per-sprite wrappers (0x56c610 / _4e0 / _470) ────────────────
 *
 * Thin per-sprite forwards the title render half calls per phase.  In
 * retail each re-derives the dest (the primary surface) from
 * `DAT_008a93cc->[0x16c]` (the global ZDD's primary_obj); we take it as
 * the `dest` parameter (the same surface the caller passes as the keyed
 * paths' `a1`), keeping this module free of a god-object global until the
 * sink lands.  `sprite` is the already-resolved frame surface
 * (ar_sprite_slot_frame).  `x`/`y` are plain dst offsets (NOT centi-pixel
 * — unlike the compositor); placement adds the sprite's metric_0c/_10
 * (inside zdd_object_blt_keyed on the keyed path, explicitly on the alpha
 * path — same result).
 *
 * The deferred sparkle wrapper 0x56c580 is NOT here yet — it forwards to
 * the unported 0x5b9bf0 (a 256 B blt_keyed sibling) on one path. */

/* Optional keyed-blit hook (porting affordance for host tests, mirrors
 * title_compositor_blit_hook).  NULL ⇒ the wrappers call
 * zdd_object_blt_keyed directly.  Signature matches it exactly. */
typedef int (*title_keyed_fn)(zdd_object *self, zdd_object *dest,
                              int32_t dest_x, int32_t dest_y);
extern title_keyed_fn title_keyed_blit_hook;

/* FUN_0056c610 — plain sprite (TITLE_DRAW_SPRITE).  Straight color-keyed
 * Blt, no alpha: zdd_object_blt_keyed(sprite, dest, x, y).  (Retail's
 * unused first stack arg is dropped here.) */
void title_draw_sprite(zdd_object *dest, zdd_object *sprite,
                       int32_t x, int32_t y);

/* FUN_0056c4e0 — leveled sprite (TITLE_DRAW_SPRITE_LEVEL).  Fades via the
 * 0x8a9308 ramp (`ramp_b`):
 *   if (level_num <= 0) return;                       // nothing drawn
 *   idx = (level_div <= 0) ? 0 : (level_num*20)/level_div, clamped ≥0
 *   if (idx >= 20 || ramp_b[idx] == NULL)             // ramp 0 / past end
 *       zdd_object_blt_keyed(sprite, dest, x, y);     // plain opaque
 *   else
 *       zdd_blit_orchestrate(ramp_b[idx], dest, sprite,
 *                            sprite->metric_0c + x, sprite->metric_10 + y,
 *                            sprite->metric_14, sprite->metric_18,
 *                            0, 0, sprite->colorkey_out, NULL);
 * `ramp_b` NULL ⇒ every level resolves to the plain path. */
void title_draw_sprite_level(zdd_object *dest, zdd_object *sprite,
                             int32_t level_num, int32_t level_div,
                             int32_t x, int32_t y,
                             const zdd_blend_desc *const *ramp_b);

/* FUN_0056c470 — menu cursor (TITLE_DRAW_MENU_CURSOR).  Always alpha via
 * the 0x8a92b8 ramp (`ramp_a`, same table as the compositor):
 *   if (level_num <= 0) return;
 *   idx = clamp((level_num*20)/level_div, 0, 19)      // [0x8a9304] == &ramp_a[19]
 *   zdd_blit_orchestrate(ramp_a[idx], dest, sprite,
 *                        sprite->metric_0c + x, sprite->metric_10 + y,
 *                        sprite->metric_14, sprite->metric_18,
 *                        0, 0, sprite->colorkey_out, NULL);
 * Retail idivs by level_div with no guard (faults on 0); we skip the draw
 * for headless safety (same class as the compositor's div-0 skip). */
void title_draw_menu_cursor(zdd_object *dest, zdd_object *sprite,
                            int32_t level_num, int32_t level_div,
                            int32_t x, int32_t y,
                            const zdd_blend_desc *const *ramp_a);

/* Optional clipped-blit hook (mirrors title_keyed_blit_hook).  NULL ⇒ the
 * sparkle wrapper's non-alpha path calls zdd_object_blt_clipped directly. */
typedef int (*title_clipped_fn)(zdd_object *src, zdd_object *dest,
                                int32_t dst_x, int32_t dst_y,
                                int32_t width, int32_t height,
                                int32_t src_x, int32_t src_y);
extern title_clipped_fn title_clipped_blit_hook;

/* FUN_0056c580 — sparkle / trail sprite (TITLE_DRAW_SPARKLE).  Unlike the
 * other wrappers it carries an explicit source sub-rect (src_x/src_y +
 * width/height) and is gated directly on a caller-supplied blend
 * descriptor `desc`:
 *   if (desc != NULL)   // alpha trail
 *     zdd_blit_orchestrate(desc, dest, sprite,
 *                          sprite->metric_0c + dst_x, sprite->metric_10 + dst_y,
 *                          width, height, src_x, src_y,
 *                          sprite->colorkey_out, NULL);
 *   else                // opaque clipped copy
 *     zdd_object_blt_clipped(sprite, dest, dst_x, dst_y,
 *                            width, height, src_x, src_y);
 * Note the asymmetry mirrors retail: the alpha path adds the sprite's
 * placement metric to the dest origin and passes the explicit src rect,
 * while the clipped path passes the raw dest origin (zdd_object_blt_clipped
 * applies the metric as its clip origin instead).  `dest` is the primary
 * surface (retail re-derives it from 0x8a93cc->[0x16c] on the clipped
 * path; same surface as the alpha path's a1). */
void title_draw_sparkle(zdd_object *dest, zdd_object *sprite,
                        int32_t dst_x, int32_t dst_y,
                        int32_t width, int32_t height,
                        int32_t src_x, int32_t src_y,
                        const zdd_blend_desc *desc);

#endif /* OPENSUMMONERS_TITLE_RENDER_H */
