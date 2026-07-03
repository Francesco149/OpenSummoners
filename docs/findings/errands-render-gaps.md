# The errands (freeroam) render gaps — the USER studio notes, RE'd

The USER's `osr_notes.jsonl` (on `port-errands.osr`) flags the remaining errands gaps.
This is the RE of each (off `retail.osr` via `osr_prof` recon + `draw_probe`), so the
ports are clean follow-ups.  The exact USER marks (tick + crop + text):

| tick | crop [x,y,w,h]      | USER note                          |
|------|---------------------|------------------------------------|
| 1726 | [309,154,96,89]     | **missing fire in fireplace**      |
| 1726 | [75,283,30,38]      | **wrong wall colour** (→ DONE ckpt 148, §3 — per-room palette swap) |
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

**The SHEET decode was a port OVER-GRADE — FOUND + FIXED (ckpt 147, `main.c`), now
`differ_px==0`.**  At a SETTLED, fire-frame-MATCHED recon the decoded fire sheet's dhash
DIFFERED port↔retail (fr4 0x52fc66d1 vs 0xfa994282) while neighbour sheets matched cross-
side (res 1722 fr0-7 identical) — so res 1034 decoded to different RGB565 pixels, amplified
by the additive blend (maxd 66).  ROOT CAUSE: the port's in-game 8bpp colour-grade
(`main.c` `ar_sprite_decode`, gates 700/850) is applied to ALL 8bpp sheets except a
hardcoded UI blocklist; but retail grades ONLY sheets bound via the `0x417c40` getter
(tiles/sky) — the fire is an additive EFFECT sheet bound via the PLAIN getter `0x418470(0)`,
so retail does NOT grade it.  The port was over-darkening it (same class as
PORT-DEBT(banner-grade)).  FIX: add the fire's slot 406 (`FIRE_BANK_SLOT`) to the grade
exclusion list.  VERIFIED: the fire sheet dhash now == retail (fr4/fr5 byte-identical) and
the fire rect recon (matched fr4, settled) is **`differ_px==0`, maxd 0** — pixel-perfect.
(My first recon wrongly read the residual as a "fireplace base gap"; that was an artifact —
port tick 1695 was mid reveal-FADE at a different anim frame; the draw lists are IDENTICAL.)
Feed: the differ_px==0 PORT|RETAIL|DIFF montage.

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

## 3. The wall colour (note #4) — PER-ROOM PALETTE SWAP (FUN_00417bc0) — RESOLVED + PIXEL-EXACT (ckpt 148)

[75,283,30,38] "wrong wall colour": PORT wall WARM BROWN, retail cooler GREENISH-GRAY.
**RESOLVED + ported; the floor sheets are now `differ_px==0` vs retail and the USER's exact
wall crop reconstructs pixel-identical.**

**Pinned to res 1897/1898** (the dhash census method that nailed the fire).  Of every sheet
in the wall region, the backdrop (res 1002/1722/1082) + furniture (res 1023) MATCH retail
bit-for-bit; only **res 1897 (fr4/5/8) + res 1898 (fr4/7/11) DIFFERED** — the errands floor
tileset, the CLONED banks (0x187←0x184 / 0x188←0x185 = the town floor res 0x769/0x76a).
The town doesn't draw 1897/1898 in the arrival, so this is errands-only.

**The mechanism — a per-room PALETTE SWAP via the info-entry +8 (the SAME field as the NPC
colour variant, but a different sub-case of FUN_00417bc0).**  The town floor PIXELS (res
0x769/0x76a) render in the errands sliced against a DIFFERENT bank's palette — res **0x76b**
(a 32×32 palette-holder sheet whose embedded palette is the cooler errands-floor colours):
1. The errands floor banks 0x187/0x188 carry **+8 = &DAT_00675500** (`0057ca40.c:2286/2291`,
   a group3 DATA_SET in `group3_info_events[]`).
2. **FUN_00417bc0** is the UNIFIED +8 consumer.  Its table's first u16 is a SOURCE selector:
   `== 0` ⇒ remap entries WITHIN the bank's own embedded palette (the NPC body-bank shift,
   ported as the equivalent pixel-index remap); `!= 0` ⇒ a CROSS-SLOT swap — copy palette
   entries from the slot whose POOL index == that u16.  &DAT_00675500's first u16 = **0x186**
   = pool 0x186 = res 0x76b, copying entries 1..255 → the floor reads res 0x76b's grey palette.
3. Retail's per-tile palette build **0x490f30** does, per bank: embedded (`FUN_004178e0`) →
   FUN_00417bc0 (+8 swap) → FUN_004182d0 (+4 flag scale; a NO-OP for the floor — flag 0 — and
   for town-area rooms where `DAT_008a93fc`=0x14 hits the switch default) → tone-curve grade →
   install.  So the errands floor = grade(res 0x76b palette), the town floor = grade(res 0x769
   palette) — same grade, different source palette, same sheet pixels.

**Why the port was wrong:** the port modelled +8 ONLY as the NPC within-palette PIXEL remap
(`ar_npc_palette_remap`), which is IDENTITY for the floor's table (first u16 != 0) → no effect
→ the floor kept its own warm palette.  The earlier "grade ruled out then back in" confusion was
a red herring: the floor IS graded, but the grade runs AFTER a palette swap the port never did.

**The fix (`asset_register.c`, ckpt 148):** `ar_apply_slot_palette_swap` ports the cross-slot
half of FUN_00417bc0 — at decode (before the grade in `title_sheet_format`), if the bank's +8
names a swap table (`AR_PAL_SWAP_TABLES`, generated by `tools/extract/info_palette_swap_tables.py`),
overwrite the named palette entries with the source slot's RAW +0x34 palette (via
`ar_decode_slot_palette_raw` — NOT the BGRA-reordered `ar_palette_session_begin`, so src/dst stay
in the same channel order the grade+convert read).  Disjoint from the NPC remap (those tables have
first u16 == 0).  **VERIFIED off `port-walltint.osr` vs `retail.osr`:** floor sheets res 1897 fr4/5/8
+ res 1898 fr4/7/11 all `differ_px==0` (dhash byte-identical); the USER wall crop [75,283,30,38] at
tick 1726 reconstructs `differ_px==0, maxd==0`; town **58/58** + errands **73/73** shared sheets match
(no regression).  +host tests `cross_slot_palette_swap_tables` + `apply_slot_palette_swap_noop_paths`.
Feed: the PORT|RETAIL|DIFF wall+floor montage (floor dark = match).

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

## 5. Decode census — the errands DECODE arc is confirmed 1:1 (ckpt 148)

