# The errands (freeroam) render gaps — the USER studio notes, RE'd

The USER's `osr_notes.jsonl` (on `port-errands.osr`) flags the remaining errands gaps.
This is the RE of each (off `retail.osr` via `osr_prof` recon + `draw_probe`), so the
ports are clean follow-ups.  The exact USER marks (tick + crop + text):

| tick | crop [x,y,w,h]      | USER note                          |
|------|---------------------|------------------------------------|
| 1726 | [309,154,96,89]     | **missing fire in fireplace**      |
| 1726 | [75,283,30,38]      | **wrong wall colour**              |
| 1726 | [315,343,143,85]    | **missing props in shelves**       |
| 1726 | [81,290,102,129]    | **bookshelf missing props**        |
| 2413 | [0,0,294,92]        | **top left hud**                   |
| 2413 | [0,443,140,37]      | **bottom left hud**                |
| 2413 | [430,433,210,47]    | **bottom right hud**               |
| 2413 | [192,402,49,67]     | door indicator                     |
| 2413 | [176,348,65,74]     | arche idle animation (when still)  |
| 1579-1604 | [369,343,48,71] | arche turns the other way (→ DONE ckpt 146, `findings/arche-house-turn.md`) |

The errands camera is STATIC (no follow) in the opening — screen positions below are
stable across the opening ticks.

## 1. The fireplace FIRE (note #3) — PORTED + DRAWCALL-EXACT (ckpt 147, `actor_spawn.c`)

The fireplace is at screen ~(309-405, 154-243) — the recess behind the grate at the
base of the brick chimney (upper-center).  PORT showed a BLACK empty recess; RETAIL has
a roaring orange fire.  Now PORTED as an additive-alpha `ERRANDS_CAST` member; verified
off a fresh `port-fire.osr` vs `retail.osr`.

**The fire is `res=1034`** (PE DATA resource 1034), drawn:
- dst **(329,178), 48x39**, CONSTANT position/size.
- **ALPHA-blended**: primitive `alpha`, `st=0x8000` (KEYSRC armed), **`bmode=1`**, ckey
  `0xf81f` — the additive flame glow, NOT the colorkey blit the furniture uses.
- **frames 0,1,2,3,4,5 LOOPING, exactly 6 sim-ticks/frame** (a 36-tick loop).  This is the
  retail clip's SINGLE uniform `frame_dur` (the 0x154-byte clip format carries ONE duration
  for all frames, +0x44 — `anim_clip.h`; stepper `0x54f980:174`).  Read off the clean
  (non-coalesced) retail ticks: fr0 1700-1705, fr1 1726-1731, fr4 1744-1749 = 6t each (the
  1736-1739 "gap" is retail flip-coalescing, quirk #99 — NOT a short frame).  The PORT
  renders every tick → all six cels held exactly 6t, confirming the data duration.
- **The additive blend = `ramp_a[14]`** (weight 750, mode 1).  PROVEN bit-exact: the fire's
  blend descriptor extracted from retail.osr (CONSTANT `blend_ref=36` across all 6 frames +
  every tick; RGB565 shift (11,6,0)/mask (0xf800,0x7e0,0x1f); 3×1024-byte LUTs) is
  BYTE-IDENTICAL to the port's `g_pd_boot_group_a[14]` LUTs — a host compare of all 20
  ramp_a entries found exactly one full match at index 14 (`/tmp/fire_match` harness).
  (This also independently confirms the port's pixel_drawer ramp_a == retail's, bit-exact.)

**THE BANK (RE correction).**  res 0x40a is registered at boot (group 3) as g_ar_sprite_slots
slot **406** (`asset_register.c:2389 {406,0x040a,0x40,0x40,0x0,1,2}`, 64x64; decoded lazily
on first render — the findings' earlier "not registered/loaded" was WRONG, the real gap was
no SPAWN).  But the actor's `bank` field is the `ar_pool_get_slot` POOL index, not the slot
index: `slot = g_ar_sprite_slots[pool_idx - (AR_SPRITE_RAMP_COUNT+1)]`, RAMP_COUNT=12 ⇒
offset 13.  So the fire's bank = **0x1a3 (419)** = slot 406 + 13 (cross-checked: the furniture
bank 0x16f=367 → slot 354 = res 0x3ff=1023 ✓).  (First attempt used 0x196=406 the SLOT index
→ resolved the wrong/empty slot → 0 blits; corrected to 0x1a3.)

**THE ALPHA WIRING.**  `room_cast_member` gained an `alpha` field → `actor.node_alpha`
(+0xf4); `actor_emit_part` already routes node_alpha≠0 → mode-1 node (`bmode=1`) →
`map_present` PRESENT_ALPHA → `game_present_blit` resolves `g_ramp_a[param8 & 0xff]`.  So
`alpha = 14` selects ramp_a[14].  (The retail emit's additive path is the EFFECT
behaviour-code arm in `0x491ae0`, NOT the clip's +0xa8 — a prior agent misread
`psVar12[0xa8]`: psVar12 is a `short*`, so [0xa8] is byte +0x150 = the clip `link`/dir-override
field, not an alpha byte.)

