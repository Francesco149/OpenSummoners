/*
 * src/title_render.c — title-screen render bridge (pure logic).
 *
 * Per-function provenance is in title_render.h; the full pipeline decode
 * is docs/findings/sprite-pipeline.md.
 */
#include "title_render.h"

#include "asset_register.h"

/* Clamp a blend-ramp index to [0, 19] — retail's "<0 → table[0]; >=20 →
 * [0x8a9304] (== &table[19])" pattern, shared by the compositor and the
 * cursor wrapper.  (The level wrapper does NOT use this: its >=20 case
 * diverts to the plain keyed path instead of clamping.) */
static int32_t ramp_clamp(int32_t idx)
{
    if (idx < 0)                       return 0;
    if (idx >= TITLE_FADE_RAMP_LEN)    return TITLE_FADE_RAMP_LEN - 1;
    return idx;
}

/* ─── FUN_0056c180 (per-entry body) — title_compositor_resolve ────────
 *
 * Mirrors the retail per-iteration body at 0x56c19b..0x56c28d.  All of
 * the divisions retail performs with magic-number reciprocals
 * (0x51eb851f → ÷100, 0x10624dd3 → ÷1000, and the idiv ÷anim_div) are
 * signed and truncate toward zero — exactly C integer division on int32.
 */
title_blit_params title_compositor_resolve(const title_sprite_entry *entry,
                                           const zdd_blend_desc *const *ramp)
{
    title_blit_params out;
    out.valid = 0;
    out.desc = NULL;
    out.src = NULL;
    out.dst_x = out.dst_y = out.width = out.height = out.colorkey = 0;

    if (entry == NULL) {
        return out;
    }

    /* Animation frame index.  Retail reads these as u16; the multiply and
     * idiv run on the zero-extended 32-bit values. */
    int32_t fc = (int32_t)entry->frame_count;
    int32_t an = (int32_t)entry->anim_num;
    int32_t ad = (int32_t)entry->anim_div;

    if (ad == 0) {
        /* Retail issues `idiv` here unconditionally — a zero divisor faults.
         * Skip the entry for headless safety (same defensive class as the
         * NULL-sprite skip below). */
        return out;
    }

    int32_t p = (an * fc) / ad;           /* signed, toward zero            */
    if (p > fc - 1) {
        p = fc - 1;                       /* min(p, frame_count - 1)        */
    }
    /* frame_base - p + frame_count - 1, masked to 16 bits (retail's `sub cx`
     * is 16-bit and the final `and esi, 0xffff` clips the lea result). */
    uint16_t frame = (uint16_t)((int32_t)entry->frame_base - p + fc - 1);

    /* Asset-pool bank → frame surface (FUN_00418470 inlined in retail). */
    ar_sprite_slot *slot = ar_pool_get_slot(entry->bank_id);
    zdd_object *src = (zdd_object *)ar_sprite_slot_frame(slot, frame);
    if (src == NULL) {
        /* Unloaded / unresolved bank.  Retail dereferences NULL; we skip. */
        return out;
    }

    /* Blend-descriptor ramp index: (alpha_level * 20) / 1000, clamped to
     * [0, 19].  Retail's <0 → table[0]; >=20 → [0x8a9304] (== &table[19]). */
    int32_t idx = ramp_clamp((entry->alpha_level * 20) / 1000);
    out.desc = (ramp != NULL) ? ramp[idx] : NULL;

    /* Centi-pixel geometry: the sprite's placement metric + numerator/100. */
    out.src      = src;
    out.dst_x    = src->metric_0c + entry->x_num / 100;
    out.dst_y    = src->metric_10 + entry->y_num / 100;
    out.width    = src->metric_14;
    out.height   = src->metric_18;
    out.colorkey = src->colorkey_out;
    out.valid    = 1;
    return out;
}

/* ─── FUN_0056c180 — the per-frame compositor ─────────────────────────── */

title_blit_fn title_compositor_blit_hook = NULL;