After the wall-tint fix, a FULL-capture sheet-dhash census (`port-walltint.osr` vs
`retail.osr`, all ticks) confirms **every shared decoded sheet matches retail** except two
PRE-EXISTING, non-errands residuals — i.e. the floor was the LAST wrong-decode gap in the
errands; everything the port draws in the town + errands decodes bit-for-bit like retail.
- Town (ticks 80-360): **58/58** shared sheets match.  Errands (1680-2430): **73/73** match.
- Full capture: **287/289** shared sheets match.  The 2 that differ are NOT errands floor
  gaps and are NOT touched by the +8 swap (no swap table):
  - **res 1110 (=0x456, the dialogue-box 9-slice)** — the port slices it into 30×30 cels,
    retail into 26×26 (a slice/padding difference); a byte compare of the differently-sized
    cels is meaningless geometry.  The box RENDER is verified `differ_px==0` (ckpt 135/136,
    the matched-cadence nav); a recon at tick 690 over THIS errands nav shows ~1700 box-band
    px diff, but that is the DIALOGUE/PORTRAIT phase (the errands-reaching nav isn't the
    tick-1:1 dialogue nav — the bust sits at a different fade step), NOT res 1110.  Benign.
  - **res 2331 fr2 (=0x91b, the title MAIN sheet)** — a ~960px localized diff in one title
    frame, a pre-game TITLE residual (out of the errands arc; the title was verified 1:1
    early — likely a title menu-state/animation-phase frame not on the verified path).
- The REMAINING errands gaps are all MISSING-element (not wrong-decode): the res=0 freeroam
  HUD (notes #7-9) + door indicator (#5), and the idle-fidget Arche cels (res 1392, note #6,
  the RNG behaviour subsystem 0x54f980) — best with the USER / a subsystem port.

## 6. The t2278 mark "missing pot and kitchen cabinet" — the POT RENDERS (stale-trace artifact); the CABINET was the real gap (FIXED, ckpt 180)

USER mark (`osr_notes` t2278, on the `port-waitfix | retail-stairs` pair; crop [218,83,209,159],
differ 22498).  RE'd off `draw_probe` on `retail-stairs.osr` / `port-waitfix.osr` / `port-stairs2.osr`.

**The POT is NOT missing — it is a `port-waitfix` STALE-TRACE artifact.**  At t2278 every prop in the
port-waitfix crop sits **+176px right** of retail (measured on 4 props: pot res1074 fr2 552 vs 376;
wall-shelf res1023 fr0 284 vs 108; counter res1023 fr6 208 vs 32; res1139 232 vs 56) while Arche
(res1392 fr14) is screen-CENTRED both sides (272 vs 270) — an X-follow camera holding Arche centred
while her WORLD position lags retail's by 176px (17600 wx).  So the pot (res1074 fr2) DOES render — it
is shoved off the crop's right edge.  **Proof:** `port-stairs2.osr` (ckpt-177, pre-waitfix, walk
0-div/301t) at t2278 matches retail-stairs BIT-FOR-BIT — wall-shelf 108/108, counter 32/32, Arche
(270,296)/(270,296), **pot res1074 fr2 376/376**.  The walk sim is bit-exact; port-waitfix's freeroam
walk is DESYNCED because the ckpt-179 dialogue fix (`ARRIVAL_EXIT_WAIT` 10→20) shifted the scene phase
but the tick-keyed held-trace (`synth-stairs`: hold RIGHT @2050) was NOT re-timed → the RIGHT hold
lands at a different scene phase → Arche walks 176px less by t2278.  **⇒ the studio pair for FREEROAM
verification must be `port-stairs2 | retail-stairs`, NOT port-waitfix** (waitfix is valid only for the
DIALOGUE window, ≲tick 2000).

**The real gap = the RIGHT-side / upstairs props ERRANDS_CAST SYSTEMATICALLY missed — 6 objects, FIXED.**
`ERRANDS_CAST` was captured from the STATIC tick-2200 view (cam 0/16000), which shows only the LEFT/centre
of the room — so EVERY object on the right (world_x ≳ 55000) was dropped.  A full port-vs-retail draw diff
at the **camera clamp (t2500, both cameras pinned at the map's right edge → phase-independent alignment)**
surfaces them all.  Each is a DATA-1025 CHARACTER object; res→bank = `asset_register` slot + 13; world =
map layer pos ×100 (validated: the map position AND the retail clamp screen pos agree exactly):

| USER mark | res / fr | bank | map code (layer) | world | screen @clamp |
|-----------|----------|------|------------------|-------|---------------|
| kitchen cabinet | 1023 / 4 | 0x16f | 0x112d1 (L18) | (70400,25600) | (256,168) |
| upstairs hutch  | 1023 / 2 | 0x16f | 0x112d1 (L31) | (70400,6400)  | (256,−24) |
| **the POT** (2nd USER mark, "right of Mom's head") | 1026 / 58 | 0x16b | 0x112da (L27) | (67600,29600) | (228,208) |
| upstairs prop   | 1026 / 38 | 0x16b | 0x11279 (L34) | (56000,8800)  | (112,0) |
| upstairs furniture | 1023 / 13 | 0x16f | 0x112d3 (L36) | (60000,6400) | (152,−24) |
| upstairs prop   | 1022 / 4 | 0x156 | 0x1124c (L97) | (83200,12800) | (384,40) |

**Fix (`actor_spawn.c`):** 6 ERRANDS_CAST entries (the cabinet/hutch at LAYER 7 = pre-structure per retail
seq 223/224; the rest at the default foreground layer 13 per seq 282-288).  **VERIFIED** off the rebuilt
walk-driven `port-cabinet.osr` at the clamp: all 6 render at retail's EXACT screen pos + dims.  Still
PORT-DEBT(errands-cast) — a captured stand-in; the recurring gap (cabinet, then pot+3) shows the
tick-2200-capture approach is fragile, so the proper fix (the CHARACTER def-table fill spawning from the
MAP, so ALL objects come regardless of camera) is now the priority retire.

**ROOM PROPS NOW VERIFIED COMPLETE (both camera positions).**  A cross-ref of the DATA-1025 map CHARACTER
objects vs the retail draws (`tools/…/xref.py`, staged in the scratchpad) resolves EVERY furniture/prop
code to a bank (res 1023→0x16f, 1026→0x16b, 1027→0x16c, 1022→0x156; most at d=0 exact-position match) and
confirms ERRANDS_CAST now covers ALL 20 of the map's VISIBLE CHARACTER objects (6×res1023 + 3×res1026 +
10×res1027 + 1×res1022 == the 10 shop-prop `res1027` frames + the 6+3+1 furniture already listed).  A
port-vs-retail draw diff at BOTH t1800 (left/centre) AND the t2500 clamp (right) shows **no missing
prop** on either side.  The map's other CHARACTER codes (0x111d6/d9/f2, 0x11365/6f, 0x112e4 — variant 0,
downstairs / off-screen far-right x≥1056) are INVISIBLE VOLUMES (as in the town, where only 3 of 32
CHARACTER codes draw).  The frame_base=VARIANT (+0x18) model is proven (bookshelf var3 / cabinet var4 /
hutch var2 / counter var6 / upfurn var13 / prop1022 var4 all == the retail frame); the 2 apparent
mismatches (clock var43 vs seen fr44, pot var56 vs fr58) are the ANIMATED objects at a mid-anim frame.

