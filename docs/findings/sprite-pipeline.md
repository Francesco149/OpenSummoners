# Sprite display-list pipeline — asset pool → bank → compositor

How a title-screen frame turns the scene's *sprite-group display list* into
actual alpha-blended pixels. This is the chain behind the render sink's
`TITLE_DRAW_FRAME_END` (and, via the wrappers, every other sprite draw op).

Read with `title-scene.md` (the render half that emits the draw stream) and
`ddraw-init.md` (the ZDDObject surfaces these blits target).

## The big picture

```
title render half (FUN_0056aea0 render branch)
  ├─ per-phase handlers → sprite wrappers 0x56c610/_4e0/_470/_580
  │     each resolves ONE sprite + blits it onto the primary surface
  └─ frame-end 0x56bec4 → compositor 0x56c180
        walks the scene's sprite-group display list, blits EACH entry

both funnel through:
  zdd_blit_orchestrate (0x5bd550, ported) ── simple path ──▶ software alpha blit

each blit needs a SOURCE SURFACE, resolved out of:
  asset pool  DAT_008a760c[bank_id]      → ar_sprite_slot  (the "bank")
     bank->entries[0].frames[frame_idx]  → zdd_object*      (the frame surface)
        ↑ lazily decoded by FUN_004184a0 (the sprite-sheet loader) on first use
```

## The asset pool — `DAT_008a760c`

A 909-entry array of `ar_sprite_slot*` (the unified sprite pool, see
`0057ca40-rabbit-hole.md` §7 and `asset_register.h`/`ar_pool_get_slot`).
Index 0 is the allocator-zeroed sentinel; 1..12 ramp slots, 13..908 the
main slots. The compositor and the wrappers index it directly by a 16-bit
`bank_id` taken from the display-list entry.

`ar_sprite_slot` (0x44 B, fully pinned in `asset_register.h`) **is** the
"sprite bank". The fields the render side reads:

| off  | field          | meaning (render side)                          |
|------|----------------|------------------------------------------------|
| 0x00 | `entries`      | ptr to `ar_sprite_entry[]`; `entries[0].frames` is the frame-surface array |
| 0x20 | `width`        | (decoder input) sheet width                    |
| 0x24 | `height`       | (decoder input) sheet height                   |
| 0x28 | `colorkey`     | transparent COLORREF                           |
| 0x38 | `f_38`         | **frame count** — written by the decoder/slicer FUN_004188b0 |
| 0x40 | `resource_id`  | PE "DATA" resource id (the sheet bytes)        |

> `ar_sprite_entry.frames` (off +0x00) was the opaque `uint32_t a` until
> ckpt 16 — widened to `void *frames` (still 4 B on the 32-bit build, so
> the 8-byte record size holds) once the getter pinned its role. The
> per-entry `.b` (+0x04) is a separately-owned aux buffer the slot
> destructor frees; `.frames` is never freed (the surfaces belong to the
> ZDD).

## The frame getter — `FUN_00418470` (PORTED, ckpt 16)

`ar_sprite_slot_frame(slot, frame_id)` in `asset_register.c`. Two-level
indirection + lazy decode:

```
this = ecx = ar_sprite_slot*
eax = *this              ; slot->entries
if (*eax == 0)           ; entries[0].frames == NULL  → not yet decoded
    FUN_004184a0(this)   ; decode the sheet (the lazy loader)
edx = *this              ; slot->entries
eax = *edx               ; entries[0].frames  (the zdd_object*[] array)
return eax[id & 0xffff]  ; the frame surface
```

The decoder `FUN_004184a0` is routed through the nullable
`ar_sprite_decode_hook` so the getter ports + tests now; headless (no hook)
returns NULL where retail would decode-then-deref. The decoder is the
**next chip** (see below).

## The display-list entry — 0x1c bytes

The compositor walks an array of these (the scene's sprite group; the
header is `{ entries@+0x00; count(u16)@+0x06 }`). Positions are stored in
**centi-pixels** (the compositor divides by 100). Offsets pinned at
0x56c19b..0x56c28d:

| off  | type | name        | role                                         |
|------|------|-------------|----------------------------------------------|
| 0x00 | i32  | x_num       | dst-left numerator (÷100, + sprite metric_0c)|
| 0x04 | i32  | y_num       | dst-top numerator (÷100, + sprite metric_10) |
| 0x0c | u16  | anim_num    | animation progress numerator                 |
| 0x0e | u16  | anim_div    | animation progress divisor                   |
| 0x10 | u16  | bank_id     | index into the asset pool DAT_008a760c       |
| 0x12 | u16  | frame_base  | first frame of the animation                 |
| 0x14 | u16  | frame_count | number of frames in the animation            |
| 0x18 | i32  | alpha_level | 0..1000 fade level → blend-descriptor ramp   |

## The compositor — `FUN_0056c180` (PORTED, ckpt 17)

