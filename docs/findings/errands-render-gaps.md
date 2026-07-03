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

## Tooling note
`osr_prof.exe` (built `make -C tools/osr_view prof` → `build/osr_prof.exe`) reconstructs
any `.osr` frame headless: `osr_prof.exe <file.win> dump <frame_idx> <out.bmp>`, and names the draw
that painted a pixel: `osr_prof.exe <file.win> pick <frame_idx> <px> <py>`.  Map a
sim_tick → frame index with `osr.stream_frames(path)` (enumerate; the Nth FRAMEBEG = index
N).  This is the "LOOK at it yourself" path used to locate the fire above + the bed cull here.