**The clock + pot ANIMATE (ckpt 180b).**  Both render static no longer — RE'd off the retail res1026
frame sequences: the **pot STEAMS** (var56 + `POT_CLIP` delta {1,2,3,4} = cels 57→58→59→60, dur 6, loop)
and the **clock pendulum SWINGS** (var43 + `CLOCK_CLIP` delta {0,1,2,1} = 43→44→45→44, dur 25, loop) —
VERIFIED off `port-cabinet.osr` (pot 58→59→60→57 dur6; clock 45→44→43→44 dur25 == retail).  Only the
per-object start PHASE is un-seeded (the `effect-anim-phase` pattern — the loops run, but not tick-locked
to retail's phase).

**Family Z-ORDER fixed (ckpt 180b, USER mark).**  A chair (res1027 fr5 @184,232, the 0x112a2 shop prop)
drew OVER Mom because the family (Father/counter/Mother) were spawned BEFORE the 10 res1027 shop props
in ERRANDS_CAST — both layer 13, so draw order = array order.  Retail's t2500 seq proves the PEOPLE are
frontmost (all props 252-286, THEN Mom 289), so the family block moved to AFTER the shop props (the
counter kept right after Father = drawn in front of him).  VERIFIED: chairs now at seq 280/281, Mom at
287.  (This is the same class as the §4 layer-7 shelf-frame fix — a "missing" element is often
EMITTED-but-mis-Z'd; the port's `draw_pool` has no Y-sort, so intra-layer order = spawn order.)

**Remaining (NOT missing props):** (1) the freeroam HUD — **res=1900 fr0 @(8,444)** is the bottom-left
HUD strip (registered slot 39, SCREEN-anchored seq 431 among the res1103/1104 HUD draws — NOT a room
object; there is no map object at its world pos), plus the blank portrait (`leader_uid` blocker) + the
lower strips (§2).  (2) Mom's POSE (retail res1127 fr0 vs port fr2 — a facing/frame diff) + the clock/pot
anim start-phase seed.  The runtime map-driven CHARACTER spawn (retiring the ERRANDS_CAST
capture) is now pure cleanup — blocked only on the character-band z-order (the port's `draw_pool`
draws by layer+emission with NO Y-sort, unlike retail's depth-sorted band), with the code→bank table +
variant model ready.

LESSON: a studio mark on a PHASE-desynced capture pair reads a real prop as "missing" when it is merely
DISPLACED (the pot's neighbour, the STOVE res1074, is one such — it renders, just off-crop).  Confirm the
pair is walk/phase-aligned — or use the camera CLAMP (both pinned → identical view regardless of phase) —
before attributing a crop diff to a missing spawn.  Attribute to the PHASE pillar first (`parity-model.md`).

## 7. FIXED — the upstairs BED culled "until fully in frame" = a mode-0 CANVAS-vs-CONTENT cull bug (ckpt 181)

USER marks (`osr_notes`): t2158 crop[459,0,128,49] "bed missing on port"; t2325 crop[129,0,141,86]
"bed becomes visible on port".  **The ckpt-180c "decode bug in res=1071 fr6" hypothesis was WRONG on
every count** — recorded here as the trap.  What it got wrong, proven step-by-step:

1. **The bed is NOT res=1071 fr6.**  res=1071 fr6 is the upstairs WALL-WITH-WINDOW tile (128×160,
   magenta 0xf81f = the colorkeyed window panes at rows ~96-128; rows 128-160 are plain dark wood).
   Its sheet is **byte-IDENTICAL** port↔retail (`sheet_export.py DIFF … 1071 … 6`: all 160 rows equal,
   both dhash 0x3ad4b5df) — so NOT a decode bug, and the "64-bit dhash collides on a 32-row diff"
   reasoning was spurious (the SHEET dhash is a full FNV-1a of the bytes; a real collision is ~1/4e9).
2. **The BED is a keyed ACTOR — `res=1023 fr13`** (bank 0x16f, the furniture sheet; dhash 0x37f2156f),
   already in `ERRANDS_CAST` at world (60000,6400).  `osr_prof pick` (added: `osr_prof … pick <idx>
   <px> <py>`) names it: the white pixel (530,8) is painted by retail draw `#341 keyed res=1023 f=13
   @(464,-96)`.  A `keyed` blit ignores reqw/reqh and draws the cel's FULL cel via the pivot — the .osr
   "108×60" is the trimmed content; the pivot (metric_0c/10 = 5,68) drops the content to dest top =
   68-96 = -28..+32, poking on-screen.
3. **The port SPAWNS it (drew it fine at t2325, dst_y=-60) but CULLED it at t2158 (dst_y=-96).**  So a
   CULL bug, not a missing spawn — `frame_diff.py` confirms the port stream simply lacks the fr13 draw
   (+ 8 more upstairs objects) at t2158.

**ROOT CAUSE:** `map_present` mode-0 cull box (retail `FUN_0048eac0` case-0, line 51) tests
`-1 < cel[0x20] + sy` reading the cel's **CANVAS dims +0x1c/+0x20** (`metric_1c/20` = the untrimmed
frame size).  The port's `game_cel_dims` fed the **CONTENT dims +0xb8/+0xbc** (`metric_b8/bc` = the
trimmed source rect) instead.  For an untrimmed cel (pivot 0) the two coincide, so it passed until
now; the bed (canvas h 128, content h 60, pivot y 68) at projected sy=-96 is CULLED by content-h
(-96+60 = -36 < 0) but DRAWN by canvas-h (-96+128 = +32 ≥ 0), and the keyed blit's pivot lands the
content at -28..+32, visibly poking in.  (op.w/op.h from `game_cel_dims` feed ONLY the cull — the
keyed blit reads the cel's own metric_b8/bc for the Blt + trace, so the fix doesn't touch reqw/reqh.)

**FIX (`main.c` `game_cel_dims`):** return `metric_1c`/`metric_20` (canvas) — bit-exact with retail's
`0x48eac0` case-0/1 cull (both read cel +0x1c/+0x20).  The decode already stamps them: `zdd.c` sprite
decode → `stamp_metrics(self, metric_w, metric_h, trim.x_left, trim.y_top, w, h)` puts canvas → 1c/20,
pivot → 0c/10, content → b8/bc.  **VERIFIED** off `port-bedfix.osr` (rebuilt `spam-confirm-nav` +
`synth-stairs`): at t2158 the port now emits `res=1023 fr13 @(464,-96)` == retail (same frame/dhash/
dst/o); the bed reconstructs **`differ_px==0`** vs retail over [460-575 × 0-33]; the 3-way OLD|FIXED|
retail feed montage shows it appear.  `frame_diff` port-OLD vs port-NEW: 0 removed, +9 added (the bed
+ 2 upstairs chests + 6 props — exactly the objects retail draws that were wrongly culled).  +host
test `map_present_walk_mode0_canvas_cull_bed`; 1096 host pass.  New tools: `tools/trace_studio2/
sheet_export.py` (SHEET raw-pixel dump + row diff), `blits_at.py`/`region_draws.py`/`frame_diff.py`
(streaming per-tick blit dumps), `osr_prof pick` (which draw painted a pixel).

LESSON: a "renders empty / until fully in frame" symptom on a colorkeyed background tile invites a
decode/blit-src theory, but the missing thing was a foreground ACTOR wrongly CULLED — attribute via
`osr_prof pick` (what actually painted the pixel) before theorising, and never let a "dhash collides"
hand-wave stand in for extracting the bytes.  Also: `game_cel_dims`'s "b8/bc == the cull dims" comment
was a KNOWN-shaky shortcut (it said "for these frame cels those equal the source dims") — the bed's
non-zero vertical pivot is the trimmed cel that finally broke that assumption.

## 8. FIXED — the upstairs shelf PILE hidden behind its shelf-back = a band/layer z-order bug (ckpt 182)

USER note (`osr_notes` t2148, crop[288,0,65,40]): "props missing on shelf on port".  The upstairs
shelf PILE — colored books + a cream book-stack + a green box — rendered as bare dark wood in the
port; retail shows the full pile.  NOT the §7 cull bug (the objects ARE emitted).

**What each object is (proven via `map_data.py` on DATA-1025 + `osr_prof pick`):**
- The PILE = two **STRUCTURE**-band objects, code `0xec69` (bank 0x16b = res1026), layer 8:
  idx 43 var13 @map(408,128) → res1026 **fr13** (the books), idx 42 var34 @map(404,148) → res1026
  **fr34** (the stack/box).  (All errands STRUCTURE objects are fg=0 → layer 8.)
- The shelf-BACK it sits in = three **CHARACTER**-band objects @map y128: code `0x112dc` var8 →
  res1027 **fr8**, code `0x112db` var14/var11 → res1027 **fr14/fr11**.  These resolve to bank 0x16c
  (res1027) with frame = the variant.

**ROOT CAUSE (a BAND/LAYER mismatch, not a cull, not a decode):**  The port has no map-driven
CHARACTER band for non-town rooms (PORT-DEBT(errands-cast)); those three shelf-backs are stood in by
`ERRANDS_CAST` (`g_room_cast`) entries at the DEFAULT layer 13.  `g_structs` (the STRUCTURE band,
holding the L8 pile) renders BEFORE `g_room_cast` in `game_actor_walk` (main.c) — but the draw pool
presents by LAYER, so layer 8 (pile) presents before layer 13 (shelf-backs): **the shelf-backs drew
OVER the pile → pile hidden.**  In RETAIL the shelf-backs are real CHARACTER objects: the CHARACTER
band (`0x431e30`, pool 0x11e0) is walked BEFORE the STRUCTURE band (`0x493230`, pool 0x2560) in the
present builder `0x48c150` (lines 48 vs 81), and BOTH emit into layer 8, so the character shelf-backs
land at a LOWER seq than the structure pile — behind it (retail flip 3250: res1027 fr14/fr8 seq
282/283, res1026 fr34/fr13 seq 304/305).  The dispatcher `0x58d460` inserts each band in MAP-OBJECT
ORDER with NO depth sort; the layer (8 vs 15) is the object's +0x30 flag (`puVar14[0xc]`: 0→8, 1→15).

**FIX (`ERRANDS_CAST`):** the 3 upstairs shelf-backs res1027 fr8/fr11/fr14 → **layer 7** (== the
downstairs shelf units fr9 already at 7, "behind its layer-8 props").  Since the port renders
`g_room_cast` AFTER `g_structs`, layer 7 (one below the L8 pile) is the correct PORT classification —
layer 8 would still draw them over the pile (opposite band order to retail).  Same class as the §6
family z-order fix.  **VERIFIED** off `port-shelffix.osr` at the camera-aligned flip 5412 (res1071
fr9 dst_x=276 == retail; note: tick 2148 coalesces flips 5411@281/5412@276 — compare on the 276
sub-frame): res1027 fr8/fr14 now seq #282/#286 BEFORE res1026 fr34 #300; the pile reconstructs
**`differ_px==0`** vs retail over x[290,360] y[0,80]; full-frame excl-HUD diff 1531→904 (exactly the
627 pile px removed, nothing added — the 904 residual = the pre-existing family-pose + Arche
walk-phase gaps).  1096 host pass.

**OPEN follow-up:** res1027 **fr64** (ERRANDS_CAST, retail seq 289 — also behind the res1026 block,
i.e. another shelf-back) is left at layer 13.  No res1026 prop overlaps it at t2148 so it produces
NO pixel diff there (unverifiable by the frame diff); it is a drawcall-ORDER-only faithfulness item.
The real retirement is the map-driven CHARACTER-band spawn (codes 0x112db/0x112dc/… → bank + variant
+ the 0x431e30 layer), which subsumes all of these ERRANDS_CAST shelf stand-ins — still PORT-DEBT.

LESSON: a "missing prop on a shelf" whose object IS in the draw stream (frame_diff shows it present
both sides at matching res/frame/dst) is a Z-ORDER bug — and when the two objects come from DIFFERENT
port bands (here STRUCTURE vs the ERRANDS_CAST stand-in), the fix is the LAYER, because the port's
fixed band-render order can't reproduce retail's character-before-structure emission.  Confirm the
compare frame is CAMERA-aligned (a tick can coalesce two flips at different camera-x) before reading
a pixel diff, else a 5px camera phase paints the whole frame as "differ".

## 9. The errands CHARACTER band is MAP-DRIVEN — ERRANDS_CAST prop capture retired (ckpt 183)

USER: "un-mvp whatever is mvp'd about this scene, session by session."  §8 fixed the shelf pile via
an ERRANDS_CAST layer tweak (a capture stand-in); this session retires the capture itself: the shop
furniture/shelf/props now spawn from the MAP (DATA-1025), like the STRUCTURE band already did.

**The two bands (both are map objects; dispatcher 0x58d460 routes by code range):**
- STRUCTURE (60000-69999 → 0x438a60 → pool 0x2560, layer 8/15): already map-driven (`g_structs`).
- CHARACTER (70000-79999 → 0x431e30 → pool 0x11e0): was the ERRANDS_CAST capture; now map-driven.

**The def table (RE'd from 0x431e30's per-case switch), `CHAR_BANK_DEFS` in actor_spawn.c:** each
VISIBLE case installs one sprite row via `0x426d70(0, BANK, variant)`; the draw layer (actor+0xfc) is
`0x438610(N)` when the case calls it, else the DEFAULT 9 (`0x426ec0` = the clock/pot anim-phase init).
`frame_base = the map record's VARIANT (+0x18)` = `0x426d70`'s param_6 — the SAME source STRUCTURE
uses, and == the town's ex-captured frame_base (0x1129e var1 / 0x1129f var2 / 0x112e5 var36, verified).
Codes with no sprite install = invisible volumes (bank 0): the town's 29/32 + the errands' 0x111d6/
d9/f2 / 0x11365 / 0x1136f (var-0 collision/trigger zones).

errands CHARACTER code → (bank, layer, frame=variant):
  0x112cf→0x16f L9 (wall shelf) | 0x112d1→0x16f **L5** (bookshelf/cabinet/hutch) | 0x112d3→0x16f L9 |
  0x11279→0x16b L9 | 0x112db/dc→0x16c **L5** (shelf-BACK units; fr64 IS 0x112dc var64 → L5) |
  0x112a1/a2→0x16c L9 | 0x11274→0x16c L9 | 0x1124c→0x156 L9.
The L5 shelf-backs draw BEHIND the L8 structure pile — SUBSUMES §8's L7 stand-in (retail's true L5)
AND auto-fixes fr64.  z is PURELY by layer number (char L5/9/10 vs struct L8/15 — no shared layer),
so the present (0x48eac0, layer-ordered) needs no band reorder; the CHARACTER band renders where it
sits in game_actor_walk.

