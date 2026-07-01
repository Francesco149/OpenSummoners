# The freeroam status HUD — RE + port scoping (ckpt 152)

The `res=0` freeroam status HUD (USER `osr_notes.jsonl` #7-9 on `port-errands.osr`,
errands tick 2413): the top-left leader panel (portrait + HP/MP bars + numbers +
level + element stars), the bottom-left/right strips, the bottom-right item bar, and
the door indicator.  This is the full drawcall ground truth (off `retail.osr`) + the
render architecture (off the decompile) + the port plan.  See also
`errands-render-gaps.md` §2 (the USER note crops/ticks).

## 1. The drawcall ground truth — retail.osr tick 2413 (the HUD overlay layer)

The HUD draws AFTER the world (seq 0-461 = the room backdrop/tiles); the HUD is the
overlay layer, **seq 462-536** (`tools/trace_studio2/hud_probe.py retail.osr 2413`).
All HUD source blits are `res=0` (system/UI banks, resource_id 0) + GDI TextOutA:

**Top-left leader panel** (panel x-base = 1 when fully slid in):
- seq 462-469 — **HP bar**: 4× `rects` strips `(118,7,168,2)`,`(117,9,..)`,`(116,11,..)`,`(115,13,..)`
  (x −1/row = the italic skew; w=168=`0xa8`; src y 0) + width-0 `alpha` companions (HP full).
- seq 470-475 — **MP bar**: 3× `rects` strips `(118,21,168,2)`,`(117,23)`,`(116,25)` (src y 2).
- seq 478 — **panel frame**: `keyed res=0 (-31,1,286,90)` (the ornate border, drawn over the bars).
- seq 481-494 — **HP text** `' 100 / 100'`: dark `0x202020` outline (x 217-220 × y 4-6) + white
  `0xffffff` center (218-219, 5); font 2; bk=1 (transparent).  Format `"%s / %d"` (cur formatted, max).
