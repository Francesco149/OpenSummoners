/*
 * src/title_render.c — title-screen render bridge (pure logic).
 *
 * Per-function provenance is in title_render.h; the full pipeline decode
 * is docs/findings/sprite-pipeline.md.
 */
#include "title_render.h"

#include "asset_register.h"

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
    int32_t idx = (entry->alpha_level * 20) / 1000;
    if (idx < 0) {
        idx = 0;
    } else if (idx >= TITLE_FADE_RAMP_LEN) {
        idx = TITLE_FADE_RAMP_LEN - 1;
    }
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