**Wiring (main.c):** `reload_room_backdrop` now spawns `g_actors` from the room map for the non-town
rooms too (mirroring the g_structs spawn); `game_actor_walk` renders the CHARACTER band for EVERY
room (gate `room_is_town` dropped).  `actor_spawn_sprite_for_code` returns (bank, layer); the caller
reads the variant for frame_base.  `TOWN_SPRITE_DEFS` folded into `CHAR_BANK_DEFS`.

**DEFERRED to later un-MVP sessions (still ERRANDS_CAST; the map band spawns these 4 codes as
invisible volumes → no double-draw):** clock 0x112d9 / pot 0x112da (need the g_actors per-tick clip
update wired for non-town), fire 0x112e4 (res1034, ADDITIVE mode-1 — needs node_alpha in the
map-spawn), and the FAMILY Father 0xe3 / Mother 0xb5 + Dad's COUNTER 0x112d2 (the party band
0x4997b0, PORT-DEBT(cutscene-party-chars); the counter's z rides with the family).

**VERIFIED** off `port-charband.osr` (camera-aligned flip 5412 vs retail-stairs t2148): the shelf pile
reconstructs **`differ_px==0`**; full-frame excl-HUD diff **899px** (5px BETTER than the ckpt-182 L7
build's 904 — fr64 now correct) = the pre-existing family-pose + Arche walk-phase residuals only.
TOWN t800 + HOUSE t1450 are **0px** vs the pre-session build (no regression — frame_base==variant for
the town; the house map has no in-table CHARACTER furniture at t1450).  1096 host pass.  `port-debt.md`
errands-cast SHRUNK to the deferred cast.

## 10. The errands ANIM props (clock + pot) are MAP-DRIVEN — bit-exact vs retail (ckpt 184)

USER: "un-mvp whatever is mvp'd about this scene, session by session."  §9 map-drove the STATIC shop
furniture; this session retires the two ANIMATED props — the pendulum CLOCK (code 0x112d9) and the
cooking POT (0x112da) — from the ERRANDS_CAST capture.  They now spawn from the map (DATA-1025) into
the CHARACTER band (`g_actors`) with their anim clip, driven by the SAME per-tick stepper as the town
wagon.

**The map positions are ground truth** (`map_data.py` DATA-1025, the +0x18 variant): clock @map(528,248)
var43 → world (52800,24800); pot @map(676,296) var56 → world (67600,29600) — EXACTLY the ex-ERRANDS_CAST
fitted world (which ckpt-180 had already set to map×100).  So the migration moves nothing.

**Wiring (three edits):**
1. `CHAR_BANK_DEFS` (actor_spawn.c): +2 rows — 0x112d9/0x112da → (bank 0x16b, layer **9**).  L9 is
   retail's default-9 for these codes (0x431e30 routes them through `0x426ec0` = the clock/pot
   ANIM-PHASE init, NOT `0x438610(N)` = a custom layer), superseding the ex-capture's default L13.