**THE SPAWN (PORT-DEBT(errands-cast) stand-in).**  The proper spawn is an un-RE'd map EFFECT
(the lazy def-table fill `0x41e600`, in binary DATA 1025).  Captured here as an additive
room-cast member at world (32900,33800) [= the errands projection of screen (329,178), cam
0/16000] + `dst_base (-9,-18)` (the 64x64 cel's pivot, fitted off port-fire.osr).  +host test
`errands_fire`.

**RESIDUAL — the res-1034 SHEET decodes slightly differently (a decode-fidelity
follow-up, NOT the spawn/blend).**  At a SETTLED, fire-frame-MATCHED recon (port fr4 tick
2403 vs retail fr4 tick 2430) the backdrop AROUND the fire is `differ_px==0` and ALL the
residual is inside the 48x39 fire rect (~1046px, maxd 66 sumRGB).  The decoded fire sheet's
**dhash DIFFERS port↔retail** (fr4 0x52fc66d1 vs 0xfa994282) while neighbour sheets in the
same region MATCH cross-side (res 1722 fr0-7 all identical dhash) — so the dhash IS
comparable and res 1034 genuinely decodes to different pixels in the port.  The additive
blend (×750) amplifies the small per-pixel sheet delta.  Likely a palette / colour-grade /
flag decode subtlety (cf. the NPC colour-variant ckpt 142, and note #4's wall-tint HUE
SHIFT — possibly the SAME per-sheet grade); bank 406 carries an info-event `FLAG_SET 0x2`
(`asset_register.c:2593`) + scale_flag 1 — candidates to investigate.  TODO next session:
hook the port vs retail decode of res 1034 (the sheet bytes) to find the divergence.
(My first recon wrongly read this as a "fireplace base gap" — that was a recon artifact:
port tick 1695 was mid reveal-FADE + the fire was at a different anim frame; the draw lists
are in fact IDENTICAL.)  Feed: the settled matched-fr4 PORT|RETAIL|DIFF montage.

## 2. The freeroam HUD (notes #13-18) — IDENTIFIED, not ported

The `res=0` draws present in retail's errands but ABSENT in the port are the **freeroam
status HUD** (the gameplay UI), NOT particles:
- **top-left** [0,0,294,92]: the leader portrait + **HP "100 / 100"** + **MP "20 / 20"** +
  **level "1"** + **★★** (the two element stars) + the bar.
- **bottom-left** [0,443,140,37] + **bottom-right** [430,433,210,47]: the lower HUD strips.
- The HUD numbers/glyphs render as `res=0` primitive draws (the small clustered draws that
  "move" frame-to-frame in `draw_probe` are the changing digits/glyphs, not motion).

This is the in-game status panel — a real UI subsystem to port (the party/character stats
exist port-side; the panel LAYOUT + bar/star/number rendering is the work).  Best done
with the USER (the HUD layout is very visual).

## 3. The wall colour (note #11) — a HUE SHIFT (color-grade / palette), deferred

[75,283,30,38] "wrong wall colour".  Recon (osr_prof, tick 1780 vs 1726) shows it is a
clear **hue shift on the wood-plank WALL**: the PORT wall is WARM BROWN, retail's is a
cooler GREENISH-GRAY — while the props (the plant/vase), the sign, and the shelves render
identically.  So it is NOT a tile-frame or sprite difference; it is a **palette / scene
color-grade** difference applied to the wall tiles (the errands scene's grade, or the wall
bank's decoded palette).  Investigate `color_grade.c` (the per-scene grade) + the wall
tile bank's palette decode.  DEFERRED — a color change is very visual + wants USER
confirmation, and it is a subsystem (grade/palette) not a quick tile fix.  Feed: the
PORT(brown)|RETAIL(grey) wall montage.

## 4. The shelf props (notes "missing props in shelves" / "bookshelf missing props") — FIXED (z-order, ckpt 146, `ead9c49`)

[315,343,143,85] + [81,290,102,129] — these were **NOT a missing spawn**.  The port
ALREADY emits the shelf props (the structure band, `res=1026`/`res=1027`, at the exact
retail frames + positions — verified via `draw_probe`).  The bug was **Z-ORDER**: the
ckpt-145 ERRANDS_CAST furniture stand-in (the bookshelf FRAME `res=1023 fr3` @80,288, and
the two shelf UNITS `res=1027 fr9` @320,352 / @384,352) spawned at the **cast layer 13**,
which draws OVER the layer-8 structure props and OCCLUDED them.  The draw pool walks its 27
layers in index order (lower = behind), so layer 13 > layer 8.  Retail draws the
frame/units FIRST (seq 257/261) then the small props ON TOP (seq 268+).

**Fix:** `room_cast_member` gained a per-member `layer` (0 ⇒ the default cast layer 13);
the BACKGROUND furniture (the bookshelf frame + the two shelf units) is set to **layer 7**
(below the layer-8 structure props).  `actor_spawn_room_cast` honours it; the family /
foreground props stay 13.  VERIFIED: the port seq order now matches retail (frame/units
before props) and both flagged shelves recon pixel-matching retail (fully stocked).
+test `errands_cast_zorder`.

LESSON: a "missing" element may be EMITTED-but-OCCLUDED — always check the draw-stream seq
order (the z), not just "is it drawn".  The ckpt-145 furniture stand-in (cast layer 13)
silently regressed the structure-band props behind it.

## Tooling note
`osr_prof.exe` (built `make -C tools/osr_view prof` → `build/osr_prof.exe`) reconstructs
any `.osr` frame headless: `osr_prof.exe <file.win> dump <frame_idx> <out.bmp>`.  Map a
sim_tick → frame index with `osr.stream_frames(path)` (enumerate; the Nth FRAMEBEG = index
N).  This is the "LOOK at it yourself" path used to locate the fire above.