Ported as **`title_compositor_draw`** (+ the pure per-entry helper
**`title_compositor_resolve`**) in the NEW render-bridge module
**`src/title_render.{c,h}`** — the first module to include both
`asset_register.h` (pool/getter) and `zdd.h` (blit + surfaces).  The
display-list entry (0x1c B) + group header are modeled as
`title_sprite_entry` / `title_sprite_group` (offsets `_Static_assert`-pinned).
The ramp `0x8a92b8` is passed in as a 20-entry `const zdd_blend_desc *const *`
(NULL ⇒ all blits no-op), matching the `title_fade_ramp` decoupling pattern;
in retail this IS pixel_drawer's `g_pd_boot_group_a` viewed as a pointer
table.  The per-entry blit forwards to `zdd_blit_orchestrate` directly
(or `title_compositor_blit_hook` when a test installs one).  Decode below
is the original recovery.



Call site (render frame-end 0x56bec4): `ecx = &scene[esp+0x38]` (the scene-
local sprite group), `edx = DAT_008a93cc->[0x16c]` (the primary surface =
the blit dest). `void __thiscall(group, dest)`.

Per entry `i` in `[0, group->count)`:

```c
p     = min((anim_num * frame_count) / anim_div, frame_count - 1);  // signed idiv
frame = (uint16_t)(frame_base - p + frame_count - 1);
sprite= ar_sprite_slot_frame(pool[bank_id], frame);                 // = FUN_00418470 inlined
idx   = (alpha_level * 20) / 1000;                                  // → ramp index
idx   = clamp(idx, 0, 19);          // <0 → ramp[0]; >=20 → [0x8a9304] == &ramp[19]
desc  = ramp[idx];                  // ramp base 0x8a92b8 (20 blend-desc PTRS)
dst_x = sprite->metric_0c + x_num / 100;
dst_y = sprite->metric_10 + y_num / 100;
zdd_blit_orchestrate(desc /*=ecx*/, dest, sprite, dst_x, dst_y,
                     sprite->metric_14 /*w*/, sprite->metric_18 /*h*/,
                     0, 0, sprite->colorkey_out, DAT_008a6ec0 /*gdi_ctx, always NULL*/);
```

Notes:
- `0x51eb851f / sar 5 / shr+add` = signed ÷100; `0x10624dd3 / sar 6 / …` =
  signed ÷1000 — both truncate toward zero, exactly C integer division.
- **The ramps** `0x8a92b8` (compositor + cursor wrapper 0x56c470) and
  `0x8a9308` (level wrapper 0x56c4e0 + the logo via `title_fade_ramp`) are
  arrays of **blend-descriptor pointers**, indexed by the fade level, filled
  at runtime. In the static image they are all-zero (= the existing
  `title_fade_ramp` NULL behaviour): a NULL desc makes the alpha blit a
  plain copy / no-blend. `0x8a9304` is literally `&ramp_0x8a92b8[19]` — the
  ">=20" clamp and "last entry" are the same dword.
- A NULL resolved sprite (unloaded bank) crashes retail; the port skips the
  entry (headless safety, same class as the getter's NULL return).

## The sprite wrappers — `0x56c610 / _4e0 / _470 / _580`

Thin per-sprite forwards (the title render half's per-phase draws). All read
the primary surface from `DAT_008a93cc->[0x16c]` and resolve `desc` from a
fade ramp:

- **0x56c610** (plain `TITLE_DRAW_SPRITE`): straight to `zdd_object_blt_keyed`
  (0x5b9b70) — no alpha, just a color-keyed Blt.
- **0x56c4e0** (`TITLE_DRAW_SPRITE_LEVEL`): `idx=(level*20)/level5...`; ramp
  `0x8a9308[idx]`. If the ramp entry is **zero** → plain `0x5b9b70` (ramp 0 =
  full opaque); else alpha via `0x5bd550` with that descriptor.
- **0x56c470** (`TITLE_DRAW_MENU_CURSOR`): ramp `0x8a92b8[idx]`, always alpha
  via `0x5bd550`.
- **0x56c580** (`TITLE_DRAW_SPARKLE`): the `arg` gate at +0x24 picks between
  `0x5b9bf0` (a blt_keyed sibling, 256 B, not yet ported) and `0x5bd550`.

These are the remaining `TITLE_DRAW_*` arms of the render sink. They want a
small render-bridge module that can include both `asset_register.h`
(the slot/getter) and `zdd.h` (the blit + the primary surface).

## What's left (next chips, in dependency order)

1. ~~**The compositor `0x56c180`**~~ **DONE ckpt 17** (`title_compositor_draw`
   in `src/title_render.{c,h}`).  The render-bridge module now exists — the
   wrappers `0x56c610/_4e0/_470/_580` are its next residents.
2. **The sprite-sheet decoder `FUN_004184a0` + slicer `FUN_004188b0`** — the
   `ar_sprite_decode_hook` target. Reads the "DATA" PE resource (via the
   resource-stream readers `0x5b7800` + the trivial field getters
   `0x5b6eb0/_ee0/_ef0/_f00`, `0x5ba390`), decodes 24bpp pixels, applies the
   per-channel brightness LUT (`slot+0x10/0x14/0x18` × pixel ÷100), honours
   the `0xff00ff` magenta color-key, and slices the sheet into
   `frame_count` per-frame surfaces (`FUN_004188b0` divides the sheet dims by
   the frame grid). This is the genuine pixel source — nothing renders
   without it. It needs the sprite-sheet binary format pinned first.
3. **The render sink + drive from main.c** — wire `title_render_sink_hook`
   over (1)+(2)+the wrappers, drive `title_scene_step`, capture port frames,
   diff vs the harness goldens.