2. `actor_spawn_clip_for_code(code)` (actor_spawn.c): a code→clip lookup — 0x112d9→CLOCK_CLIP (swing
   {0,1,2,1} dur25 → cels 43,44,45,44), 0x112da→POT_CLIP (steam {1,2,3,4} dur6 → cels 57..60); every
   other CHARACTER code → NULL (static).  Wired in `actor_spawn_from_map` (rs->clip = lookup(code)).
3. `main.c` game-loop: the non-town per-tick branch now calls `actor_pool_update(&g_actors)` (it only
   ran `actor_pool_update(&g_room_cast)` before — the town's `game_actor_update` doesn't run for
   non-town rooms).  The static map props no-op on clip==NULL; only the clock/pot advance.
Removed clock/pot from ERRANDS_CAST (else double-draw); the map band spawned them as invisible volumes
before, now visible + animated.

**VERIFIED bit-exact vs retail-stairs at the CLAMP (tick 2419-2421, both cameras pinned at the map's
right edge → phase-independent).**  `draw_probe --res 1026`, port-clockpot.osr vs retail-stairs.osr:

    RETAIL  seq=282 keyed res=1026 fr=57 dst=(228,208 28x35)   # the POT
            seq=283 keyed res=1026 fr=45 dst=(80,160 29x40)    # the CLOCK
    PORT    seq=282 keyed res=1026 fr=57 dst=(228,208 28x35)   # == retail (pot @228,208 = ckpt-180 GT)
            seq=283 keyed res=1026 fr=45 dst=(80,160 29x40)    # == retail

Identical draw / frame / dst AND **seq (z-order 282/283)** — the map band emits them at retail's exact
draw-sequence slot; and the anim PHASE aligns at tick-equal (both pot=57 clock=45 @t2420, deterministic,
no RNG).  Over ticks 2420-2470 the port clock swings 45→44→43 (dur25) + pot steams 57→58→59→60 (dur6),
and the z-order overlap scan reports **"nothing over" the pot or clock at every tick** — so the L13→L9
layer change is NO regression (nothing occludes them at the clamp).  `actor_spawn_room_cast` now returns
**4** members (fire + family + counter, was 6).  1097 host pass (+`test_errands_clock_pot_mapdriven`:
the map-spawn assigns bank 0x16b + the variant frame_base + the swing/steam clip, and no bank-0x16b
room-cast member remains).

REMAINING in ERRANDS_CAST (next sessions): the additive FIRE 0x112e4 (mode-1 node_alpha map-spawn),
the FAMILY (Father/Mother) + counter (the party band 0x4997b0, `cutscene-party-chars`).

## 11. The fireplace FIRE (0x112e4) is MAP-DRIVEN + ADDITIVE at LAYER 6 — vs the ex-capture's L13 (ckpt 185)

Continuing the un-MVP: the additive fireplace FIRE (res1034) was the last animated ERRANDS_CAST member.
Now map-driven from DATA-1025 into the CHARACTER band, carrying its additive blend.

