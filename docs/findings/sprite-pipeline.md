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

**Ported ckpt 18 (except the sparkle 0x56c580):** `title_draw_sprite` /
`title_draw_sprite_level` / `title_draw_menu_cursor` in `src/title_render.c`.
They take `dest` (the primary surface) as a parameter instead of re-deriving
it from `DAT_008a93cc->[0x16c]` in retail (same surface — the caller already
passes it as the keyed paths' `a1`), and the ramp as a `const zdd_blend_desc
*const *` (NULL ⇒ keyed/plain).  `x`/`y` are plain offsets (no ÷100); the
keyed path lets `zdd_object_blt_keyed` add metric_0c/_10, the alpha path adds
them explicitly.  Provenance + arg shapes (`ret 0x10/0x18`, the dead first
arg of the plain wrapper, the level wrapper's idx>=20 → plain divert vs the
cursor's [0,19] clamp, the cursor's unguarded idiv) are in title_render.h.
NB these three VAs are NOT in `functions.csv`, so the ledger lists them as
sub-helper labels — real ports, but the headline count doesn't move.

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
- **0x56c580** (`TITLE_DRAW_SPARKLE`, PORTED ckpt 19 → `title_draw_sparkle`):
  gated directly on a caller-supplied descriptor (not a ramp lookup).
  desc != NULL → `0x5bd550` alpha with an **explicit src sub-rect**
  (src_x/src_y + w/h, unlike the other wrappers' 0,0 / metric_14·18);
  desc == NULL → `zdd_object_blt_clipped` (`0x5b9bf0`), a color-keyed Blt
  that clips the src rect against the sprite's metric_0c/_10 origin +
  metric_14/_18 extent.  The alpha path metric-offsets the dest; the
  clipped path passes the raw dest (clip applied inside).

These are the remaining `TITLE_DRAW_*` arms of the render sink. They want a
small render-bridge module that can include both `asset_register.h`
(the slot/getter) and `zdd.h` (the blit + the primary surface).

## The sprite-sheet decoder — `FUN_004184a0` + slicer `FUN_004188b0` (PORTED, ckpt 20)

Ported as **`ar_sprite_decode`** (the `ar_sprite_decode_hook` target) +
**`ar_sprite_slice`** + the pure transform **`ar_sheet_decode_pixels`** in
`src/asset_register.c`.  The big realisation that shrank this chip: the whole
**resource-load + DIB/decompress layer is already ported** as
`bitmap_session` (`bs_decode_resource` = `FUN_005b7800`,
`bs_parse_compressed_header` = `FUN_005b7c10`, etc.).  So the decoder is just:

```
ar_sprite_decode(slot):                       # entry 0 (getter's only caller)
  free any previously-decoded frames          # re-decode cleanup 0x4184ce
  bs_decode_resource(settings, resource_id, "DATA", compressed=1)   # ported
  if (slot->f_08 && bpp == 24):
      ar_sheet_decode_pixels(...)             # the 24bpp brightness/key pass
  ar_sprite_slice(slot, 0, sheet, slot->width, slot->height, slot->colorkey)
  bs_release(sheet)
```

**The brightness pass** (`ar_sheet_decode_pixels`) is the genuine new pixel
logic and is fully host-tested: per-channel `ch = ch * scale / 1000` with an
optional gamma LUT (`slot->f_18`), magenta `0xff00ff` left untouched, and the
**reversed byte→field mapping** (byte0·f_14, byte1·f_10, byte2·f_0c) — see
engine-quirks **#46**.  **The slicer** (`ar_sprite_slice`) computes the frame
grid (`cols = sheet_w/cell_w`, `rows = sheet_h/cell_h`, `count = cols*rows`),
stamps `slot->f_38 = count`, allocates `entries[0].frames`, and fills it via
the per-cell surface-builder hook.  The colour-key it hands the builder is
magenta unless the `0x1ffffff` sentinel — engine-quirks **#47**.

**Still behind hooks (the DDraw leaf layer):** the per-cell surface builder
`0x5b9280` (`ar_frame_build_hook` — turns one cell + the sheet pixels into a
real keyed `zdd_object*`), the surface release `FUN_005b9390`
(`ar_frame_free_hook` = the already-ported `zdd_object_dtor`), the trim-metadata
builder `0x5b6f80` (**ported ckpt 23** = `bs_trim_opaque_rect`), the
god-object display-depth format switch (`[zdd+0x168]` →
`0x5b7310/_74f0/_7270`), and the 8bpp indexed-palette apply `0x5b7bd0`.
Headless (no build hook) sizes + zero-fills the frames array but leaves the
surfaces NULL — exactly the frame getter's "still undecoded" path.

### 8d call graph (decoded ckpt 23 — r2, ready to wire in the live session)

```
0x5b9280  ar_frame_build  (this=parent ZDD; 9 stack args; ret 0x28)
  ├─ operator_new(0xd8) + zdd_object_ctor(obj, parent)   [both ported]
  ├─ FUN_005b9630(obj, args…)                            [orchestrator, below]
  └─ on fail: zdd_object_dtor(obj) + free(obj); else *out = obj, ret 1
        — structurally identical to the ported zdd_object_new, but the
          orchestrator is 0x5b9630 (not 0x5b95c0).

0x5b9630  (this=new ZDDObject; ret 0x28)  the per-cell surface orchestrator
  args (C order): (p_1ch, p_24h, p_28h, p_2ch, colorkey_34h, p_38h,
                   p_3ch=trim-count, trim_30h=ptr to bs_trim_rect)
  ├─ if (trim_30h && p_3ch >= 1):                         [trim present]
  │     load trim[0x14]→A, trim[0x10]→B
  │     if (p_3ch >= 2): width  = max(0, trim[4]-trim[0]+1)   // x_right-x_left+1
  │                      height = max(0, trim[0xc]-trim[8]+1) // y_bottom-y_top+1
  │                      src_x  = trim[0] (x_left), src_y = trim[8] (y_top)
  │     if (A==0): colorkey = 0x1ffffff   (fully-opaque → no key; cf. found_key)
  │     if (B==0): goto metrics_only      (fully-transparent → no surface)
  ├─ zdd_object_create_surface_pair(self, p1=p_1ch, p2=p_24h, p3=0,
  │       p4=colorkey, p5=p_38h, p6=src_x, p7=src_y, w=width, h=height)  [ported]
  │     on 0 → ret 0 (build failed)
  ├─ FUN_005b9910(self, srcbitmap=var_1ch, 0, 0, width, height,
  │       src_x+var_20h, src_y+var_24h)   [the pixel writer, below] on 0 → ret 0
  └─ ret 1
  metrics_only: zdd_object_prefill_desc(self,0,0); zdd_object_stamp_metrics(
        self, p_28h, p_2ch, 0,0,0,0); ret 1   [both ported]

0x5b9910  (this=dest ZDDObject; 7 args)  copy the cell pixels into the surface
  ├─ reads src bitmap: 0x5b6ec0 bottom-row, 0x5b6f00 depth/8=bytes, 0x5b6ef0 stride
  ├─ FUN_005b9490(dest)  — Lock the dest surface (DDraw) → dest->[0]/[4]/[8]
  ├─ clamp the copy rect to the locked extent
  ├─ rep stosd/stosb — zero-fill the dest surface
  └─ row-copy loop (the per-format pixel write; the 0x5b7310/_74f0/_7270 switch
        on [zdd+0x168] display depth selects the converter, each LocalAlloc'ing
        a scratch line buffer + 0x5b7bd0 applying the 8bpp palette)
  ⇒ DDraw-bound (needs a live locked surface) — the live-session chip.
```

So of 8d, the **pure-logic pieces are now ported** (`bs_trim_opaque_rect`,
`zdd_object_create_surface_pair`, `_ctor`/`_dtor`/`_prefill_desc`/
`_stamp_metrics`, `zdd_object_new`'s shape).  What remains is **DDraw-bound and
interdependent**: `0x5b9490` (Lock), `0x5b9910` (clip+zero+copy), and the
`0x5b7310/_74f0/_7270` + `0x5b7bd0` format converters — best ported + verified
together in the live session, where a registered bank + the real display depth
let the produced pixels be diffed against the harness goldens.

## What's left (next chips, in dependency order)

1. ~~compositor / wrappers / blt layer~~ **DONE ckpt 17–19.**
2. ~~sprite-sheet decoder `FUN_004184a0` + slicer `FUN_004188b0`~~ **DONE
   ckpt 20** (`ar_sprite_decode` / `ar_sprite_slice` / `ar_sheet_decode_pixels`).
3. **The per-cell DDraw surface builder `0x5b9280`** — call graph decoded
   above (ckpt 23).  `0x5b6f80` trim metadata = **DONE ckpt 23**
   (`bs_trim_opaque_rect`); `0x5b9390` release = the ported `zdd_object_dtor`;
   `0x5b95c0` create_surface_pair = ported.  **Remaining (the live-session
   chip):** the orchestrator `0x5b9630`, the pixel writer `0x5b9910`
   (+ `0x5b9490` Lock), and the `0x5b7310/_74f0/_7270` + `0x5b7bd0` format
   converters — wire `ar_frame_build_hook`/`ar_frame_free_hook` to real keyed
   surfaces.  Needs the DDraw god object; live-verified against goldens.
4. **The render sink + drive from main.c** — wire `title_render_sink_hook`
   over the compositor/wrappers + the now-real frame surfaces, drive
   `title_scene_step`, capture port frames, diff vs the harness goldens.