void title_compositor_draw(const title_sprite_group *group, zdd_object *dest,
                           const zdd_blend_desc *const *ramp)
{
    if (group == NULL || group->entries == NULL) {
        return;                            /* retail's count<=0 early-out    */
    }

    for (uint32_t i = 0; i < group->count; i++) {
        title_blit_params p = title_compositor_resolve(&group->entries[i], ramp);
        if (!p.valid) {
            continue;
        }
        if (title_compositor_blit_hook != NULL) {
            title_compositor_blit_hook(p.desc, dest, p.src,
                                       p.dst_x, p.dst_y, p.width, p.height,
                                       0, 0, p.colorkey, NULL);
        } else {
            zdd_blit_orchestrate(p.desc, dest, p.src,
                                 p.dst_x, p.dst_y, p.width, p.height,
                                 0, 0, p.colorkey, NULL);
        }
    }
}

/* ─── the per-sprite wrappers (0x56c610 / _4e0 / _470) ────────────────── */

title_keyed_fn title_keyed_blit_hook = NULL;

/* Internal: the keyed-blit + alpha-blit forwards behind their test hooks. */
static int keyed_blit(zdd_object *sprite, zdd_object *dest,
                      int32_t x, int32_t y)
{
    if (title_keyed_blit_hook != NULL) {
        return title_keyed_blit_hook(sprite, dest, x, y);
    }
    return zdd_object_blt_keyed(sprite, dest, x, y);
}

static void alpha_blit(const zdd_blend_desc *desc, zdd_object *dest,
                       zdd_object *sprite, int32_t x, int32_t y)
{
    int32_t dst_x = sprite->metric_0c + x;
    int32_t dst_y = sprite->metric_10 + y;
    if (title_compositor_blit_hook != NULL) {
        title_compositor_blit_hook(desc, dest, sprite, dst_x, dst_y,
                                   sprite->metric_14, sprite->metric_18,
                                   0, 0, sprite->colorkey_out, NULL);
    } else {
        zdd_blit_orchestrate(desc, dest, sprite, dst_x, dst_y,
                             sprite->metric_14, sprite->metric_18,
                             0, 0, sprite->colorkey_out, NULL);
    }
}

/* FUN_0056c610 — plain sprite. */
void title_draw_sprite(zdd_object *dest, zdd_object *sprite,
                       int32_t x, int32_t y)
{
    keyed_blit(sprite, dest, x, y);
}

/* FUN_0056c4e0 — leveled sprite (0x8a9308 ramp). */
void title_draw_sprite_level(zdd_object *dest, zdd_object *sprite,
                             int32_t level_num, int32_t level_div,
                             int32_t x, int32_t y,
                             const zdd_blend_desc *const *ramp_b)
{
    if (level_num <= 0) {
        return;                            /* test eax; jle ret              */
    }

    int32_t idx;
    if (level_div <= 0) {
        idx = 0;                           /* test ecx; jg .. else xor eax   */
    } else {
        idx = (level_num * 20) / level_div;
        if (idx < 0) {
            idx = 0;                       /* test eax; jge .. else xor eax  */
        }
    }

    /* idx >= 20 diverts straight to the plain path (retail never loads the
     * ramp there); otherwise a NULL ramp entry (ramp 0) also falls to plain. */
    const zdd_blend_desc *desc = NULL;
    if (idx < TITLE_FADE_RAMP_LEN) {
        desc = (ramp_b != NULL) ? ramp_b[idx] : NULL;
    }

    if (desc == NULL) {
        keyed_blit(sprite, dest, x, y);    /* plain opaque copy              */
    } else {
        alpha_blit(desc, dest, sprite, x, y);
    }
}

/* FUN_0056c470 — menu cursor (0x8a92b8 ramp; always alpha). */
void title_draw_menu_cursor(zdd_object *dest, zdd_object *sprite,
                            int32_t level_num, int32_t level_div,
                            int32_t x, int32_t y,
                            const zdd_blend_desc *const *ramp_a)
{
    if (level_num <= 0) {
        return;                            /* test eax; jle ret              */
    }
    if (level_div == 0) {
        return;                            /* retail idivs unguarded (faults);
                                            * skip for headless safety        */
    }

    int32_t idx = ramp_clamp((level_num * 20) / level_div);
    const zdd_blend_desc *desc = (ramp_a != NULL) ? ramp_a[idx] : NULL;
    alpha_blit(desc, dest, sprite, x, y);
}