**RE'd off the 0x431e30 fire case (`docs/decompiled/by-address/431e30.c:739`):**

    case 0x112e4:
      FUN_00426620(2,param_4,0,param_1,param_2,0x1900,0x1900,...);  // spawn box 6400x6400 @ (param_1,param_2)=map*100
      FUN_00426d70(0,0x1a3,0);            // install sprite row: bank 0x1a3, frame_base 0
      FUN_00407b80(iVar2,&DAT_00647e58);  // the FIRE clip (6 frames)
      FUN_00426ec0(iVar2);                // anim-phase init (the clock/pot default-9 path)
      FUN_004385c0(DAT_008a92f0);         // the ADDITIVE blend descriptor = ramp_a[14]
      FUN_00438610(6);                    // draw LAYER 6  <-- NOT the ex-capture's default L13

So retail draws the fire at **layer 6** — BEHIND the layer-8 structure furniture (the fireplace grate +
mantel).  Because the blit is ADDITIVE, the layer genuinely matters (it adds to whatever is behind it),
unlike the opaque props.  The ex-ERRANDS_CAST fire was at the default cast L13 (drawn OVER everything) —
a stand-in that looked OK only because nothing distinguished it at the tested tick.

**Wiring:** `CHAR_BANK_DEFS` +1 row 0x112e4 → (bank 0x1a3, layer 6); `actor_spawn_clip_for_code` +fire
→ FIRE_CLIP; new `actor_spawn_alpha_for_code(code)` → 14 for the fire (else 0), wired into
`actor_spawn_from_map` (`a->node_alpha`).  Removed the fire from ERRANDS_CAST (now 3 members: family +
counter).  The map band already spawns 0x112e4 (was an invisible volume); now it's visible + additive.

**Position is exact by construction:** the map world (32000,32000) + dst_base 0 projects to
(320,160)+pivot — the SAME net screen pos as the ex-fit world (32900,33800)+dst(-9,-18) = (320,160)+pivot
(the fit just split the placement between world and dst_base).  So the fire lands at the ckpt-163 verified
screen dst (329,178) 48×39, unchanged.

**VERIFIED off `port-fire.osr` vs `retail-stairs.osr`** at the errands ENTRY (camera at the left; the fire
x=32000 is off-screen at the right-edge CLAMP, so the entry is the compare window).  `draw_probe --res
1034`:

    RETAIL @t1710  seq=264  alpha res=1034 fr=3 dst=(329,178 48x39) bmode=1
    PORT   @t1710  seq=262  alpha res=1034 fr=4 dst=(329,178 48x39) bmode=1