- seq 495-508 — **MP text** `'  20 / 20'`: same outline pattern, base (218,18).
- seq 509 — **level glyph**: `keyed res=0 (161,25,8,14)` (the "Lv 1" digit; a UI-bank cel, NOT text).
- seq 510 — **leader portrait**: `keyed res=0 (1,1,82,89)` (Arche's face).

**Bottom-left** (seq 513-516): small `keyed res=0` glyphs `(40,456,13,11)`,`(49,456,10,14)`,
`(112,456,13,11)`,`(121,456,8,14)` — two small numeric/icon clusters (gold / time, TBD).
**Bottom-right item bar** (seq 518-535): **6 slots** at x=440,472,504,536,568,600 (step 32), y=444;
each slot = a 32×32 frame + an item icon (~24-30²) + a 14×10 quantity glyph (3 blits/slot).
**Door indicator** (seq 536): `keyed res=0 (200,415,24,42)` (the exit-door arrow).

## 2. The render architecture (off the decompile)

- **`FUN_00494e60` (0x494e60, 3904 B) = the HUD render orchestrator.**  Called **×2** from the
  in-game render driver `FUN_0048c150:165` (`do { FUN_00494e60(paint_ctx); } while(--i)`), AFTER the
  world layers + fade grid.  Its `this` (ECX) = the scene/room object (`DAT_008a9b50`-rooted); the
  party slots are at **`room+0x4030`** (8 slots, stride 4 → each a `*char` ptr; the char data is
  `*(slot+0x9f4)+0x750`).  It reads a HUD context with fields incl. `+0x10` (slide phase 0..3),
  `+0x1b0` (slide progress 0..1000; x-base `= ((prog*0xb−11000)*0x20)/1000+1` → 1 when in),
  `+0x1b4` (leader id), `+0x1b8`/`+0x1bc` (HP/MP ratios), `+0x340`/`+0x20` (per-member sub-structs),
  `+0x388`/`+0x398`/`+0x3c8`/`+0x3d8` (item/element sub-objects).
- **Sub-renderers** (all called by 0x494e60):
  | VA | role | draw |
  |----|------|------|
  | `0x498680` | HP bar (401 B) | `rects` filled + `alpha` depleted, N rows, x-skew, samples pool `0x2f` gradient (src y = color) |
  | `0x498820` | MP bar | same, 3 rows |
  | `0x498f10` | small gauge/icon bar | `rects`/`alpha` |
  | `0x498620` | element star | keyed blit, per-star |
  | `0x43e250` | **outlined text** (334 B) | dark `0x202020` 3×3 (or 4×4) TextOutA outline + white center; font `DAT_008a9274[idx]` |
  | `0x495e40` | a text/number draw | TextOutA |
  | `0x497f40` | per-member element row | bars + keyed |
  | `0x498960` | per-member | |
  | `0x496560` | multi-party portrait row (party>1) | |
  | `0x497c20` | bottom-left strip | |
  | `0x4963a0` | bottom strip | |
  | `0x4962a0` | **item slot** (×6) — the bottom-right item bar | frame+icon+qty |
  | `0x4975e0` | bottom-right cluster | |
  | `0x496ec0` | bottom strip (gated `DAT_008a6e80+0xcc==0`) | |
  | `0x4969b0` | **door indicator** | keyed blit |
  | `0x495fe0` | screen overlay/fade (×2, conditional) | alpha |
- **Sprite blit mechanism**: `FUN_00418470(N)` returns frame N's descriptor of the current UI bank
  (the decompiler hides the `mov ecx,eax`); `FUN_005b9b70(dst,x,y)` blits THAT descriptor (reads
  `this+0x2c` surface, `+0xc/+0x10` offset, `+0xb8/+0xbc` size, `+0xd4` blend).  The port already
  models this (`ar_pool_get_slot` + `ar_sprite_slot_frame`).
- **The animator** `FUN_0049af40` (the HP/MP-ratio + portrait-fade lerper, retail_fields
  `hud_party_anim_update`) runs 2×/sim-tick from `0x499ab0:180` — it produces the `+0x1b8`/`+0x1bc`
  ratios 0x494e60 renders.  (Not the renderer.)

## 3. Dependencies (why this is multi-checkpoint, not a one-liner)

1. **Source sheets** — every visual element (bars' gradient pool `0x2f`, frame, portrait, star,
   level glyph, item icons, door arrow) reads a `res=0` UI bank.  **Recoverable** from retail.osr:
   the capture grabs source pixels for ALL blits incl. res=0 (`engine_hooks.h sheet_capture_source`,
   keyed by dhash; UI/panel sheets re-grabbed).  So extract each HUD source sheet by dhash from the
   `.osr` SHEET stream → port asset (the captured-asset pattern, like the fire ramp / npc palette).
   OPEN: whether these banks are instead loadable from the user's own files via the existing
   asset_register (find their real pool indices / the HUD init that registers them) — preferred if
   cheap, since it avoids shipping captured pixels.
2. **HUD context / party data** — the values (HP 100/100, MP 20/20, Lv 1, 2 element stars, the 6
   item-bar contents, the leader) come from the party subsystem (`room+0x4030`), which is **unported**
   (the port's freeroam Arche is a standalone `character` mover, not a party slot).  Faithful path:
   a captured **`PORT-DEBT(hud-party-context)`** stand-in with the errands values + a faithful render,
   retired when the party subsystem lands (the cutscene-coords / ERRANDS_CAST precedent).
3. **Render hook** — call the HUD render after the world/effect render in `main.c`, gated on the
   freeroam/errands room (`g_freeroam_active && !room_is_town`), emitting through the port's blit
   primitives so it shows in the port `.osr`.

## 4. Port plan (incremental, verify each slice bit-exact vs retail.osr)

1. **Top-left panel** — HP/MP bars (`0x498680`/`0x498820`) + numbers (`0x43e250`) + frame + portrait
   + level glyph + element stars.  The coherent first visual unit.
2. **Item bar** (bottom-right, `0x4962a0` ×6) + the bottom-left strip (`0x497c20`).
3. **Door indicator** (`0x4969b0`).
4. Retire `PORT-DEBT(hud-party-context)` when the party subsystem is ported.

Verify: `draw_probe`/`hud_probe` (the rects/keyed positions), the TEXT records (the numbers),
`osr_prof` recon (`differ_px==0` per region).  `tools/trace_studio2/hud_probe.py` is the ground-truth
probe (dumps the HUD layer at any tick).

## 5. Slice 1c-1 (ckpt 169) — element STARS bit-exact; LEVEL blocked on the ramp-palette gap

Ground truth re-probed off `sword2.osr` tick 2200 (`hud_probe` + a dhash→SHEET resolve),
the top-left leader panel's data glyphs (seq after the frame at 495):

| seq | element | res | frame | dst | dhash |
|----|----|----|----|----|----|
| 493-494 | EXP gauge | 1102 (0x44e) | 0 | (144,42) w104 | 0x3a65dc81 |
| 496-497 | element stars ×2 | 1103 (0x44f) | 16 | (187,30),(200,30) 12×9 | 0xaedb8faa |
| 526 | level '1' | res0 (ramp0=0x413) | 16 | (161,25,8,14) | 0x192317ef |
| 527 | portrait | res0 | — | (1,1,82,89) | 0xbbf24c22 |
| 528 | portrait sub-blit | 1909 (0x775) | 0 | (92,29,38,16) | 0x72a588ef |

Bank resolution (all "free" — registered by `ar_register_palette_ramps`): stars =
`ar_pool_get_slot(0x31)` = `g_ar_sprite_slots[36]` = res 0x44f (the icon sheet, plain
getter `FUN_004184a0(0)` in `0x498620`); level = `ar_pool_get_slot(1)` =
`g_ar_sprite_ramp_slots[0]` = res 0x413 (the small-font ramp, plain getter in `0x495e40`).

**STARS — PORTED + VERIFIED bit-exact.**  `game_render_hud` blits res 0x44f frame 16 keyed
at `(xbase+0xba+k·0xd, ybase+0x1d)` for `k=0..1` (Arche's 2 affinity stars, PORT-DEBT(hud-
party-context) stand-in count/element).  Added slot 36 to the 8bpp grade skip-list (plain-
getter UI sheet, NOT graded — same class as the bars/frame).  Verified off `port-hud1c.osr`
vs `sword2.osr` tick 2200: both stars res 1103 fr16 dst (187,30)/(200,30) 12×9 **dhash
0xaedb8faa — byte-identical** to the recording.  Geometry host-tested (`hud_star_level_positions`).

**LEVEL — PORTED + VERIFIED bit-exact (slice 1c-2, ckpt 170).**  The geometry is exact (glyph
`c-0x21`, +9px advance, base (161,25) — `hud_glyph_frame` + `HUD_LEVEL_*`, host-tested); the
bank is ramp0/res 0x413 frame 16 ('1').  `game_render_hud` blits the value string left-to-right
via `hud_glyph_frame` from `ar_pool_get_slot(HUD_LEVEL_POOL_IDX=1)`.  Verified off
`port-hud1c2.osr` vs `sword2.osr`: res 1043 fr16 dst (161,25,8,14) **dhash 0x192317ef —
byte-identical** to the GT (was 0x14573bd0 too-dark); the digit is the ONLY ramp draw across the
whole title→newgame→cutscene→errands run (3956 draws, all 0x192317ef), so no other ramp consumer
regressed; bars/frame/stars dhashes unchanged.

### The ramp custom-palette gap — RESOLVED (the LEVEL blocker, slice 1c-2)

The digit rendered **one grade-step too dark** (dhash 0x14573bd0 vs GT 0x192317ef; the '1'
outline read 0x303030 vs retail 0x404040) because the port sliced res 0x413 against its raw
EMBEDDED palette (entry 1 = 0x333333) instead of the installed custom ramp palette
(`entries[0].b` = 0x404040 that `ar_run_palette_ramp` built).

**Root RE'd (the exact retail bind, not curve-fit):** `FUN_004184a0` (the frame-getter decode =
the port's `ar_sprite_decode`) does, right after decode and BEFORE the slice (`:70-73`):
`if (entries[frame].b != 0 && session 8bpp) FUN_005b7bd0(entries[frame].b)`.  **`FUN_005b7bd0`**
overwrites the session's bmiColors (session+0x34, RGBQUAD B,G,R,0) from the installed
PALETTEENTRY buffer (R,G,B,_) — the exact inverse channel-swap of `bs_emit_palette_bgra`.  So a
slot that had `ar_palette_install` run (the 12 ramps + the title seed slot 0) slices against its
INSTALLED palette.  The port omitted this bind → ramp glyphs used the embedded palette.

**Fix (ckpt 170):** `bs_install_palette` (FUN_005b7bd0, `bitmap_session.c`) + a call in
`ar_sprite_decode` right after decode-success, gated exactly as retail (`entries[0].b != NULL &&
8bpp`).  Plain sprite slots never `ar_palette_install` (entries[0].b == NULL) so it's a no-op for
them.  ALSO: the ramp banks are plain-getter (no 0x417c40 grade descriptor) so retail does NOT
grade them — added the `g_ar_sprite_ramp_slots` range to the `title_sheet_format` grade-skip (else
the bound 0x404040 grades back to 0x333333, undoing the bind).  **No regression:** the title seed
(slot 0) is never drawn as a sprite (verified: res 0x90b absent from the run), and res 0x413 is the
only ramp drawn (bit-exact).  `PORT-DEBT(hud-ramp-palette)` RETIRED.  Host: `bs_install_palette`
×2 (swap + roundtrip-vs-emit).

## 6. Slice 1c-2 remainder — RE'd + scoped (ckpt 170), ready to port

### The EXP gauge (`FUN_00498f10`) — FULLY RE'd; only the blend-desc sourcing + a capture remain

Call site `0x494e60:95`: `FUN_00498f10(ctx, xbase+0x8f, ybase+0x29, 1, 4, cur=0, char+0xe8,
char+0xec /*max*/, 0x68 /*w=104*/, 0x2e /*pool idx → g_ar_sprite_slots[33] = res 0x44e*/, 0, 1)`.
Position **(144,42)** confirmed.  Two spans (ground truth `sword2.osr` tick 2200):
- **seq493 FILLED** — `blt_rects` (mode 2) dst `(144,42,0,2)` src `(104,4)` — **width 0** (Arche's
  errands EXP = 0: `cur=0` and `char+0xe8=0`; there's no combat in the errands, so it stays 0).
  A 0-width no-op → OMIT it, like the HP/MP bars omit their 0-width depleted (ckpt 167).
- **seq494 DEPLETED** — `blt_alpha` (mode 4, `FUN_005bd550`) dst `(144,42,104,2)` src `(0,14)`
  **blend_ref=9** — the full empty gradient.  `498f10`'s display-mode branch
  (`*DAT_008a6e80+0x94==2` → `0x5bd550` alpha, else `0x5b9ae0` rects) takes the ALPHA path here.
  The port's `zdd_blit_orchestrate` (0x5bd550) simple path (gdi_ctx=`DAT_008a6ec0`=NULL, quirk #45)
  → `zdd_alpha_blit(desc, primary, exp_frame, 144,42, 104,2, 0,14, ckey)`.

**The blend desc:** retail passes `*(frame+0x28)` (the res 0x44e frame-0 surface's attached blend)
= **blend_ref 9 = LUT md5 `ed6214bd`** — and the PORT ALREADY REGISTERS THIS EXACT DESCRIPTOR at
its own blend_ref 9 (port osr ref 1..9 LUTs == retail's; `oe_blend_register` content-dedup order
matches).  It's a common mid-alpha ramp (the fire/title/fades emit it).  **The only open mechanical
step:** source it in `game_render_hud` — either (a) match `ed6214bd` against the `g_pd_boot_group_a/b[]`
descriptors (compute each LUT via `oe_blend_lut_len`; the trail anchor is `g_ramp_a[19]`, the fire
`g_ramp_a[14]`), or (b) replicate retail's `*(frame+0x28)` (attach the blend to the res 0x44e frame at
register time).  **Also:** add res 0x44e (`g_ar_sprite_slots[33]`) to the `title_sheet_format` grade-skip
(plain-getter bar sheet, same class as the HP/MP bars idx 34).  Then verify res 0x44e fr0 dst
(144,42,104,2) dhash **0x3a65dc81** + the alpha blend.

### The 82×89 face PORTRAIT (`FUN_00494e60:125-164`) — a bank hunt (Frida)

A per-member descriptor at `char+0x50`: head-state `hud_ctx+0x1c8` selects the frame (`==2`→`+8`,
`==3`→`+10`, else→`+6`); main blit at **(1,1)** 82×89, then a sub-blit at **(92,29)** frame `+0x14`
(= res 0x775, registered idx 56).  Bank = `pool[*(char+0x50 +4)]`.  The main face is **res=0** in the
capture, **dhash 0xbbf24c22** — a dedicated small-face bank, NOT the res-1000 dialogue bust.  NEEDS
Frida (host up, ckpt-153 res-probe pattern): drive retail to the errands, hook `0x494e60` (or
`0x418470` filtered by caller) at the portrait draw, read the bank ECX's `+0x3c` (HMODULE) / `+0x40`
(PE res id), then register that bank in the port + blit it.  `PORT-DEBT(hud-party-context)`: the
descriptor (bank + head-state frames) is Arche's leader stand-in until the party subsystem lands.
