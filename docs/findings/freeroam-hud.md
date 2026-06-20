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