Identical primitive (alpha/additive), res, dst, and bmode=1.  The seq (262 vs 264) is in the LOW
CHARACTER-band range in BOTH — confirming layer 6 (the ex-L13 fire would sit at seq ~518, AFTER the
mantel res1098 @seq517).  The fire's z is behind the later structure draws in both streams.  The ±1 anim
frame (port fr4 vs retail fr3 at tick-equal) is the known −6t errands-entry latency
(PORT-DEBT(cutscene-errands-entry-latency)), not a fire gap.  **VISUALLY PIXEL-IDENTICAL** (osr_prof recon
of BOTH port-fire + retail-stairs @t2040, dialogue clear; feed `ckpt185 fire PORT | RETAIL`): the fire
glows in the hearth BEHIND the metal grate + wooden mantel — the two fireplaces are indistinguishable, so
the layer-6 additive z is faithful and the fire is fully visible (the res=0 grate/mantel that the retail
proxy couldn't ID DO reconstruct — the ambiguity was a capture-side ID gap only).  1097 host pass
(`test_errands_fire` rewritten for the map spawn: bank 0x1a3 / L6 / node_alpha 14 / FIRE_CLIP / world
32000,32000, GONE from ERRANDS_CAST).

REMAINING in ERRANDS_CAST: only the FAMILY (Father 0xe3 / Mother 0xb5) + Dad's counter 0x112d2 — the
party band 0x4997b0 (`cutscene-party-chars`), a Phase-3 subsystem.

## 12. Status after the prop/anim/fire migrations — the family is the LAST stand-in (ckpt 185)

After ckpt 183-185 the errands scene's STRUCTURE + CHARACTER props, the anim props (clock/pot), and the
additive fire are ALL map-driven + bit-exact.  The only remaining ERRANDS_CAST stand-in is the FAMILY
(Father 0xe3, Mother 0xb5, Dad's counter 0x112d2), which is NOT map-driven-able: Father/Mother are NOT
map objects (their codes 0xe3/0xb5 are outside the 70000-79999 CHARACTER band — they are party-band
entities), and the counter's z "rides with the family" (drawn in front of Father, who stands behind the
counter), so it can't be map-driven independently while the family sits at the port's L13 stand-in.  So
the family is genuinely blocked on the **party band 0x4997b0** (`PORT-DEBT(cutscene-party-chars)`, Phase 3
— the persistent-leader path + multi-part body + the 0x402730/0x402330 actor movers).

**Mom's-pose note (corrects the FRONT §180 "res1127 fr0 vs fr2" read):** at the clamp, retail's Mother
ANIMATES an idle breathe (res1127 fr2→fr3, `draw_probe --res 1127` t2420-25) and the port's Mother
animates the SAME clip (IDLE_CLIP) at a **~1-frame phase offset** — the dst is IDENTICAL (176,200) when
the frames match (both fr3 @t2423).  So it is NOT a wrong pose/clip; it is an anim-PHASE residual, the
same class as the −6t errands-entry latency (`cutscene-errands-entry-latency`) + the family clip_phase
seed.  It will resolve when the family's phase is driven by the party band, not by fixing a frame here.

## 13. The family are CHARACTER-band NPCs, NOT party-band — Father's z-order FIXED (ckpt 186)

**Corrects the ckpt-180b/183/185/§12 model.**  The FRONT + §12 said the errands FAMILY
(Father 0xe3 / Mother 0xb5 / Dad's counter 0x112d2) are "party-band entities blocked on
0x4997b0".  The **retail-stairs CLAMP draw seq (t2420)** DISPROVES that: the parents render
in the CHARACTER band (the 0x1160 pool, `0x48c150:89-96` → `0x493ba0(0,slot,0)` param_3=0 —
the SAME band as the shop props), interleaved with the props by SLOT-INDEX order.  Only
ARCHE (the leader, room_state+0x200c) is party-band (`0x48c150:99-106` → `0x4997b0` param_2=1,
rendered AFTER the character band).

**Ground truth — retail-stairs.osr @ t2420 (both cameras pinned at the right-edge clamp):**

| entity            | retail seq | port (pre-fix) | dst          | frame (R/port) | status |
|-------------------|-----------:|---------------:|--------------|----------------|--------|
| Father  res1139   | **#257**   | #288           | (32,392)     | fr6 / fr5      | pos exact; **Z-ORDER wrong** + anim-phase |
| counter res1023f6 | #287       | #289           | (8,360)      | fr6 / fr6      | **bit-exact** |
| Mother  res1127   | #289       | #290           | (176,200)    | fr2 / fr0      | pos exact; anim-phase |
| Arche   res1392   | #290       | #291           | (398,248)    | fr13 / fr13    | **bit-exact** (leader) |

Retail draws **Father EARLY (#257)** — right after the shelf-backs/cabinets (#250-256) and
BEFORE the res1026 floor items — so everything he overlaps sits IN FRONT of him: the two
floor items **res1026 fr48 @(24,400)** (retail #269) and **fr51 @(28,444)** (#270) occlude
his legs, and the counter (#287) his torso.  The port put the whole family at L13/frontmost
(the ckpt-180b "family block after the props" over-correction — it fixed Mom-over-the-chair
but wrongly pulled FATHER to the front), so the port drew Father's legs OVER the two floor
items.

**FIX (1 field): Father's ERRANDS_CAST layer 0 (→13) → 7.**  The two floor items he overlaps
are modeled in the port's STRUCTURE band at **L8** (interleaved with the L8 upstairs pile, §8)
— NOT L9 — so Father must sit BELOW L8.  A first attempt at L8 did NOT clear them (g_room_cast
emits AFTER g_structs, so an L8 Father still draws over the L8 structs — which *confirms* the
floor items are L8-structure); **L7** draws before the L8 structs and before the L9 character
band + the L13 counter, and after the L5/L6 shelf-backs/fire he does NOT overlap.  Mother +
counter keep L13 (Mom over the chair res1027 fr5 @184,232; counter over Father).

**VERIFIED off `port-fatherz.osr` (Father-visible tick, camera-independent z-order):** Father
now emits at **seq #263 — BEFORE** res1026 fr48 (#279) / fr51 (#282) / the counter (#315) —
so the floor items + counter draw over him, matching retail's Father-behind ordering (#257 <
#269/#270/#287).  Placement is clean: #258-261 L5 shelf-backs → #262 L6 fire → **#263 Father
(L7)** → #265+ L8 structure → L9 characters → L13 counter.  1097 host pass; town/house use no
ERRANDS_CAST (no regression by construction).

**CAMERA-ALIGNED confirm (the fixed Z-spam capture recipe reaches the right-edge clamp — see the FRONT
tooling note):** `port-fatherz.osr` @t2420 with both cameras pinned == retail-stairs: Father **#257**
fr5 @(32,392) (retail #257 fr6 — SAME seq + dst), BEFORE res1026 fr48 (#270) / fr51 (#271); counter
@(8,360), Mother @(176,200), Arche @(398,248) all == retail's positions.  The excl-HUD pixel-diff at
the clamp is ONLY the parents' breathe anim-PHASE (Father fr5-vs-fr6 silhouette) + the HUD item-bar
stand-in — **NO z-order artifact** (the register/vase render over Father in BOTH).  Feed image
`ckpt186: Father z-order VERIFIED camera-aligned`.

**Spawn provenance (decompile, `0x4dc510` case-7/8):** the parents are PERSISTENT handle
entities `0x5f5e1d3` (Father) / `0x5f5e1d4` (Mother), created in the arrival cutscene and
positioned by the errands script via `0x41ec20(handle, 0x65, worldX, 0, facing)` (Father
worldX -4000/fac1, Mother +4000/fac3, relative to anchor 0x65).  The `+0x4030` party-slot loop
at `0x4dc510:1390` iterates 8 slots checking `!= 0x5f5e168` (Arche/leader) — LEADER bookkeeping,
NOT parent placement.  So the retire path for PORT-DEBT(cutscene-party-chars) (parents) is the
errands SCENE-SCRIPT spawn (0x41ec20 handle placement); **the parents are NOT blocked on the
party band 0x4997b0** — only ARCHE-as-leader + her multi-part body are.

**Remaining residuals (unchanged, RNG-blocked):** the anim-PHASE — Father fr5 vs retail fr6,
Mother fr0 vs fr2 (the idle-breathe clips 0x62a8c8 loop, but the per-actor START frame is
`0x426ec0`'s RNG draw `rs+0x72 = (rand()*frame_count)>>15`, the `effect-anim-phase` pattern —
needs the scene RNG census, Phase 2).  The character-band SLOT-order interleave is modeled by
port LAYERS (not reproduced slot-for-slot); it is visually exact wherever draws overlap and
invisible where they don't.

## 14. The house→errands TRANSITION: backdrop renders 1:1, the "drift" is a STALE NAV (ckpt 187)

Resolves the ckpt-186 OPEN flag ("the port cutscene drifts BEHIND retail through the
transition") + retires the `cutscene-room-render` debt.  Three findings, all frame/data-verified.

**(a) `cutscene-room-render` is DONE — the house + errands BACKDROPS load + render (debt was
STALE + WRONG).**  The debt claimed "the house lines play over the TOWN-ARRIVAL backdrop
(retail's house is a multi-floor interior)".  BOTH claims false.  The port swaps the backdrop
on the cutscene room-key advance (`main.c:3806` `reload_room_backdrop(cutscene_room_key)`):
capture log — `load_room: room 0x334c8 -> DATA scene 1023` (house) on entering the house room,
`room 0x334dc -> scene 1025` (errands) at chain-complete.  **Frame-verified (`osr_prof dump`):**
port house-dialogue @t1600 (Father's line) vs retail-stairs @t1445 (Mother's line) render the
**IDENTICAL house-EXTERIOR backdrop** — the "Liens" new-house building + the family stood in
front + the portrait bust + the box.  0x334c8 is the house EXTERIOR (approaching the new house),
NOT an interior; the port matches retail.  Feed `house dialogue backdrop renders 1:1`.

**(b) The port-fatherz "drift" = a STALE-NAV artifact, NOT a port-timing bug.**  `nav-full-
errands`'s confirm ticks were extracted from **`retail.osr`** [600,1900] — a capture that **no
longer exists** (only `retail-stairs.osr` + `retail-decomp.osr` remain).  retail.osr's advance
schedule differed from the current reference retail-stairs by ~8-90t, so the port dialogue
advanced at the WRONG ticks → apparent drift.  Proof (`dlg_reconstruct` port-fatherz vs the
retail-stairs `dialogue_timeline`): the port's "I will, I promise!" @t1630 vs retail @1590,
"…moving us in…" @t1699 vs retail @1620 — the port lags by the stale confirm ticks.  The clean
ckpt-179 measurement (matching presses) already found the house dialogue tick-1:1.

**(c) Retail's house→errands ENVELOPE (retail-stairs ground truth, for the −6t entry latency):**
house-close (L17 adv=1634) → HOUSE_EXIT cover to FULL BLACK by ~t1646 (the errands box stages
empty underneath) → **black HOLD ~1646-1655 (~9t = the errands room-load latency)** → reveal
TOP-DOWN t1655-1691 → errands box + L18 "…the store. Cool!" ~t1710-1725.  The **−6t is the
black-HOLD the port skips** (it arms the reveal immediately on chain-complete, no room-load
wait).  A faithful fix = insert the ~9t hold before the errands reveal arm, PORT-DEBT-tagged as
a load-latency stand-in for the unported `0x586010` map-load (a measured stand-in for an
unported subsystem — the allowed exception) — see `cutscene-errands-entry-latency`.

**Nav rebuild attempt (`nav-errands-stairs.jsonl`, re-keyed to retail-stairs skip@full/adv@adv):
FAILED to lockstep — a retail-tick-keyed nav is fundamentally insufficient.**  Captured
`port-navstairs.osr`: L0-L6 tracked with an accumulating +8t drift (the port's speaker-change box
REOPEN adds ~8t vs retail — the old nav's `[spkr-change -8]` compensation), then the L7→L8
RUN-OFF beat + the L9→L10 / L17→L18 FADE beats BLOCK advancement, so confirms placed at retail's
dialogue-ticks fire DURING the beats + are wasted → by L13 the port is 250t+ behind + never
reaches errands.  A faithful transition nav needs BEAT-AWARE placement (the port's beat durations
≠ retail's) — genuinely iterative, low-value for a −6t residual.  **The Z-spam recipe (FRONT
tooling note) stays THE working recipe for the CLAMP verify** (both sides settle by t2420, so the
transition-window drift is irrelevant there).  retail-stairs's 21-line advance schedule is
recorded above (envelope) + in `dialogue_timeline.py <retail-stairs> 600 1900` for a future
beat-aware rebuild.

**(d) LANDED ckpt 188 — the errands entry reveal is EDGES-IN, not center-out; the "9t
room-load HOLD" hypothesis is DISPROVEN (`main.c` `scene_fade_arm` variant 0 → 1).**

The FRONT/§14c "fix ready = insert the ~9t black-hold" was WRONG — a MISREAD of a slow
edges-in reveal as a discrete full-black hold.  A fresh `retail-stairs.osr` pixel measurement
(whole-screen mean brightness + 5-band row profile per sim-tick, `osr_prof` recon; independently
re-measured, not one probe) shows the real house→errands envelope:
- cover (`HOUSE_EXIT`, edges-in) → the central band is lit LAST, snapping to full black **t1650**;
- **full-black core only ~2t** (t1650-1651 mean 0.0; NOT ~9t — §14c's "1646-1655/9t" over-counted
  by starting at t1646 where a central strip is still lit, mean 7.9, and by folding the slow
  reveal's dark early ticks into the "hold");
- **reveal = EDGES-IN** (NOT top-down, NOT center-out): the top edge lights first (t1652), the
  bottom follows (t1655), both grow INWARD, and the vertical CENTER band (rows ~168-336) fills
  **LAST** (~t1685-1691).  Row-band evidence (top→bot, 96px bands): t1670 `[69,22,0,6,77]`,
  t1680 `[70,116,5,74,79]`, t1690 `[70,125,66,86,79]` — center (band 2) is the last to leave 0.
  (§14c's "top-down" saw the top LEAD but missed the bottom joining + center-last.)

So retail has NO ~9t full-black room-load hold; the long "black period" §14c saw is the SLOW
edges-in reveal keeping the CENTER dark while the edges clear.  The port's real bug was the
reveal SHAPE: it armed **CENTER-OUT (variant 0)** — the center opened FIRST — sourced from the
now-DELETED `retail.osr` (ckpt 179).  FIX: force the errands reveal to **variant 1 (edges-in)**
in `main.c` (the same shape as the port's own `HOUSE_EXIT_COVER_VAR=1` cover, its mirror); no
hold inserted (a hold was tried ckpt-188a as `ERRANDS_LOAD_HOLD_TICKS=9` and REVERTED — it made
the port's full-black 13t vs retail's 2t, the wrong mechanism).  Still `PORT-DEBT(cutscene-fade-
variant)`: a forced-variant stand-in (the real variant is an RNG draw `(rand*3)>>15`), now pinned
to retail-stairs' 1 instead of the dead retail.osr's 0.

*VERIFIED off `port-edgesin.osr`* (nav-errands-spam Z-spam): the port reveal now lights the
top+bottom edges first + fills the center LAST, matching retail BAND-FOR-BAND at aligned reveal
phase (absolute ticks differ ~84t via the Z-spam nav):
port t1756 `[70,31,0,11,79]` ≈ retail t1670 `[69,22,0,6,77]`;
port t1770 `[70,125,42,86,79]` ≈ retail t1690 `[70,125,66,86,79]`.
Port full-black is now ~4t (t1735-1739) vs retail ~2t — the residual ~2t is the reveal ONSET
sharpness (the port's freshly-marked MODE_OUT cells age from alpha 31 over ~2-4t; a finer fade-
rate residual within the ±1-2t coalescing slop, folded into `cutscene-fade-variant`).  The reveal
DURATIONS already matched (~40t both), so the fabled "−6t" was ~measurement noise off the old
fuzzy anchors, not a missing hold.  Feed `edgesin_cmp.png` (port|retail edges-in, phase-aligned).

**(d.2) The arrival→house COVER + house-entry REVEAL had the SAME bug (ckpt 188b, `cutscene.c`).**
Re-measuring ALL FOUR town-intro fade shapes vs `retail-stairs.osr` (osr_prof row-band recon) found
each fade's variant is an INDEPENDENT RNG roll and **3 of the 4 were forced to the wrong center-out
(variant 0)** from the dead `retail.osr`:
- (1) arrival→house COVER = **TOP-DOWN** (`ARRIVAL_EXIT_COVER_VAR` 0→2: the sky/roof darkens first,
  sweeping to the ground LAST; scene_fade variant 2 + MODE_IN = `sf_pattern_sweep_down`);
- (2) house-entry REVEAL = **EDGES-IN** (`HOUSE_ENTRY_REVEAL_VAR` 0→1);
- (3) house→errands COVER (`HOUSE_EXIT_COVER_VAR`=1, edges-in) — already right;
- (4) errands entry REVEAL 0→1 (edges-in, §14d above).

VERIFIED off `port-ahfix.osr` (osr_prof row-band): the arrival COVER matches retail **BAND-FOR-BAND**
— port t1258 `[0,0,0,0,0,2,9,23,35,0]` == retail t1224 exactly (top darkens first, bottom last); the
house REVEAL is edges-in (port t1288 `[107,106,40,0,0,0,0,25,118,96]` top+bottom first, center fills
last, matching retail t1250 `[107,47,2,0,0,0,0,0,45,95]`).  Feed `ahfix_cmp.png` (port|retail, cover +
reveal).  The cover's 8-tick full-black core + the reveal's edges-in center-last both reproduce retail
(the earlier `dialogue-advance-early.md` "top-down cover" note for the ARRIVAL was RIGHT in spirit but
the port forced center-out; now corrected).  `cutscene-fade-variant` retire condition unchanged (the
forced shapes await the cast so the live LCG rolls them).

## Tooling note
`osr_prof.exe` (built `make -C tools/osr_view prof` → `build/osr_prof.exe`) reconstructs
any `.osr` frame headless: `osr_prof.exe <file.win> dump <frame_idx> <out.bmp>`, and names the draw
that painted a pixel: `osr_prof.exe <file.win> pick <frame_idx> <px> <py>`.  Map a
sim_tick → frame index with `osr.stream_frames(path)` (enumerate; the Nth FRAMEBEG = index
N).  This is the "LOOK at it yourself" path used to locate the fire above + the bed cull here.
